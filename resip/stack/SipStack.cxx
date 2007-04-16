#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#ifndef WIN32
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "rutil/compat.hxx"
#include "rutil/Data.hxx"
#include "rutil/Fifo.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Timer.hxx"

#include "resip/stack/Message.hxx"
#include "resip/stack/Security.hxx"
#include "resip/stack/ShutdownMessage.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/ApplicationMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/Inserter.hxx"
#include "resip/stack/StatisticsManager.hxx"
#include "resip/stack/AsyncProcessHandler.hxx"
#include "resip/stack/TcpTransport.hxx"
#include "resip/stack/TlsTransport.hxx"
#include "resip/stack/UdpTransport.hxx"
#include "resip/stack/DtlsTransport.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "resip/stack/TransactionUserMessage.hxx"
#include "rutil/WinLeakCheck.hxx"

#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4355 )
#endif

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

SipStack::SipStack(Security* pSecurity, 
                   const DnsStub::NameserverList& additional,
                   AsyncProcessHandler* handler, 
                   bool stateless,
                   AfterSocketCreationFuncPtr socketFunc,
                   Compression *compression
   ) : 
#ifdef USE_SSL
   mSecurity( pSecurity ? pSecurity : new Security()),
#else
   mSecurity(0),
#endif
   mDnsStub(new DnsStub(additional, socketFunc)),
   mCompression(compression ? compression : new Compression(Compression::NONE)),
   mAsyncProcessHandler(handler),
   mTUFifo(TransactionController::MaxTUFifoTimeDepthSecs,
           TransactionController::MaxTUFifoSize),
   mAppTimers(mTuSelector),
   mStatsManager(*this),
   mTransactionController(*this),
   mShuttingDown(false),
   mStatisticsManagerEnabled(true),
   mTuSelector(mTUFifo),
   mSocketFunc(socketFunc)
{
   Timer::getTimeMs(); // initalize time offsets
   Random::initialize();
   initNetwork();
   if (pSecurity)
   {
      pSecurity->preload();
   }

   assert(!mShuttingDown);
}

SipStack::~SipStack()
{
   DebugLog (<< "SipStack::~SipStack()");
#ifdef USE_SSL
   delete mSecurity;
#endif
   delete mCompression;
   delete mDnsStub;
}

void
SipStack::shutdown()
{
   InfoLog (<< "Shutting down sip stack " << this);

   static Mutex shutDownMutex;
   {
      Lock lock(shutDownMutex);
      assert(!mShuttingDown);
      mShuttingDown = true;
   }

   mTransactionController.shutdown();
}

Transport*
SipStack::addTransport( TransportType protocol,
                        int port, 
                        IpVersion version,
                        StunSetting stun,
                        const Data& ipInterface, 
                        const Data& sipDomainname,
                        const Data& privateKeyPassPhrase,
                        SecurityTypes::SSLType sslType)
{
   assert(!mShuttingDown);
   assert( port >  0 );
   InternalTransport* transport=0;
   Fifo<TransactionMessage>& stateMacFifo = mTransactionController.transportSelector().stateMacFifo();   
   try
   {
      switch (protocol)
      {
         case TCP:
            transport = new TcpTransport(stateMacFifo, port, version, ipInterface, *mCompression);
            break;
         case UDP:
            assert(mSocketFunc==NULL);
            transport = new UdpTransport(stateMacFifo, port, version, stun, ipInterface, mSocketFunc, *mCompression);
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
                                         *mCompression);
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
                                          *mCompression);
#else
            CritLog (<< "DTLS not supported in this stack.");
            assert(0);
#endif
            break;
         default:
            assert(0);
            break;
      }
   }
   catch (Transport::Exception& )
   {
      ErrLog(<< "Failed to create transport: "
             << (version == V4 ? "V4" : "V6") << " "
             << Tuple::toData(protocol) << " " << port << " on "
             << (ipInterface.empty() ? "ANY" : ipInterface.c_str()));
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
   mPorts.insert(transport->port());
   mTransactionController.transportSelector().addTransport(transport);
}

