#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifndef WIN32
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "rutil/compat.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Fifo.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Timer.hxx"
#include "rutil/FdPoll.hxx"

#include "rutil/dns/DnsThread.hxx"
#include "resip/stack/Message.hxx"
#include "resip/stack/ShutdownMessage.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/ApplicationMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/Inserter.hxx"
#include "resip/stack/StatisticsManager.hxx"
#include "rutil/AsyncProcessHandler.hxx"
#include "resip/stack/TcpTransport.hxx"
#include "resip/stack/UdpTransport.hxx"
#include "resip/stack/WsTransport.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "resip/stack/TransactionUserMessage.hxx"
#include "resip/stack/TransactionControllerThread.hxx"
#include "resip/stack/TransportSelectorThread.hxx"
#include "rutil/WinLeakCheck.hxx"

#ifdef USE_SSL
#include "resip/stack/ssl/Security.hxx"
#include "resip/stack/ssl/DtlsTransport.hxx"
#include "resip/stack/ssl/TlsTransport.hxx"
#include "resip/stack/ssl/WssTransport.hxx"
#endif

#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4355 )
#endif

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

SipStack::SipStack(const SipStackOptions& options) :
        mTUFifo(TransactionController::MaxTUFifoTimeDepthSecs,
                TransactionController::MaxTUFifoSize),
        mTuSelector(mTUFifo),
        mAppTimers(mTuSelector),
        mStatsManager(*this),
        mNextTransportKey(1)
{
   // WARNING - don't forget to add new member initialization to the init() method
   init(options);
   mTUFifo.setDescription("SipStack::mTUFifo");
}


SipStack::SipStack(Security* pSecurity,
                   const DnsStub::NameserverList& additional,
                   AsyncProcessHandler* handler,
                   bool stateless,
                   AfterSocketCreationFuncPtr socketFunc,
                   Compression *compression,
                   FdPollGrp *pollGrp) :
#ifdef WIN32
   // If PollGrp is not passed in, then EventStackThead isn't being used and application
   // is most likely implementing a select/process loop to drive the stack - in this case
   // we want windows to default to fdset implementation, since the Poll implementation on
   // windows does not support the select/process loop
   mPollGrp(pollGrp?pollGrp:FdPollGrp::create("fdset")),
#else
   mPollGrp(pollGrp?pollGrp:FdPollGrp::create()),
#endif
   mPollGrpIsMine(!pollGrp),
#ifdef USE_SSL
   mSecurity( pSecurity ? pSecurity : new Security()),
#else
   mSecurity(0),
