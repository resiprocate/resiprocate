/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#ifndef WIN32
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#ifdef RESIP_USE_SCTP
#include <netinet/sctp.h>
#endif
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
#include "resip/stack/TransactionUser.hxx"
#include "resip/stack/TransactionUserMessage.hxx"
#include "resip/stack/TransactionControllerThread.hxx"
#include "resip/stack/TransportSelectorThread.hxx"
#include "rutil/WinLeakCheck.hxx"

#ifdef USE_SSL
#include "resip/stack/ssl/Security.hxx"
#include "resip/stack/ssl/DtlsTransport.hxx"
#include "resip/stack/ssl/TlsTransport.hxx"
#endif

#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4355 )
#endif

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

SipStack::SipStack(Security* pSecurity, 
                   bool threadedStack,
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
   mDnsStub(new DnsStub(additional, socketFunc, handler)),
   mDnsThread(threadedStack ? new DnsThread(*mDnsStub) : 0),
   mCompression(compression ? compression : new Compression(Compression::NONE)),
   mAsyncProcessHandler(handler),
   mTUFifo(),
   mAppTimers(mTuSelector),
   mNextTimer(0),
   mStatsManager(*this,5),
   mCongestionManager(0),
   mTransactionController(*this),
   mTransactionControllerThread(threadedStack ? new TransactionControllerThread(mTransactionController) : 0),
   mTransportSelectorThread(threadedStack ? new TransportSelectorThread(mTransactionController.transportSelector()) : 0),
   mRunning(false),
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
#ifdef USE_SSL
      pSecurity->preload();
#else
      assert(0);
#endif
   }

   assert(!mShuttingDown);
   
   mTUFifo.setDescription("SipStack::mTUFifo");

   mTransactionController.mThreaded=threadedStack;
}

SipStack::~SipStack()
{
   DebugLog (<< "SipStack::~SipStack()");
#ifdef USE_SSL
   delete mSecurity;
#endif
   delete mCompression;

   join();

   delete mDnsStub;
}

void 
SipStack::run()
{
   if(mRunning)
   {
      return;
   }

   mRunning=true;
   if(mDnsThread)
   {
      mDnsThread->run();
   }

   if(mTransactionControllerThread)
   {
      mTransactionControllerThread->run();
   }

   if(mTransportSelectorThread)
   {
      mTransportSelectorThread->run();
   }
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

   mTransactionController.shutdown();
}

