#include <climits>

#include "resip/stack/EventStackThread.hxx"
#include "resip/stack/SipStack.hxx"
//#include "resip/stack/SipMessage.hxx"
//#include "rutil/SelectInterruptor.hxx"
//#include "resip/stack/FdPoll.hxx"

#include "rutil/Logger.hxx"
#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

using namespace resip;

/****************************************************************
 *
 * EventThreadInterruptor
 *
 ****************************************************************/

EventThreadInterruptor::EventThreadInterruptor(FdPollGrp& pollGrp)
  : mPollGrp(pollGrp)
{
   mPollItemHandle = mPollGrp.addPollItem(getReadSocket(), FPEM_Read, this);
}

EventThreadInterruptor::~EventThreadInterruptor()
{
   mPollGrp.delPollItem(mPollItemHandle);
}

/****************************************************************
 *
 * EventStackThread
 *
 ****************************************************************/

EventStackThread::EventStackThread(EventThreadInterruptor& si,
      FdPollGrp& pollGrp)
   : mIntr(si), mPollGrp(pollGrp)
{
}

EventStackThread::EventStackThread(SipStack& stack, EventThreadInterruptor& si,
      FdPollGrp& pollGrp)
   : mIntr(si), mPollGrp(pollGrp)
{
    addStack(stack);
}


EventStackThread::~EventStackThread()
{
   //InfoLog (<< "EventStackThread::~EventStackThread()");
}

void
EventStackThread::addStack(SipStack& stack)
{
    mStacks.push_back(&stack);
}

void
EventStackThread::thread()
{
   while (!isShutdown())
   {
      unsigned waitMs = getTimeTillNextProcessMS();
      if ( waitMs > INT_MAX )
         waitMs = INT_MAX;
      StackList::iterator it;
      for ( it=mStacks.begin(); it!=mStacks.end(); ++it)
      {
         SipStack *ss = *it;
         unsigned wms = ss->getTimeTillNextProcessMS();
         if ( wms < waitMs )
            waitMs = wms;
         // NOTE: In theory, we could early-out when waitMs gets to zero
         // but I fear the stack may depend upon doing real-work in the query.
      }
      mPollGrp.waitAndProcess((int)waitMs);
      for ( it=mStacks.begin(); it!=mStacks.end(); ++it)
      {
         SipStack *ss = *it;
         ss->processTimers();
      }
      afterProcess();
   }
   InfoLog (<< "Shutting down stack thread");
}

void
EventStackThread::shutdown()
{
   ThreadIf::shutdown();
   mIntr.interrupt();
}

unsigned int
EventStackThread::getTimeTillNextProcessMS() const
{
   return 10000;
}

void
EventStackThread::afterProcess()
{
}


/****************************************************************
 *
 * EventStackSimpleMgr
 *
 * This is a helper class that constructs the thread-related
 * classes in the appropriate order, and destructs them when done.
 * It is to help save typing in simple applications.
 *
 ****************************************************************/

EventStackSimpleMgr::EventStackSimpleMgr(const char *implName)
   : mPollGrp(0), mIntr(0), mThread(0), mStack(0)
{
   mPollGrp = FdPollGrp::create(implName);
   mIntr = new EventThreadInterruptor(*mPollGrp);
   mThread = new EventStackThread(*mIntr, *mPollGrp);
}

EventStackSimpleMgr::~EventStackSimpleMgr()
{
   release();
}

void
EventStackSimpleMgr::setOptions(SipStackOptions& options)
{
   options.mPollGrp = mPollGrp;
   options.mAsyncProcessHandler = mIntr;
}


SipStack&
EventStackSimpleMgr::createStack(SipStackOptions& options)
{
   setOptions(options);
   mStack = new SipStack(options);
   mThread->addStack(*mStack);
   return *mStack;
}

void
EventStackSimpleMgr::release() 
{
   if ( mThread )
   {
      delete mThread; mThread = NULL;
   }
   if ( mStack ) 
   {
      // we only delete the stack if we created, not if externally created
      delete mStack; mStack = NULL;
   }
   if ( mIntr )
   {
      delete mIntr; mIntr = NULL;
   }
   if ( mPollGrp )
   {
      delete mPollGrp; mPollGrp = NULL;
   }
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
