#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/SipMessage.hxx"
#include "resiprocate/ApplicationMessage.hxx"
#include "resiprocate/ShutdownMessage.hxx"
#include "resiprocate/TransactionController.hxx"
#include "resiprocate/TransactionState.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/NotifierFifo.hxx"
#include "resiprocate/StatisticsManager.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION

#if defined(WIN32)
#pragma warning( disable : 4355 ) // using this in base member initializer list 
#endif
TransactionController::TransactionController(bool multi, Fifo<Message>& tufifo, 
                                             bool stateless, ProcessNotifier::Handler* asyncHandler,
                                             ExternalDns* dns) : 
   mStateless(stateless),
   mRegisteredForTransactionTermination(false),
   mDiscardStrayResponses(true),
   mTUFifo(tufifo),
   mStatelessHandler(*this),
   mStateMacFifo( asyncHandler ? new NotifierFifo<TransactionMessage>(asyncHandler):  new Fifo<TransactionMessage> ),
   mTransportSelector(multi, *mStateMacFifo, dns),
   mTimers(*mStateMacFifo, mTUFifo),
   StatelessIdCounter(1),
   mShuttingDown(false)
{
   RESIP_STATISTICS(mStatsManager = new StatisticsManager(tufifo));
}

TransactionController::TransactionController(bool multi, Fifo<Message>& tufifo, 
                                             ExternalSelector* tSelector, 
                                             bool stateless, ProcessNotifier::Handler* asyncHandler,
                                             ExternalDns* dns) :
   mStateless(stateless),
   mRegisteredForTransactionTermination(false),
   mDiscardStrayResponses(true),
   mTUFifo(tufifo),
   mStatelessHandler(*this),
   mStateMacFifo( asyncHandler ? new NotifierFifo<TransactionMessage>(asyncHandler) : new Fifo<TransactionMessage> ),
   mTransportSelector(multi, *mStateMacFifo, dns, tSelector),
   mTimers(*mStateMacFifo, mTUFifo),
   StatelessIdCounter(1),
   mShuttingDown(false),
   mStatsManager(0)
{
   RESIP_STATISTICS(mStatsManager = new StatisticsManager(tufifo));
 }



#if defined(WIN32)
#pragma warning( default : 4355 )
#endif

TransactionController::~TransactionController()
{
   delete mStateMacFifo;
   delete mStatsManager;
}

void
TransactionController::shutdown()
{
   mShuttingDown = true;
   mTransportSelector.shutdown();
}

void
TransactionController::process(FdSet& fdset)
{
   if (mShuttingDown && 
       //mTimers.empty() && 
       !mStateMacFifo->messageAvailable() && 
       !mTUFifo.messageAvailable() &&
       mTransportSelector.isFinished())
   {
      mTUFifo.add(new ShutdownMessage);
   }
   else
   {
      mTransportSelector.process(fdset);
      mTimers.process();

      while (mStateMacFifo->messageAvailable())
      {
         if (mStateless)
         {
            mStatelessHandler.process();
         }
         else
         {
            TransactionState::process(*this);
         }
      }
   }
}

unsigned int 
TransactionController::getTimeTillNextProcessMS()
{
   if ( mStateMacFifo->messageAvailable() ) 
   {
      return 0;
   }
   else if ( mTransportSelector.hasDataToSend() )
   {
      return 0;
   }
   
   int ret = mTimers.msTillNextTimer();

#if 1 // !cj! just keep a max of 500ms for good luck - should not be needed   
   if ( ret > 1 )
   {
      ret = 500;
   }
#endif

   return ret;
} 
   
void 
TransactionController::buildFdSet( FdSet& fdset)
{
   mTransportSelector.buildFdSet( fdset );
}

// void
// TransactionController::addTransport( TransportType protocol, 
//                                      int port,
//                                      IpVersion version,
//                                      const Data& ipInterface)
// {
//    mTransportSelector.addTransport(protocol, port, version, ipInterface);
// }

// void 
// TransactionController::addTlsTransport( int port, 
//                                         const Data& keyDir,
//                                         const Data& privateKeyPassPhrase,
//                                         const Data& domainname,
//                                         IpVersion version,
//                                         const Data& ipInterface)
// {
//    mTransportSelector.addTlsTransport(domainname, keyDir, privateKeyPassPhrase, port, version, ipInterface);
// }

void
TransactionController::send(SipMessage* msg)
{
   mStateMacFifo->add(msg);
}

void
TransactionController::post(const ApplicationMessage& message)
{
   assert(!mShuttingDown);
   Message* toPost = message.clone();
   mTUFifo.add(toPost);
}

void
TransactionController::post(const ApplicationMessage& message,
                            unsigned int ms)
{
   assert(!mShuttingDown);
   Message* toPost = message.clone();
   mTimers.add(Timer(ms, toPost));
}

void
TransactionController::registerForTransactionTermination()
{
   mRegisteredForTransactionTermination = true;
}

StatisticsManager&
TransactionController::getStatisticsManager() const
{
   assert(mStatsManager);
   return *mStatsManager;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2004 Vovida Networks, Inc.  All rights reserved.
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