#endif
   mDnsStub(new DnsStub(additional, socketFunc, handler, mPollGrp)),
   mDnsThread(0),
   mCompression(compression ? compression : new Compression(Compression::NONE)),
   mAsyncProcessHandler(handler ? handler : new SelectInterruptor),
   mInterruptorIsMine(!handler),
   mTUFifo(TransactionController::MaxTUFifoTimeDepthSecs,
           TransactionController::MaxTUFifoSize),
   mCongestionManager(0),
   mTuSelector(mTUFifo),
   mAppTimers(mTuSelector),
   mStatsManager(*this),
   mTransactionController(new TransactionController(*this, mAsyncProcessHandler)),
   mTransactionControllerThread(0),
   mTransportSelectorThread(0),
   mInternalThreadsRunning(false),
   mProcessingHasStarted(false),
   mShuttingDown(false),
   mStatisticsManagerEnabled(true),
   mSocketFunc(socketFunc),
   mNextTransportKey(1)
{
   Timer::getTimeMs(); // initalize time offsets
   Random::initialize();
   initNetwork();
   if (pSecurity)
   {
#ifdef USE_SSL
      pSecurity->preload();
#else
      resip_assert(0);
#endif
   }
   
   mTUFifo.setDescription("SipStack::mTUFifo");
   mTransactionController->transportSelector().setPollGrp(mPollGrp);

#if 0
  // .kw. originally tried to share common init() for the two
  // constructors, but too much risk for changing sequencing,
  // first prove out new constructor before merging (or obsoleting)
        mTUFifo(TransactionController::MaxTUFifoTimeDepthSecs,
           TransactionController::MaxTUFifoSize),
        mTuSelector(mTUFifo),
        mAppTimers(mTuSelector),
        mStatsManager(*this)
{
   SipStackOptions options;
   options.mSecurity = pSecurity;
   options.mExtraNameserverList = &additional;
   options.mStateless = stateless;
   options.mSocketFunc = socketFunc;
   options.mCompression = compression;
   options.mPollGrp = pollGrp;
   init(options);
#endif
}

void
SipStack::init(const SipStackOptions& options)
{
   mPollGrpIsMine=false;
   if ( options.mPollGrp )
   {
      mPollGrp = options.mPollGrp;
   }
   else
   {
#ifdef WIN32
      // If PollGrp is not passed in, then EventStackThead isn't being used and application
      // is most likely implementing a select/process loop to drive the stack - in this case
      // we want windows to default to fdset implementation, since the Poll implementation on
      // windows does not support the select/process loop
      mPollGrp = FdPollGrp::create("fdset");
#else
      mPollGrp = FdPollGrp::create();
#endif
      mPollGrpIsMine=true;
   }

#ifdef USE_SSL
   mSecurity = options.mSecurity ? options.mSecurity : new Security();
   mSecurity->preload();
#else
   mSecurity = 0;
   resip_assert(options.mSecurity==0);
#endif

   if(options.mAsyncProcessHandler)
   {
      mAsyncProcessHandler = options.mAsyncProcessHandler;
      mInterruptorIsMine = false;
   }
   else
   {
      mInterruptorIsMine = true;
      mAsyncProcessHandler = new SelectInterruptor;
   }

   mDnsStub = new DnsStub(
         options.mExtraNameserverList
                ? *options.mExtraNameserverList : DnsStub::EmptyNameserverList,
         options.mSocketFunc,
         mAsyncProcessHandler,
         mPollGrp);
   mDnsThread = 0;

   mCompression = options.mCompression
         ? options.mCompression : new Compression(Compression::NONE);

   mCongestionManager = 0;

   // WATCHOUT: the transaction controller constructor will
   // grab the security, DnsStub, compression and statsManager
   mTransactionController = new TransactionController(*this, mAsyncProcessHandler);
   mTransactionController->transportSelector().setPollGrp(mPollGrp);
   mTransactionControllerThread = 0;
   mTransportSelectorThread = 0;

   mInternalThreadsRunning = false;
   mProcessingHasStarted = false;
   mShuttingDown = false;
   mStatisticsManagerEnabled = true;
   mSocketFunc = options.mSocketFunc;

   // .kw. note that stats manager has already called getTimeMs()
   Timer::getTimeMs(); // initalize time offsets
   Random::initialize();
   initNetwork();
}

SipStack::~SipStack()
{
   DebugLog (<< "SipStack::~SipStack()");
   shutdownAndJoinThreads();

   delete mDnsThread;
   mDnsThread=0;
   delete mTransactionControllerThread;
   mTransactionControllerThread=0;
   delete mTransportSelectorThread;
   mTransportSelectorThread=0;

   delete mTransactionController;
#ifdef USE_SSL
   delete mSecurity;
#endif
   delete mCompression;

   delete mDnsStub;
   if (mPollGrpIsMine)
   {
      // delete pollGrp after deleting DNS
      delete mPollGrp;
      mPollGrp=0;
   }

   if(mInterruptorIsMine)
   {
      delete mAsyncProcessHandler;
      mAsyncProcessHandler=0;
   }
}

void 
SipStack::run()
{
   if(mInternalThreadsRunning)
   {
      return;
   }

   mInternalThreadsRunning=true;
   delete mDnsThread;
   mDnsThread=new DnsThread(*mDnsStub);
   mDnsThread->run();

   delete mTransactionControllerThread;
   mTransactionControllerThread=new TransactionControllerThread(*mTransactionController);
   mTransactionControllerThread->run();

   delete mTransportSelectorThread;
   mTransportSelectorThread=new TransportSelectorThread(mTransactionController->transportSelector());
   mTransportSelectorThread->run();
}

void
SipStack::shutdown()
{
   InfoLog (<< "Shutting down sip stack " << this);

   {
      Lock lock(mShutdownMutex);
      resip_assert(!mShuttingDown);
      mShuttingDown = true;
   }

   mTransactionController->shutdown();
}

void 
SipStack::shutdownAndJoinThreads()
{
   if(mDnsThread)
   {
      mDnsThread->shutdown();
      mDnsThread->join();
   }

   if(mTransactionControllerThread)
   {
      mTransactionControllerThread->shutdown();
      mTransactionControllerThread->join();
   }

   if(mTransportSelectorThread)
   {
      mTransportSelectorThread->shutdown();
      mTransportSelectorThread->join();
   }
   mInternalThreadsRunning=false;
}

Transport*
SipStack::addTransport( TransportType protocol,
                        int port,
                        IpVersion version,
                        StunSetting stun,
                        const Data& ipInterface,
                        const Data& sipDomainname,
                        const Data& privateKeyPassPhrase,
                        SecurityTypes::SSLType sslType,
                        unsigned transportFlags,
                        const Data& certificateFilename, const Data& privateKeyFilename,
                        SecurityTypes::TlsClientVerificationMode cvm,
                        bool useEmailAsSIP,
                        SharedPtr<WsConnectionValidator> wsConnectionValidator,
                        SharedPtr<WsCookieContextFactory> wsCookieContextFactory,
                        const Data& netNs)
{
   resip_assert(!mShuttingDown);

   // If address is specified, ensure it is valid
   if(!ipInterface.empty())
   {
      if(version == V6)
      {
         if(!DnsUtil::isIpV6Address(ipInterface))
         {
            ErrLog(<< "Failed to create transport, invalid ipInterface specified (IP address required): V6 "
                   << Tuple::toData(protocol) << " " << port << " on "
                   << ipInterface.c_str());
            throw Transport::Exception("Invalid ipInterface specified (IP address required)", __FILE__,__LINE__);
         }
      }
      else // V4
      {
         if(!DnsUtil::isIpV4Address(ipInterface))
         {
            ErrLog(<< "Failed to create transport, invalid ipInterface specified (IP address required): V4 "
                   << Tuple::toData(protocol) << " " << port << " on "
                   << ipInterface.c_str());
            throw Transport::Exception("Invalid ipInterface specified (IP address required)", __FILE__,__LINE__);
         }
      }
   }

#ifdef USE_NETNS
   if(!netNs.empty() && protocol != TCP)
   {
      ErrLog(<< "Failed to create transport, netns is currently only supported for TCP.  Cannot use netns: " << netNs);
      throw Transport::Exception("netns only supported for TCP", __FILE__,__LINE__);
   }
#endif

   InternalTransport* transport=0;
   Fifo<TransactionMessage>& stateMacFifo = mTransactionController->transportSelector().stateMacFifo();
   try
   {
      switch (protocol)
      {
         case UDP:
            transport = new UdpTransport(stateMacFifo, port, version, stun, ipInterface, mSocketFunc, *mCompression, transportFlags);
            break;
         case TCP:
            transport = 
               new TcpTransport(stateMacFifo, port, version, ipInterface, mSocketFunc, *mCompression, transportFlags, netNs);
            break;
         case TLS:
#if defined( USE_SSL )
            transport = new TlsTransport(stateMacFifo,
                                         port,
                                         version,
                                         ipInterface,
                                         *mSecurity,
                                         sipDomainname,
                                         sslType,
                                         mSocketFunc,
                                         *mCompression,
                                         transportFlags,
                                         cvm,
                                         useEmailAsSIP,
                                         certificateFilename, 
                                         privateKeyFilename,
                                         privateKeyPassPhrase);
#else
            CritLog (<< "Can't add TLS transport: TLS not supported in this stack. You don't have openssl.");
            throw Transport::Exception("Can't add TLS transport: TLS not supported in this stack. You don't have openssl.", __FILE__,__LINE__);
#endif
            break;
         case DTLS:
#if defined( USE_DTLS )
            transport = new DtlsTransport(stateMacFifo,
                                          port,
                                          version, // !jf! stun
                                          ipInterface,
                                          *mSecurity,
                                          sipDomainname,
                                          mSocketFunc,
                                          *mCompression,
                                          certificateFilename, 
                                          privateKeyFilename,
                                          privateKeyPassPhrase);
#else
            CritLog (<< "Can't add DTLS transport: DTLS not supported in this stack.");
            throw Transport::Exception("Can't add DTLS transport: DTLS not supported in this stack.", __FILE__,__LINE__);
#endif
            break;

         case WS:
            transport = new WsTransport(stateMacFifo, 
                  port,
                  version,
                  ipInterface,
                  mSocketFunc,
                  *mCompression,
                  transportFlags,
                  wsConnectionValidator,
                  wsCookieContextFactory);
            break;

         case WSS:
#if defined( USE_SSL )
            transport = new WssTransport(stateMacFifo,
                  port,
                  version,
                  ipInterface,
                  *mSecurity,
                  sipDomainname,
                  sslType,
                  mSocketFunc,
                  *mCompression,
                  transportFlags,
                  cvm,
                  useEmailAsSIP,
                  wsConnectionValidator,
                  wsCookieContextFactory,
                  certificateFilename, 
                  privateKeyFilename,
                  privateKeyPassPhrase);
#else
            CritLog (<< "Can't add WSS transport: Secure Websockets not supported in this stack. You don't have openssl.");
            throw Transport::Exception("Can't add WSS transport: Secure Websockets not supported in this stack. You don't have openssl.", __FILE__,__LINE__);
#endif
            break;
         default:
            CritLog (<< "Can't add unknown transport.");
            throw Transport::Exception("Can't add unknown transport.", __FILE__,__LINE__);
            break;
      }
   }
   catch (BaseException& e)
   {
      ErrLog(<< "Failed to create transport: "
             << (version == V4 ? "V4" : "V6") << " "
             << Tuple::toData(protocol) << " " << port << " on "
             << (ipInterface.empty() ? "ANY" : ipInterface.c_str()) 
             << ": " << e);
      throw;
   }
   addTransport(std::auto_ptr<Transport>(transport));
   return transport;
}

void
SipStack::addTransport(std::auto_ptr<Transport> transport)
{
   // Ensure we will be able to add the transport in the transport selector by ensure we
   // don't have any transport collisions.  Note:  We store two set's here in order to 
   // avoid needing to ask the TransportSelector under some form of locking.
   Tuple tuple(transport->interfaceName(), transport->port(),
               transport->ipVersion(), transport->transport(),
               Data::Empty, // target domain
               transport->netNs());
   if(!isSecure(transport->transport()))
   {
      if(mNonSecureTransports.count(tuple) == 0)
      {
         // All is good - assign key to transport then add to mNonSecureTransports list
         transport->setKey(mNextTransportKey++);
         tuple.mTransportKey = transport->getKey();
         mNonSecureTransports[tuple] = transport.get();
      }
      else
      {
         // Nonsecure transport collision with existing transport
         ErrLog(<< "Failed to add non-secure transport, transport with similar properties already exists: " << tuple);
         throw Transport::Exception("Failed to add non-secure transport, transport with similar properties already exists.", __FILE__,__LINE__);
         return;
      }
   }
   else
   {
      tuple.setTargetDomain(transport->tlsDomain());
      TransportSelector::TlsTransportKey tlsKey(tuple);
      if(mSecureTransports.count(tlsKey) == 0)
      {
         // All is good - assign key to transport then add to mNonSecureTransports list
         transport->setKey(mNextTransportKey++);
         tlsKey.mTuple.mTransportKey = transport->getKey();
         mSecureTransports[tlsKey] = transport.get();
      }
      else
      {
         // Secure transport collision with existing transport
         ErrLog(<< "Failed to add secure transport, transport with similar properties already exists: " << tuple);
         throw Transport::Exception("Failed to add secure transport, transport with similar properties already exists.", __FILE__,__LINE__);
         return;
      }
   }

   if (!transport->interfaceName().empty())
   {
      addAlias(transport->interfaceName(), transport->port());
   }
   else
   {
      // Using INADDR_ANY, get all IP interfaces
      std::list<std::pair<Data, Data> > ipIfs(DnsUtil::getInterfaces());
      if(transport->ipVersion()==V4)
      {
         ipIfs.push_back(std::make_pair<Data,Data>("lo0","127.0.0.1"));
      }
      while(!ipIfs.empty())
      {
         if(DnsUtil::isIpV4Address(ipIfs.back().second) == (transport->ipVersion()==V4))
         {
            addAlias(ipIfs.back().second, transport->port());
         }
         ipIfs.pop_back();
      }
   }
   { 
      Lock lock(mPortsMutex);
      mPorts[transport->port()]++;  // add port / increment reference count
   }

   // Add to CongestionManager if required
   if(mCongestionManager)
   {
       transport->setCongestionManager(mCongestionManager);
   }

   // Set Sip Message Logging Handler if one was provided
   if(mTransportSipMessageLoggingHandler.get())
   {
       transport->setSipMessageLoggingHandler(mTransportSipMessageLoggingHandler);
   }

   if(mProcessingHasStarted)
   {
       // Stack is running.  Need to queue add request for TransactionController Thread
       mTransactionController->addTransport(transport);
   }
   else
   {
       // Stack isn't running yet - just add transport directly on transport selector from this thread
       mTransactionController->transportSelector().addTransport(transport, false /* isStackRunning */); 
   }
}

void 
SipStack::removeTransport(unsigned int transportKey)
{
   Tuple removeTuple;
   Transport* transportToRemove = 0;

   // Find transport using Key in SipStack lists(sets)
   for(NonSecureTransportMap::iterator itNS = mNonSecureTransports.begin(); itNS != mNonSecureTransports.end(); itNS++)
   {
      if(itNS->first.mTransportKey == transportKey)
      {
         removeTuple = itNS->first;
         transportToRemove = itNS->second;
         mNonSecureTransports.erase(itNS);
         break;
      }
   }
   // If not found look in Secure list(set)
   if(!transportToRemove)
   {
      for(SecureTransportMap::iterator itS = mSecureTransports.begin(); itS != mSecureTransports.end(); itS++)
      {
         if(itS->first.mTuple.mTransportKey == transportKey)
         {
            removeTuple = itS->first.mTuple;
            transportToRemove = itS->second;
            mSecureTransports.erase(itS);
            break;
         }
      }
   }
   if(!transportToRemove)
   {
      WarningLog (<< "removeTransport: could not find transport specified by transportKey=" << transportKey);
      return;
   }

   if(mSecureTransports.size() == 0 && mNonSecureTransports.size() == 0)
   {
      // If we have no more transports we can just clear out the mDomains map and mUri
      Lock lock(mDomainsMutex);
      mDomains.clear();
      mUri.host().clear();
      mUri.port() = 0;
   }
   else if(!transportToRemove->interfaceName().empty())
   {
      removeAlias(transportToRemove->interfaceName(), transportToRemove->port());
   }
   else
   {
      // Warning:  This removal could produce unexpected results if the querying of the 
      // current interface addresses yields a different result then when we added the 
      // transport.
      // Using INADDR_ANY, get all IP interfaces
      std::list<std::pair<Data, Data> > ipIfs(DnsUtil::getInterfaces());
      if(transportToRemove->ipVersion()==V4)
      {
         ipIfs.push_back(std::make_pair<Data,Data>("lo0","127.0.0.1"));
      }
      while(!ipIfs.empty())
      {
         if(DnsUtil::isIpV4Address(ipIfs.back().second) == (transportToRemove->ipVersion()==V4))
         {
            removeAlias(ipIfs.back().second, transportToRemove->port());
         }
         ipIfs.pop_back();
      }
   }
   
   // Remove from port map if reference count is 0
   {
      Lock lock(mPortsMutex);
      std::map<int, unsigned int>::iterator itP = mPorts.find(transportToRemove->port());
      if(itP != mPorts.end())
      {
         // Decrement reference count and erase if 0
         if(--itP->second == 0)
         {
            mPorts.erase(itP);
         }
      }
   }

   if(mProcessingHasStarted)
   {
       // Stack is running.  Need to queue remove request for TransactionController Thread
       mTransactionController->removeTransport(transportKey);
   }
   else
   {
       // Stack isn't running yet - just remove transport directly on transport selector from this thread
       mTransactionController->transportSelector().removeTransport(transportKey); 
   }
}

Fifo<TransactionMessage>&
SipStack::stateMacFifo()
{
   return mTransactionController->transportSelector().stateMacFifo();
}

void
SipStack::addAlias(const Data& domain, int port)
{
   int portToUse = (port == 0) ? Symbols::DefaultSipPort : port;

   DebugLog (<< "Adding domain alias: " << domain << ":" << portToUse);
   resip_assert(!mShuttingDown);

   Lock lock(mDomainsMutex);
   mDomains[domain + ":" + Data(portToUse)]++;

   if(mUri.host().empty())
   {
      mUri.host() = domain;
      mUri.port() = portToUse;
   }
}

void
SipStack::removeAlias(const Data& domain, int port)
{
   int portToUse = (port == 0) ? Symbols::DefaultSipPort : port;

   DebugLog (<< "Removing domain alias: " << domain << ":" << portToUse);
   resip_assert(!mShuttingDown);

   Lock lock(mDomainsMutex);
   DomainsMap::iterator it = mDomains.find(domain + ":" + Data(portToUse));
   if(it != mDomains.end())
   {
      if(--it->second == 0)
      {
         mDomains.erase(it);
      }
   }

   // TODO - could reset mUri to be first item in Domain map - would need
   // to seperate domain name and port though.  Not sure who is using mUri
   // anyway.
}

Data
SipStack::getHostname()
{
   // if you change this, please #def old version for windows
   char hostName[1024];
   int err =  gethostname( hostName, sizeof(hostName) );
   if(err != 0)
   {
      ErrLog(<< "gethostname failed with return " << err << " Returning "
            "\"localhost\"");
      resip_assert(0);
      return "localhost";
   }
   
   struct hostent* hostEnt = gethostbyname( hostName );
   if ( !hostEnt )
   {
      // this can fail when there is no name server
      // !cj! - need to decided what error to return
      ErrLog( << "gethostbyname failed - name server is probably down" );
      return "localhost";
   }
   resip_assert( hostEnt );

   struct in_addr* addr = (struct in_addr*) hostEnt->h_addr_list[0];
   resip_assert( addr );

   // if you change this, please #def old version for windows
   char* addrA = inet_ntoa( *addr );
   Data ret(addrA);

   Data retHost( hostEnt->h_name );

   return retHost;
}


Data
SipStack::getHostAddress()
{
   // if you change this, please #def old version for windows
   char hostName[1024];
   int err =  gethostname( hostName, sizeof(hostName) );
   if(err != 0)
   {
      ErrLog(<< "gethostname failed with return " << err << " Returning "
            "\"127.0.0.1\"");
      resip_assert(0);
      return "127.0.0.1";
   }
   
   struct hostent* hostEnt = gethostbyname( hostName );
   if(!hostEnt)
   {
      ErrLog(<< "gethostbyname failed, returning \"127.0.0.1\"");
      resip_assert(0);
      return "127.0.0.1";
   }
   
   struct in_addr* addr = (struct in_addr*) hostEnt->h_addr_list[0];
   if( !addr )
   {
      ErrLog(<< "gethostbyname returned a hostent* with an empty h_addr_list,"
               " returning \"127.0.0.1\"");
      resip_assert(0);
      return "127.0.0.1";
   }
   
   // if you change this, please #def old version for windows 
   char* addrA = inet_ntoa( *addr );
   Data ret(addrA);

   //Data retHost( hostEnt->h_name );

   return ret;
}

bool
SipStack::isMyDomain(const Data& domain, int port) const
{
   Lock lock(mDomainsMutex);
   return (mDomains.count(domain + ":" +
                          Data(port == 0 ? Symbols::DefaultSipPort : port)) != 0);
}

bool
SipStack::isMyPort(int port) const
{
   Lock lock(mPortsMutex);
   return mPorts.count(port) != 0;
}

const Uri&
SipStack::getUri() const
{
   Lock lock(mDomainsMutex);
   if(mUri.host().empty())
   {
      CritLog(<< "There are no associated transports");
      throw Exception("No associated transports", __FILE__, __LINE__);
   }

   return mUri;
}

void
SipStack::send(const SipMessage& msg, TransactionUser* tu)
{
   DebugLog (<< "SEND: " << msg.brief());
   //DebugLog (<< msg);
   //assert(!mShuttingDown);

   SipMessage* toSend = static_cast<SipMessage*>(msg.clone());
   if (tu)
   {
      toSend->setTransactionUser(tu);
   }
   toSend->setFromTU();

   mTransactionController->send(toSend);
}

void
SipStack::send(std::auto_ptr<SipMessage> msg, TransactionUser* tu)
{
   DebugLog (<< "SEND: " << msg->brief());

   if (tu)
   {
      msg->setTransactionUser(tu);
   }
   msg->setFromTU();

   mTransactionController->send(msg.release());
}

void
SipStack::sendTo(std::auto_ptr<SipMessage> msg, const Uri& uri, TransactionUser* tu)
{
   if (tu) msg->setTransactionUser(tu);
   msg->setForceTarget(uri);
   msg->setFromTU();

   mTransactionController->send(msg.release());
}

void
SipStack::sendTo(std::auto_ptr<SipMessage> msg, const Tuple& destination, TransactionUser* tu)
{
   resip_assert(!mShuttingDown);

   if (tu) msg->setTransactionUser(tu);
   msg->setDestination(destination);
   msg->setFromTU();

   mTransactionController->send(msg.release());
}

// this is only if you want to send to a destination not in the route. You
// probably don't want to use it.
void
SipStack::sendTo(const SipMessage& msg, const Uri& uri, TransactionUser* tu)
{
   //assert(!mShuttingDown);

   SipMessage* toSend = static_cast<SipMessage*>(msg.clone());
   if (tu) toSend->setTransactionUser(tu);
   toSend->setForceTarget(uri);
   toSend->setFromTU();

   mTransactionController->send(toSend);
}

// this is only if you want to send to a destination not in the route. You
// probably don't want to use it.
void
SipStack::sendTo(const SipMessage& msg, const Tuple& destination, TransactionUser* tu)
{
   resip_assert(!mShuttingDown);

   SipMessage* toSend = static_cast<SipMessage*>(msg.clone());
   if (tu) toSend->setTransactionUser(tu);
   toSend->setDestination(destination);
   toSend->setFromTU();

   mTransactionController->send(toSend);
}

void
SipStack::checkAsyncProcessHandler()
{
   if (mAsyncProcessHandler)
   {
      mAsyncProcessHandler->handleProcessNotification();
   }
}

void
SipStack::post(std::auto_ptr<ApplicationMessage> message)
{
   resip_assert(!mShuttingDown);
   mTuSelector.add(message.release(), TimeLimitFifo<Message>::InternalElement);
}

void
SipStack::post(const ApplicationMessage& message)
{
   resip_assert(!mShuttingDown);
   Message* toPost = message.clone();
   mTuSelector.add(toPost, TimeLimitFifo<Message>::InternalElement);
}

void
SipStack::post(const ApplicationMessage& message,  unsigned int secondsLater,
               TransactionUser* tu)
{
   resip_assert(!mShuttingDown);
   postMS(message, secondsLater*1000, tu);
}

void
SipStack::postMS(const ApplicationMessage& message, unsigned int ms,
                 TransactionUser* tu)
{
   resip_assert(!mShuttingDown);
   Message* toPost = message.clone();
   if (tu) toPost->setTransactionUser(tu);
   Lock lock(mAppTimerMutex);
   mAppTimers.add(ms,toPost);
   //.dcm. timer update rather than process cycle...optimize by checking if sooner
   //than current timeTillNextProcess?
   checkAsyncProcessHandler();
}

void
SipStack::post(std::auto_ptr<ApplicationMessage> message,
               unsigned int secondsLater,
               TransactionUser* tu)
{
   postMS(message, secondsLater*1000, tu);
}


void
SipStack::postMS( std::auto_ptr<ApplicationMessage> message,
                  unsigned int ms,
                  TransactionUser* tu)
{
   resip_assert(!mShuttingDown);
   if (tu) message->setTransactionUser(tu);
   Lock lock(mAppTimerMutex);
   mAppTimers.add(ms, message.release());
   //.dcm. timer update rather than process cycle...optimize by checking if sooner
   //than current timeTillNextProcess?
   checkAsyncProcessHandler();
}

void
SipStack::abandonServerTransaction(const Data& tid)
{
   mTransactionController->abandonServerTransaction(tid);
}

void
SipStack::cancelClientInviteTransaction(const Data& tid)
{
   mTransactionController->cancelClientInviteTransaction(tid);
}

bool
SipStack::hasMessage() const
{
   return mTUFifo.messageAvailable();
}

SipMessage*
SipStack::receive()
{
   // Check to see if a message is available and if it is return the
   // waiting message. Otherwise, return 0
   if (mTUFifo.messageAvailable())
   {
      // we should only ever have SIP messages on the TU Fifo
      // unless we've registered for termination messages.
      Message* msg = mTUFifo.getNext();
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      if (sip)
      {
         DebugLog (<< "RECV: " << sip->brief());
         return sip;
      }
      else
      {
         // assert(0); // !CJ! removed the assert - happens 1 minute after
         // stack starts up
         delete msg;
         return 0;
      }
   }
   else
   {
      return 0;
   }
}

Message*
SipStack::receiveAny()
{
   // Check to see if a message is available and if it is return the
   // waiting message. Otherwise, return 0
   if (mTUFifo.messageAvailable())
   {
      // application messages can flow through
      Message* msg = mTUFifo.getNext();
      SipMessage* sip=dynamic_cast<SipMessage*>(msg);
      if (sip)
      {
         DebugLog (<< "RECV: " << sip->brief());
      }
      return msg;
   }
   else
   {
      return 0;
   }
}

void
SipStack::setFallbackPostNotify(AsyncProcessHandler *handler) 
{
   mTuSelector.setFallbackPostNotify(handler);
}

/* Called from external epoll (e.g., EventStackThread) */
void
SipStack::processTimers()
{
   if(!mTransactionControllerThread)
   {
      mTransactionController->process();
   }

   if(!mDnsThread)
   {
      mDnsStub->processTimers();
   }

   if(!mTransportSelectorThread)
   {
      mTransactionController->transportSelector().process();
   }

   mTuSelector.process();
   Lock lock(mAppTimerMutex);
   mAppTimers.process();
}

/* Called for internal epoll and non-epoll (e.g., StackThread) */
void
SipStack::process(FdSet& fdset)
{
   mPollGrp->processFdSet(fdset);
   processTimers();
}

bool 
SipStack::process(unsigned int timeoutMs)
{
   // Go ahead and do this first. Should cut down on how frequently we call 
   // waitAndProcess() with a timeout of 0, which should improve efficiency 
   // somewhat.
   processTimers();
   bool result=mPollGrp->waitAndProcess(resipMin(timeoutMs, getTimeTillNextProcessMS()));
   return result;
}

/// returns time in milliseconds when process next needs to be called
unsigned int
SipStack::getTimeTillNextProcessMS()
{
   Lock lock(mAppTimerMutex);
   mProcessingHasStarted = true;

   unsigned int dnsNextProcess = (mDnsThread ? 
                           INT_MAX : mDnsStub->getTimeTillNextProcessMS());
   unsigned int tcNextProcess = mTransactionControllerThread ? INT_MAX : 
                           mTransactionController->getTimeTillNextProcessMS();
   unsigned int tsNextProcess = mTransportSelectorThread ? INT_MAX : mTransactionController->transportSelector().getTimeTillNextProcessMS();

   return resipMin(Timer::getMaxSystemTimeWaitMs(),
            resipMin(dnsNextProcess,
               resipMin(tcNextProcess,
                  resipMin(tsNextProcess,
                     resipMin(mTuSelector.getTimeTillNextProcessMS(), mAppTimers.msTillNextTimer())))));
}

void
SipStack::buildFdSet(FdSet& fdset)
{
   mPollGrp->buildFdSet(fdset);
}

Security*
SipStack::getSecurity() const
{
    return mSecurity;
}

void
SipStack::setStatisticsInterval(unsigned long seconds)
{
   mStatsManager.setInterval(seconds);
}

void
SipStack::zeroOutStatistics()
{
   if(statisticsManagerEnabled())
   {
      mTransactionController->zeroOutStatistics();
   }
}

bool
SipStack::pollStatistics()
{
   if(statisticsManagerEnabled())
   {
      mTransactionController->pollStatistics();
      return true;
   }
   return false;
}

void
SipStack::registerTransactionUser(TransactionUser& tu)
{
   mTuSelector.registerTransactionUser(tu);
}

void
SipStack::requestTransactionUserShutdown(TransactionUser& tu)
{
   mTuSelector.requestTransactionUserShutdown(tu);
   checkAsyncProcessHandler();
}

void
SipStack::unregisterTransactionUser(TransactionUser& tu)
{
   mTuSelector.unregisterTransactionUser(tu);
   checkAsyncProcessHandler();
}

void
SipStack::registerMarkListener(MarkListener* listener)
{
   mTransactionController->registerMarkListener(listener);
}

void
SipStack::unregisterMarkListener(MarkListener* listener)
{
   mTransactionController->unregisterMarkListener(listener);
}

DnsStub&
SipStack::getDnsStub() const
{
   return *mDnsStub;
}

void
SipStack::setEnumSuffixes(const std::vector<Data>& suffixes)
{
   mDnsStub->setEnumSuffixes(suffixes);
}

void
SipStack::setEnumDomains(const std::map<Data,Data>& domains)
{
   mDnsStub->setEnumDomains(domains);
}

void
SipStack::clearDnsCache()
{
   mDnsStub->clearDnsCache();
}

void
SipStack::logDnsCache()
{
   mDnsStub->logDnsCache();
}

void 
SipStack::getDnsCacheDump(std::pair<unsigned long, unsigned long> key, GetDnsCacheDumpHandler* handler)
{
   mDnsStub->getDnsCacheDump(key, handler);
}

volatile bool&
SipStack::statisticsManagerEnabled()
{
   return mStatisticsManagerEnabled;
}

const bool
SipStack::statisticsManagerEnabled() const
{
   return mStatisticsManagerEnabled;
}

EncodeStream&
SipStack::dump(EncodeStream& strm)  const
{
   strm << "SipStack: " << (this->mSecurity ? "with security " : "without security ") << std::endl;
   {
      Lock lock(mDomainsMutex);
      strm << "domains: " << Inserter(this->mDomains) << std::endl;
   }
   strm << " TUFifo size=" << this->mTUFifo.size() << std::endl
        << " Timers size=" << this->mTransactionController->mTimers.size() << std::endl;
   {
      Lock lock(mAppTimerMutex);
      strm << " AppTimers size=" << this->mAppTimers.size() << std::endl;
   }
   strm << " ServerTransactionMap size=" << this->mTransactionController->mServerTransactionMap.size() << std::endl
        << " ClientTransactionMap size=" << this->mTransactionController->mClientTransactionMap.size() << std::endl
        // !slg! TODO - There is technically a threading concern with the following three lines and the runtime addTransport call
        << " Exact Transports=" << Inserter(this->mTransactionController->mTransportSelector.mExactTransports) << std::endl
        << " Any Transports=" << Inserter(this->mTransactionController->mTransportSelector.mAnyInterfaceTransports) << std::endl
        << " TLS Transports=" << Inserter(this->mTransactionController->mTransportSelector.mTlsTransports) << std::endl;
   return strm;
}

EncodeStream&
resip::operator<<(EncodeStream& strm, const SipStack& stack)
{
   return stack.dump(strm);
}

void 
SipStack::terminateFlow(const resip::Tuple& flow)
{
   mTransactionController->terminateFlow(flow);
}

void 
SipStack::enableFlowTimer(const resip::Tuple& flow)
{
   mTransactionController->enableFlowTimer(flow);
}

/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * ====================================================================
 *
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 * vi: set shiftwidth=3 expandtab:
 */
