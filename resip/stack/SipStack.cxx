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
        mStatsManager(*this)
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
   mPollGrp(pollGrp?pollGrp:FdPollGrp::create()),
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
   mRunning(false),
   mShuttingDown(false),
   mStatisticsManagerEnabled(true),
   mSocketFunc(socketFunc)
{
   Timer::getTimeMs(); // initalize time offsets
   Random::initialize();
   initNetwork();
   if (pSecurity)
   {
#ifdef USE_SSL
      pSecurity->preload();
#else
      assert(0);
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
      mPollGrp = FdPollGrp::create();
      mPollGrpIsMine=true;
   }

#ifdef USE_SSL
   mSecurity = options.mSecurity ? options.mSecurity : new Security();
   mSecurity->preload();
#else
   mSecurity = 0;
   assert(options.mSecurity==0);
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

   mRunning = false;
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
   if(mRunning)
   {
      return;
   }

   mRunning=true;
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
      assert(!mShuttingDown);
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
   mRunning=false;
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
                        SecurityTypes::TlsClientVerificationMode cvm,
                        bool useEmailAsSIP,
                        SharedPtr<WsConnectionValidator> wsConnectionValidator)
{
   assert(!mShuttingDown);

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
            transport = new TcpTransport(stateMacFifo, port, version, ipInterface, mSocketFunc, *mCompression, transportFlags);
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
                                         useEmailAsSIP);
#else
            CritLog (<< "TLS not supported in this stack. You don't have openssl");
            assert(0);
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
                                          *mCompression);
#else
            CritLog (<< "DTLS not supported in this stack.");
            assert(0);
#endif
            break;

         case WS:
#if defined( USE_SSL )
            transport = new WsTransport(stateMacFifo, 
                  port,
                  version,
                  ipInterface,
                  mSocketFunc,
                  *mCompression,
                  transportFlags,
                  wsConnectionValidator);
#else
            CritLog (<< "WebSockets not supported in this stack. You don't have openssl");
            assert(0);
#endif
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
                  wsConnectionValidator);
#else
            CritLog (<< "WSS not supported in this stack. You don't have openssl");
            assert(0);
#endif
            break;
         default:
            assert(0);
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
   //.dcm. once addTransport starts throwing, need to back out alias
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
         if(DnsUtil::isIpV4Address(ipIfs.back().second)==
                                             (transport->ipVersion()==V4))
         {
            addAlias(ipIfs.back().second, transport->port());
         }
         ipIfs.pop_back();
      }
   }
   mPorts.insert(transport->port());
   mTransactionController->transportSelector().addTransport(transport,true);
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
   assert(!mShuttingDown);
   mDomains.insert(domain + ":" + Data(portToUse));


   if(mUri.host().empty())
   {
      mUri.host()=*mDomains.begin();
   }

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
      assert(0);
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
   assert( hostEnt );

   struct in_addr* addr = (struct in_addr*) hostEnt->h_addr_list[0];
   assert( addr );

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
      assert(0);
      return "127.0.0.1";
   }
   
   struct hostent* hostEnt = gethostbyname( hostName );
   if(!hostEnt)
   {
      ErrLog(<< "gethostbyname failed, returning \"127.0.0.1\"");
      assert(0);
      return "127.0.0.1";
   }
   
   struct in_addr* addr = (struct in_addr*) hostEnt->h_addr_list[0];
   if( !addr )
   {
      ErrLog(<< "gethostbyname returned a hostent* with an empty h_addr_list,"
               " returning \"127.0.0.1\"");
      assert(0);
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
   return (mDomains.count(domain + ":" +
                          Data(port == 0 ? Symbols::DefaultSipPort : port)) != 0);
}

bool
SipStack::isMyPort(int port) const
{
   return mPorts.count(port) != 0;
}

const Uri&
SipStack::getUri() const
{
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
   assert(!mShuttingDown);

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
   assert(!mShuttingDown);

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
   assert(!mShuttingDown);
   mTuSelector.add(message.release(), TimeLimitFifo<Message>::InternalElement);
}

void
SipStack::post(const ApplicationMessage& message)
{
   assert(!mShuttingDown);
   Message* toPost = message.clone();
   mTuSelector.add(toPost, TimeLimitFifo<Message>::InternalElement);
}

void
SipStack::post(const ApplicationMessage& message,  unsigned int secondsLater,
               TransactionUser* tu)
{
   assert(!mShuttingDown);
   postMS(message, secondsLater*1000, tu);
}

void
SipStack::postMS(const ApplicationMessage& message, unsigned int ms,
                 TransactionUser* tu)
{
   assert(!mShuttingDown);
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
   assert(!mShuttingDown);
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
   if(!mShuttingDown && mStatisticsManagerEnabled)
   {
      mStatsManager.process();
   }

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
   Lock lock(mAppTimerMutex);
   strm << "SipStack: " << (this->mSecurity ? "with security " : "without security ")
        << std::endl
        << "domains: " << Inserter(this->mDomains)
        << std::endl
        << " TUFifo size=" << this->mTUFifo.size() << std::endl
        << " Timers size=" << this->mTransactionController->mTimers.size() << std::endl
        << " AppTimers size=" << this->mAppTimers.size() << std::endl
        << " ServerTransactionMap size=" << this->mTransactionController->mServerTransactionMap.size() << std::endl
        << " ClientTransactionMap size=" << this->mTransactionController->mClientTransactionMap.size() << std::endl
        << " Exact Transports=" << Inserter(this->mTransactionController->mTransportSelector.mExactTransports) << std::endl
        << " Any Transports=" << Inserter(this->mTransactionController->mTransportSelector.mAnyInterfaceTransports) << std::endl;
   return strm;
}

EncodeStream&
resip::operator<<(EncodeStream& strm,
const SipStack& stack)
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