void 
SipStack::join()
{
   if(mDnsThread)
   {
      mDnsThread->shutdown();
      mDnsThread->join();
      delete mDnsThread;
      mDnsThread=0;
   }

   if(mTransactionControllerThread)
   {
      mTransactionControllerThread->shutdown();
      mTransactionControllerThread->join();
      delete mTransactionControllerThread;
      mTransactionControllerThread=0;
   }

   if(mTransportSelectorThread)
   {
      mTransportSelectorThread->shutdown();
      mTransportSelectorThread->join();
      delete mTransportSelectorThread;
      mTransportSelectorThread=0;
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
                        bool hasOwnThread)
{
   if(mShuttingDown)
   {
      return 0;
   }
   

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
   Fifo<TransactionMessage>& stateMacFifo = mTransactionController.transportSelector().stateMacFifo();   
   try
   {
      switch (protocol)
      {
         case UDP:
            transport = new UdpTransport(stateMacFifo, port, version, stun, ipInterface, mSocketFunc, *mCompression, hasOwnThread);
            break;
         case TCP:
            transport = new TcpTransport(stateMacFifo, port, version, ipInterface, mSocketFunc, *mCompression, transportFlags, hasOwnThread);
            break;
         case SCTP:
#if defined( HAVE_SCTP )
            transport = new TcpTransport(stateMacFifo, port, version, ipInterface, mSocketFunc, *mCompression, transportFlags, hasOwnThread, true);
#else
            CritLog (<< "SCTP not supported in this stack.");
            assert(0);
#endif
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
                                         hasOwnThread);
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
                                          *mCompression, 
                                          hasOwnThread);
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
   catch (Transport::Exception& e)
   {
      ErrLog(<< "Failed to create transport: "
             << (version == V4 ? "V4" : "V6") << " "
             << Tuple::toData(protocol) << " " << port << " on "
             << (ipInterface.empty() ? "ANY" : ipInterface.c_str()));
      throw e;
   }

#if defined(DISALLOW_INADDR_ANY)
   if(transport->getTuple().isAnyInterface())
   {
      ErrLog(<< "Something attempted to add a transport on INADDR_ANY"
                  ", but this is not allowed in the current stack.");
      delete transport;
      transport = 0;
      assert(0);
   }
#endif   
   
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
   mTransactionController.transportSelector().addTransport(transport, 
                                                            !mRunning);
}

Fifo<TransactionMessage>& 
SipStack::stateMacFifo()
{
   return mTransactionController.transportSelector().stateMacFifo();
}

void
SipStack::addAlias(const Data& domain, int port)
{
   assert(!mShuttingDown);

   resip::Data copy(domain);
   copy.lowercase();
   if(DnsUtil::isIpV6Address(copy))
   {
      copy = DnsUtil::canonicalizeIpV6Address(copy);
   }

   DebugLog (<< "Adding domain alias: " << copy << ":" << port);

   // !bwc! General alias (port-insensitive)
   mDomains.insert(copy);


   if(mUri.host().empty())
   {
      mUri.host()=copy;
   }

   if(port)
   {
      // !bwc! Port-sensitive alias.
      mDomains.insert(copy + ":" + Data(port));
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
   resip::Data copy(domain);
   copy.lowercase();

   if(DnsUtil::isIpV6Address(copy))
   {
      copy = DnsUtil::canonicalizeIpV6Address(copy);
   }

   DebugLog (<< "Checking domain aliases for: " << copy << ":" << port);

   if(port)
   {
      // !bwc! Port-sensitive query.
      return (mDomains.count(copy + ":" + Data(port)) != 0);
   }
   else
   {
      // !bwc! Port-insensitive query.
      return (mDomains.count(copy)!=0);
   }
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

   SipMessage* toSend = static_cast<SipMessage*>(msg.clone());
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
   
   SipMessage* toSend = static_cast<SipMessage*>(msg.clone());
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
SipStack::post( std::auto_ptr<ApplicationMessage> message,
                  TransactionUser* tu)
{
   assert(!mShuttingDown);
   if (tu) message->setTransactionUser(tu);
   mTuSelector.add(message.release());
}


void
SipStack::post(const ApplicationMessage& message,
               TransactionUser* tu)
{
   assert(!mShuttingDown);
   Message* toPost = message.clone();
   if (tu) toPost->setTransactionUser(tu);
   //mTUFifo.add(toPost);
   mTuSelector.add(toPost);
}

void
SipStack::post(const ApplicationMessage& message,  unsigned int secondsLater,
               TransactionUser* tu)
{
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
   mNextTimer=mAppTimers.add(ms,toPost);
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
   mNextTimer=mAppTimers.add(ms, message.release());
   //.dcm. timer update rather than process cycle...optimize by checking if sooner
   //than current timeTillNextProcess?
   checkAsyncProcessHandler();
}

void 
SipStack::abandonServerTransaction(const Data& tid)
{
   mTransactionController.abandonServerTransaction(tid);
   checkAsyncProcessHandler();
}

void 
SipStack::cancelClientInviteTransaction(const Data& tid)
{
   mTransactionController.cancelClientInviteTransaction(tid);
   checkAsyncProcessHandler();
}

void 
SipStack::closeConnection(const Tuple& peer)
{
   mTransactionController.closeConnection(peer);
}

bool
SipStack::hasMessage() const
{
   return mTUFifo.messageAvailable();
}

SipMessage* 
SipStack::receive()
{
   if (mTUBuffer.empty())
   {
      mTUFifo.swapOut(mTUBuffer);
   }

   // Check to see if a message is available and if it is return the 
   // waiting message. Otherwise, return 0
   if (!mTUBuffer.empty())
   {
      // we should only ever have SIP messages on the TU Fifo
      // unless we've registered for termination messages. 
      Message* msg = mTUBuffer.front();
      mTUBuffer.pop_front();
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

SipMessage* 
SipStack::receive(int waitMs)
{
   int startMs = Timer::getTimeMs();
   int nowMs = startMs;
   Message* msg = 0;

   while(nowMs < startMs + waitMs)
   {
      // waitMs - (nowMs-startMs) > 0
      msg = mTUFifo.getNext(waitMs - (nowMs-startMs));

      if(msg)
      {
         SipMessage* sip = dynamic_cast<SipMessage*>(msg);
         if (sip)
         {
            DebugLog (<< "RECV: " << sip->brief());
            return sip;
         }
         else
         {
            delete msg;
         }
      }
      nowMs = Timer::getTimeMs();
   }

   // ran out of time
   return 0;
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
SipStack::process(FdSet& fdset)
{
   if(!mShuttingDown && mStatisticsManagerEnabled)
   {
      mStatsManager.process();
   }

   {
      if(!mTransactionControllerThread)
      {
         mTransactionController.process(fdset);
      }
      mTuSelector.process();
      if(!mDnsThread)
      {
         mDnsStub->process(fdset);
      }
   }

   if(mNextTimer)
   {
      UInt64 now(Timer::getTimeMs());
      if(now >= mNextTimer)
      {
         Lock lock(mAppTimerMutex); 
         mNextTimer = mAppTimers.process();
      }
   }
}

/// returns time in milliseconds when process next needs to be called 
unsigned int 
SipStack::getTimeTillNextProcessMS()
{
   UInt64 now(Timer::getTimeMs());
   unsigned int msTillNextTimer(now > mNextTimer ? 0 : mNextTimer - now);

   unsigned int dnsNextProcess = (mDnsThread ? 
                           INT_MAX : mDnsStub->getTimeTillNextProcessMS());
   unsigned int tcNextProcess = mTransactionControllerThread ? INT_MAX : 
                           mTransactionController.getTimeTillNextProcessMS();
   return resipMin(Timer::getMaxSystemTimeWaitMs(),
                     resipMin(resipMin(dnsNextProcess,
                              tcNextProcess),
                              resipMin(mTuSelector.getTimeTillNextProcessMS(), 
                              msTillNextTimer)));
} 

void
SipStack::buildFdSet(FdSet& fdset)
{
   if(!mTransactionControllerThread)
   {
      mTransactionController.buildFdSet(fdset);
   }
   if(!mDnsThread)
   {
      mDnsStub->buildFdSet(fdset);
   }
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

/* Copyright 2007 Estacado Systems */

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