Fifo<TransactionMessage>& 
SipStack::stateMacFifo()
{
   return mTransactionController.transportSelector().stateMacFifo();
}

void
SipStack::addAlias(const Data& domain, int port)
{
   int portToUse = (port == 0) ? Symbols::DefaultSipPort : port;
   
   DebugLog (<< "Adding domain alias: " << domain << ":" << portToUse);
   assert(!mShuttingDown);
   mDomains.insert(domain + ":" + Data(portToUse));
}

Data 
SipStack::getHostname()
{
   // if you change this, please #def old version for windows 
   char hostName[1024];
   int err =  gethostname( hostName, sizeof(hostName) );
   assert( err == 0 );
   
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
   assert( err == 0 );
   
   struct hostent* hostEnt = gethostbyname( hostName );
   assert( hostEnt );
   
   struct in_addr* addr = (struct in_addr*) hostEnt->h_addr_list[0];
   assert( addr );
   
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
   if (mDomains.empty())
   {
      CritLog(<< "There are no associated transports");
      throw Exception("No associated transports", __FILE__, __LINE__);
   }

   static Uri myUri("sip:" + *mDomains.begin());

   return myUri;
}

void 
SipStack::send(const SipMessage& msg, TransactionUser* tu)
{
   DebugLog (<< "SEND: " << msg.brief());
   //DebugLog (<< msg);
   //assert(!mShuttingDown);
   
   SipMessage* toSend = new SipMessage(msg);
   if (tu) 
   {
      toSend->setTransactionUser(tu);
   }         
   toSend->setFromTU();

   mTransactionController.send(toSend);
   checkAsyncProcessHandler();
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

   mTransactionController.send(msg.release());
   checkAsyncProcessHandler();
}

void
SipStack::sendTo(std::auto_ptr<SipMessage> msg, const Uri& uri, TransactionUser* tu)
{
   if (tu) msg->setTransactionUser(tu);
   msg->setForceTarget(uri);
   msg->setFromTU();

   mTransactionController.send(msg.release());
   checkAsyncProcessHandler();
}

void 
SipStack::sendTo(std::auto_ptr<SipMessage> msg, const Tuple& destination, TransactionUser* tu)
{
   assert(!mShuttingDown);
   assert(destination.transport);
   
   if (tu) msg->setTransactionUser(tu);
   msg->setDestination(destination);
   msg->setFromTU();

   mTransactionController.send(msg.release());
   checkAsyncProcessHandler();
}

// this is only if you want to send to a destination not in the route. You
// probably don't want to use it. 
void 
SipStack::sendTo(const SipMessage& msg, const Uri& uri, TransactionUser* tu)
{
   //assert(!mShuttingDown);

   SipMessage* toSend = new SipMessage(msg);
   if (tu) toSend->setTransactionUser(tu);
   toSend->setForceTarget(uri);
   toSend->setFromTU();

   mTransactionController.send(toSend);
   checkAsyncProcessHandler();
}

