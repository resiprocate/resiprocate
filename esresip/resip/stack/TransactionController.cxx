/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/AbandonServerTransaction.hxx"
#include "resip/stack/ApplicationMessage.hxx"
#include "resip/stack/CancelClientInviteTransaction.hxx"
#include "resip/stack/ConnectionTerminated.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/ShutdownMessage.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/TransactionController.hxx"
#include "resip/stack/TransactionState.hxx"
#include "resip/stack/ssl/Security.hxx"
#include "rutil/CongestionManager.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "resip/stack/SipStack.hxx"

#include <memory>

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION

#if defined(WIN32) && !defined (__GNUC__)
#pragma warning( disable : 4355 ) // using this in base member initializer list 
#endif

unsigned int TransactionController::MaxTUFifoSize = 0;
unsigned int TransactionController::MaxTUFifoTimeDepthSecs = 0;

TransactionController::TransactionController(SipStack& stack) :
   mStack(stack),
   mDiscardStrayResponses(true),
   mFixBadDialogIdentifiers(true),
   mFixBadCSeqNumbers(true),
   mThreaded(true),
   mStateMacFifo(),
   mTuSelector(stack.mTuSelector),
   mTransportSelector(mStateMacFifo,
                      stack.getSecurity(),
                      stack.getDnsStub(),
                      stack.getCompression()),
   mTimers(mTimerFifo),
   mShuttingDown(false),
   mStatsManager(stack.mStatsManager),
   mHostname(DnsUtil::getLocalHostName())
{
   mStateMacFifo.setDescription("TransactionController::mStateMacFifo");
}

#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( default : 4355 )
#endif

TransactionController::~TransactionController()
{
   if(mClientTransactionMap.size())
   {
      WarningLog(<< "On shutdown, there are Client TransactionStates remaining!");
   }

   if(mServerTransactionMap.size())
   {
      WarningLog(<< "On shutdown, there are Server TransactionStates remaining!");
   }
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
       !mStateMacFifo.messageAvailable() && // !dcm! -- see below 
       !mStack.mTUFifo.messageAvailable() &&
       mTransportSelector.isFinished())
// !dcm! -- why would one wait for the Tu's fifo to be empty before delivering a
// shutdown message?
   {
      //!dcm! -- send to all?
      mTuSelector.add(new ShutdownMessage);
   }
   else
   {
      mTransportSelector.process(fdset);
      mTimers.process();
      assert(mTimerServiceQueue.empty());
      mTimerFifo.swapOut(mTimerServiceQueue);

      while (!mTimerServiceQueue.empty())
      {
         TransactionState::processTimer(*this,mTimerServiceQueue.front());
         mTimerServiceQueue.pop_front();
      }

      if(mStateMacServiceQueue.empty())
      {
         mStateMacFifo.swapOut(mStateMacServiceQueue);
      }

      int runs=resipMin((size_t)16, mStateMacServiceQueue.size());
      bool pokeTransportSelector=(runs!=0);

      while (runs > 0)
      {
         TransactionState::process(*this, mStateMacServiceQueue.front());
         mStateMacServiceQueue.pop_front();
         --runs;
      }

      if(pokeTransportSelector)
      {
         mTransportSelector.poke();
      }
   }
}

