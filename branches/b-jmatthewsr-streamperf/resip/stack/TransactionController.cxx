#include "precompile.h"
#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/ApplicationMessage.hxx"
#include "resip/stack/ShutdownMessage.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/TransactionController.hxx"
#include "resip/stack/TransactionState.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "resip/stack/SipStack.hxx"

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
   mStateMacFifo(),
   mTuSelector(stack.mTuSelector),
   mTransportSelector(mStateMacFifo,
                      stack.getSecurity(),
                      stack.getDnsStub(),
                      stack.getCompression()),
   mTimers(mStateMacFifo),
   mShuttingDown(false),
   mStatsManager(stack.mStatsManager)
{
}

#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( default : 4355 )
#endif

TransactionController::~TransactionController()
{
}


bool 
TransactionController::isTUOverloaded() const
{
   return !mTuSelector.wouldAccept(TimeLimitFifo<Message>::EnforceTimeDepth);
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
      mTuSelector.add(new ShutdownMessage, TimeLimitFifo<Message>::InternalElement);
   }
   else
   {
      mTransportSelector.process(fdset);
      mTimers.process();

       //while (mStateMacFifo.messageAvailable())
	  unsigned fifosize = mStateMacFifo.size();
	  for(unsigned ui=0; ui< fifosize; ui++ )
	  /*ivr mod for(int i=0; i<2 && mStateMacFifo.messageAvailable(); i++)*/
      {
         TransactionState::process(*this);
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
TransactionController::registerBlacklistListener(int rrType, DnsStub::BlacklistListener* l)
{
   mTransportSelector.registerBlacklistListener(rrType, l);
}

void TransactionController::unregisterBlacklistListener(int rrType, DnsStub::BlacklistListener* l)
{
   mTransportSelector.unregisterBlacklistListener(rrType, l);
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