// this is only if you want to send to a destination not in the route. You
// probably don't want to use it. 
void 
SipStack::sendTo(const SipMessage& msg, const Tuple& destination, TransactionUser* tu)
{
   assert(!mShuttingDown);
   assert(destination.transport);
   
   //SipMessage* toSend = new SipMessage(msg);
   SipMessage* toSend = dynamic_cast<SipMessage*>(msg.clone());
   if (tu) toSend->setTransactionUser(tu);
   toSend->setDestination(destination);
   toSend->setFromTU();

   mTransactionController.send(toSend);
   checkAsyncProcessHandler();
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
SipStack::post(const ApplicationMessage& message)
{
   assert(!mShuttingDown);
   SharedPtr<Message> toPost(message.clone());
   //mTUFifo.add(toPost, TimeLimitFifo<SharedPtr<Message> >::InternalElement);
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
   mAppTimers.add(Timer(ms, toPost));
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
   mAppTimers.add(Timer(ms, message.release()));
   //.dcm. timer update rather than process cycle...optimize by checking if sooner
   //than current timeTillNextProcess?
   checkAsyncProcessHandler();
}


bool
SipStack::hasMessage() const
{
   return mTUFifo.messageAvailable();
}

SharedPtr<SipMessage> 
SipStack::receive()
{
   // Check to see if a message is available and if it is return the 
   // waiting message. Otherwise, return 0
   if (mTUFifo.messageAvailable())
   {
      // we should only ever have SIP messages on the TU Fifo
      // unless we've registered for termination messages. 
      SharedPtr<Message> msg(mTUFifo.getNext());
      SharedPtr<SipMessage> sip(msg, dynamic_cast_tag());
      if (sip)
      {
         DebugLog (<< "RECV: " << sip->brief());
         return sip;
      }
      else
      {
         // assert(0); // !CJ! removed the assert - happens 1 minute after
         // stack starts up
         // delete msg; // !nash! let smart_ptr delete it
         return SharedPtr<SipMessage>();
      }
   }
   else
   {
      return SharedPtr<SipMessage>();
   }
}

SharedPtr<Message>
SipStack::receiveAny()
{
   // Check to see if a message is available and if it is return the 
   // waiting message. Otherwise, return 0
   if (mTUFifo.messageAvailable())
   {
      // application messages can flow through
      SharedPtr<Message> msg(mTUFifo.getNext());
      SharedPtr<SipMessage> sip(msg, dynamic_cast_tag());
      if (sip)
      {
         DebugLog (<< "RECV: " << sip->brief());
      }
      return msg;
   }
   else
   {
      return SharedPtr<Message>();
   }
}

void 
SipStack::process(FdSet& fdset)
{
   if(!mShuttingDown && mStatisticsManagerEnabled)
   {
      mStatsManager.process();
   }
   mTransactionController.process(fdset);
   mTuSelector.process();
   mDnsStub->process(fdset);
   
   Lock lock(mAppTimerMutex); 
   mAppTimers.process();
}

/// returns time in milliseconds when process next needs to be called 
unsigned int 
SipStack::getTimeTillNextProcessMS()
{
   Lock lock(mAppTimerMutex);
   return resipMin(mTransactionController.getTimeTillNextProcessMS(),
                   resipMin(mTuSelector.getTimeTillNextProcessMS(), mAppTimers.msTillNextTimer()));
} 

void 
SipStack::buildFdSet(FdSet& fdset)
{
   mTransactionController.buildFdSet(fdset);
   mDnsStub->buildFdSet(fdset);
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
   mTransactionController.registerMarkListener(listener);
}

void
SipStack::unregisterMarkListener(MarkListener* listener)
{
   mTransactionController.unregisterMarkListener(listener);
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
SipStack::clearDnsCache()
{
   mDnsStub->clearDnsCache();
}

void
SipStack::logDnsCache()
{
   mDnsStub->logDnsCache();
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

std::ostream& 
SipStack::dump(std::ostream& strm)  const
{
   Lock lock(mAppTimerMutex);
   strm << "SipStack: " << (this->mSecurity ? "with security " : "without security ")
        << std::endl
        << "domains: " << Inserter(this->mDomains)
        << std::endl
        << " TUFifo size=" << this->mTUFifo.size() << std::endl
        << " Timers size=" << this->mTransactionController.mTimers.size() << std::endl
        << " AppTimers size=" << this->mAppTimers.size() << std::endl
        << " ServerTransactionMap size=" << this->mTransactionController.mServerTransactionMap.size() << std::endl
        << " ClientTransactionMap size=" << this->mTransactionController.mClientTransactionMap.size() << std::endl
        << " Exact Transports=" << Inserter(this->mTransactionController.mTransportSelector.mExactTransports) << std::endl
        << " Any Transports=" << Inserter(this->mTransactionController.mTransportSelector.mAnyInterfaceTransports) << std::endl;
   return strm;
}

std::ostream& 
resip::operator<<(std::ostream& strm, 
const SipStack& stack) 
{
   return stack.dump(strm);
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
 */
