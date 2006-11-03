#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#ifndef WIN32
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Fifo.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Random.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Timer.hxx"

#include "resiprocate/Message.hxx"
#include "resiprocate/Security.hxx"
#include "resiprocate/ShutdownMessage.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/ApplicationMessage.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/os/Inserter.hxx"
#include "resiprocate/StatisticsManager.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"

#ifdef WIN32
#pragma warning( disable : 4355 )
#endif

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

SipStack::SipStack(bool multiThreaded, Security* pSecurity, bool stateless) : 
#ifdef USE_SSL
   mSecurity( pSecurity ? pSecurity : new Security("~/")),
#else
   mSecurity(0),
#endif
   mTUFifo(TransactionController::MaxTUFifoTimeDepthSecs,
           TransactionController::MaxTUFifoSize),
   mAppTimers(mTuSelector),
   mStatsManager(*this),
   mTransactionController(multiThreaded, mTUFifo, mStatsManager, mSecurity, stateless),
   mStrictRouting(false),
   mShuttingDown(false),
   mTuSelector(mTUFifo)
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
#ifdef USE_SSL
   delete mSecurity;
#endif
}

void
SipStack::shutdown()
{
   InfoLog (<< "Shutting down stack " << this);

   static Mutex shutDownMutex;
   {
      Lock lock(shutDownMutex);
      assert(!mShuttingDown);
      mShuttingDown = true;
   }

   mTransactionController.shutdown();
}

bool
SipStack::addTransport( TransportType protocol,
                        int port, 
                        IpVersion version,
                        const Data& ipInterface, 
                        const Data& sipDomainname,
                        const Data& privateKeyPassPhrase,
                        SecurityTypes::SSLType sslType)
{
   assert(!mShuttingDown);
   
   bool ret = mTransactionController.addTransport( protocol, port, version, ipInterface, 
                                                   sipDomainname, privateKeyPassPhrase, sslType);
   if (ret && !ipInterface.empty()) 
   {
      addAlias(ipInterface, port);
   }
   return ret;
}


void
SipStack::addAlias(const Data& domain, int port)
{
   DebugLog (<< "Adding domain alias: " << domain << ":" << port);
   assert(!mShuttingDown);
   mDomains.insert(domain + ":" + Data(port));
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
SipStack::send(std::auto_ptr<SipMessage> msg, 
               TransactionUser* tu)
{
   DebugLog (<< "SEND: " << msg->brief());
   if (tu) 
   {
      msg->setTransactionUser(tu);
   } 
   msg->setFromTU();

   mTransactionController.send(msg.release());
}

void 
SipStack::send(const SipMessage& msg, 
               TransactionUser* tu)
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
}


// this is only if you want to send to a destination not in the route. You
// probably don't want to use it. 
void 
SipStack::sendTo(std::auto_ptr<SipMessage> msg, 
                 const Uri& uri, 
               TransactionUser* tu)
{
   msg->setForceTarget(uri);
   if (tu) 
   {
      msg->setTransactionUser(tu);
   } 
   msg->setFromTU();

   mTransactionController.send(msg.release());
}

// this is only if you want to send to a destination not in the route. You
// probably don't want to use it. 
void 
SipStack::sendTo(std::auto_ptr<SipMessage> msg, 
                 const Tuple& destination, 
               TransactionUser* tu)
{
   assert(!mShuttingDown);
   assert(destination.transport);
   if (tu) 
   {
      msg->setTransactionUser(tu);
   } 
   msg->setDestination(destination);
   msg->setFromTU();

   mTransactionController.send(msg.release());
}

// this is only if you want to send to a destination not in the route. You
// probably don't want to use it. 
void 
SipStack::sendTo(const SipMessage& msg, const Uri& uri, 
               TransactionUser* tu)
{
   //assert(!mShuttingDown);

   SipMessage* toSend = new SipMessage(msg);
   if (tu) 
   {
      toSend->setTransactionUser(tu);
   } 
   toSend->setForceTarget(uri);
   toSend->setFromTU();

   mTransactionController.send(toSend);
}

// this is only if you want to send to a destination not in the route. You
// probably don't want to use it. 
void 
SipStack::sendTo(const SipMessage& msg, const Tuple& destination, 
               TransactionUser* tu)
{
   assert(!mShuttingDown);
   assert(destination.transport);
   
   SipMessage* toSend = new SipMessage(msg); 
   if (tu) 
   {
      toSend->setTransactionUser(tu);
   } 
   toSend->setDestination(destination);
   toSend->setFromTU();

   mTransactionController.send(toSend);
}

void
SipStack::post(std::auto_ptr<ApplicationMessage> message)
{
   assert(!mShuttingDown);
   mTUFifo.add(message.release(), TimeLimitFifo<Message>::InternalElement);
}

void
SipStack::post(std::auto_ptr<ApplicationMessage> message,
               unsigned int secondsLater, 
               TransactionUser* tu)
{
   assert(!mShuttingDown);
   postMS(message, secondsLater*1000);
}

void
SipStack::postMS(std::auto_ptr<ApplicationMessage> message, 
                 unsigned int ms, 
		 TransactionUser* tu)
{
   assert(!mShuttingDown);
   if (tu) 
   {
      message->setTransactionUser(tu);
   }
   Lock lock(mAppTimerMutex);
   mAppTimers.add(Timer(ms, message.release()));
}

void
SipStack::post(const ApplicationMessage& message)
{
   assert(!mShuttingDown);
   Message* toPost = message.clone();
   mTUFifo.add(toPost, TimeLimitFifo<Message>::InternalElement);
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
   if (tu)
   {
      toPost->setTransactionUser(tu);
   }
   Lock lock(mAppTimerMutex);
   mAppTimers.add(Timer(ms, toPost));
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

Message*
SipStack::receiveAny(int ms)
{
   return mTUFifo.getNext(ms);
}

void 
SipStack::process(FdSet& fdset)
{
   if(!mShuttingDown)
   {
      RESIP_STATISTICS(mStatsManager.process());
   }
   mTransactionController.process(fdset);
   mTuSelector.process();

   Lock lock(mAppTimerMutex);
   mAppTimers.process();
}

/// returns time in milliseconds when process next needs to be called 
unsigned int 
SipStack::getTimeTillNextProcessMS()
{
   Lock lock(mAppTimerMutex);
   return resipMin(mTransactionController.getTimeTillNextProcessMS(),
                   mAppTimers.msTillNextTimer());
} 

void
SipStack::registerForTransactionTermination()
{
   mTransactionController.registerForTransactionTermination();
}

void 
SipStack::buildFdSet(FdSet& fdset)
{
   mTransactionController.buildFdSet(fdset);
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
}

void 
SipStack::unregisterTransactionUser(TransactionUser& tu)
{
   mTuSelector.unregisterTransactionUser(tu);
}

std::ostream& 
SipStack::dump(std::ostream& strm)  const
{
   Lock lock(mAppTimerMutex);
   strm << "SipStack: " << (this->mStrictRouting ? "strict router " : "loose router ")
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