void 
TransactionController::process()
{
   if (mShuttingDown && 
       //mTimers.empty() && 
       !mStateMacFifo.messageAvailable() && // !dcm! -- see below 
       !mStack.mTUFifo.messageAvailable() &&
       mTransportSelector.isFinished())
// !dcm! -- why would one wait for the Tu's fifo to be empty before delivering a
// shutdown message?
   {
      //!dcm! -- send to all?
      mTuSelector.add(new ShutdownMessage);
   }
   else
   {
      unsigned int timeout=mTimers.msTillNextTimer();

      if(mStateMacServiceQueue.empty())
      {
         if(timeout)
         {
            mStateMacFifo.swapOut(resipMin(timeout,25U), mStateMacServiceQueue);
         }
         else
         {
            mStateMacFifo.swapOut(mStateMacServiceQueue);
         }
      }

      if(!timeout)
      {
         mTimers.process();
         assert(mTimerServiceQueue.empty());
         mTimerFifo.swapOut(mTimerServiceQueue);

         while (!mTimerServiceQueue.empty())
         {
            TransactionState::processTimer(*this,mTimerServiceQueue.front());
            mTimerServiceQueue.pop_front();
         }
      }

      int runs=resipMin((size_t)16, mStateMacServiceQueue.size());
      bool pokeTransportSelector=(runs!=0);

      while (runs > 0)
      {
         TransactionState::process(*this, mStateMacServiceQueue.front());
         mStateMacServiceQueue.pop_front();
         --runs;
      }

      if(pokeTransportSelector)
      {
         mTransportSelector.poke();
      }
   }
}


unsigned int 
TransactionController::getTimeTillNextProcessMS()
{
   if ( mStateMacFifo.messageAvailable() ) 
   {
      return 0;
   }
   else if ( mTransportSelector.hasDataToSend() )
   {
      return 0;
   }

   return resipMin(mTimers.msTillNextTimer(), mTransportSelector.getTimeTillNextProcessMS());   
} 
   
void 
TransactionController::buildFdSet( FdSet& fdset)
{
   mTransportSelector.buildFdSet( fdset );
}

void
TransactionController::send(SipMessage* msg)
{
  // Under no circumstances do we drop a response from the TU
   if(msg->isRequest())
   {
      CongestionManager::RejectionBehavior behavior=mStateMacFifo.getRejectionBehavior();
      if(msg->method()==ACK && 
         behavior==CongestionManager::REJECTING_NON_ESSENTIAL)
      {
         // ?bwc? Is an ACK non-essential?
         delete msg;
         return;
      }
      else if(behavior==CongestionManager::REJECTING_NEW_WORK || 
               behavior==CongestionManager::REJECTING_NON_ESSENTIAL)
      {
         // Would this be quicker if we converted the request to a response 
         // in-place?
         std::auto_ptr<SipMessage> response(Helper::makeResponse(*msg,503));
         response->setTransactionUser(msg->getTransactionUser());
         delete msg;
         response->header(h_RetryAfter).value()=
               mStateMacFifo.expectedWaitTimeMilliSec()/1000;
         mTuSelector.add(response.release());
         return;
      }
   }
   mStateMacFifo.add(msg);
}


unsigned int 
TransactionController::getTuFifoSize() const
{
   return mTuSelector.size();
}

unsigned int 
TransactionController::sumTransportFifoSizes() const
{
   return mTransportSelector.sumTransportFifoSizes();
}

unsigned int 
TransactionController::getTransactionFifoSize() const
{
   return mStateMacFifo.size();
}

unsigned int 
TransactionController::getNumClientTransactions() const
{
   return mClientTransactionMap.size();
}

unsigned int 
TransactionController::getNumServerTransactions() const
{
   return mServerTransactionMap.size();
}

unsigned int 
TransactionController::getTimerQueueSize() const
{
   return mTimers.size();
}

void
TransactionController::registerMarkListener(MarkListener* listener)
{
   mTransportSelector.registerMarkListener(listener);
}

void TransactionController::unregisterMarkListener(MarkListener* listener)
{
   mTransportSelector.unregisterMarkListener(listener);
}

void 
TransactionController::closeConnection(const Tuple& peer)
{
   mStateMacFifo.add(new ConnectionTerminated(peer));
}

void 
TransactionController::abandonServerTransaction(const Data& tid)
{
   mStateMacFifo.add(new AbandonServerTransaction(tid));
}

void 
TransactionController::cancelClientInviteTransaction(const Data& tid)
{
   mStateMacFifo.add(new CancelClientInviteTransaction(tid));
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
