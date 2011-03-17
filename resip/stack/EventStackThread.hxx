#ifndef RESIP_EventStackThread__hxx
#define RESIP_EventStackThread__hxx

#include <vector>

#include "rutil/ThreadIf.hxx"
#include "rutil/FdPoll.hxx"
#include "resip/stack/SelectInterruptor.hxx"

namespace resip
{

class SipStack;
class SelectInterruptor;
class EventThreadInterruptor;
class FdPollGrp;

/**
    This class creates a thread in which to run one or more SipStacks.  The
    thread provides cycles to the stack(s). It provides cycles in 3 ways:
    1. When socket-io is possible, the event-loop will directly invoke
       the registered callback.
    2. Prior to waiting for events, getTimeTillNextProcessMS() is called
       on all stacks.
    3. After waiting, processTimers() is called on all stacks.

    This implementation improves on StackThread and IntrruptableStackThread,
    by using the epoll() based system call (if available) provided
    by the FdPoll class.

    Note that this is different than the InterruptableStackThread and
    simple StackThread in that it doesn't use the buildFdSet()/process()
    flow.

    You must register {si} as an AsyncProcessHandler on the
    stacks in order to use this class. The same {si} instance must be
    used for all stacks.
**/
class EventStackThread : public ThreadIf
{
   public:
      EventStackThread(EventThreadInterruptor& si, FdPollGrp& pollGrp);
      EventStackThread(SipStack& stack, EventThreadInterruptor& si, FdPollGrp& pollGrp);
      virtual ~EventStackThread();

      void addStack(SipStack& stack);
      virtual void thread();
      virtual void shutdown();

   protected:
      /*
       * Return time (in milliseconds) until your next timer, or
       * ~30sec for infinity.
       */
      virtual unsigned int getTimeTillNextProcessMS() const;

      /*
       * Called after all socket IO and sip stack timers. Process
       * any application timers here.
       */
      virtual void afterProcess();

   private:
      typedef std::vector<SipStack*> StackList;
      StackList mStacks;
      EventThreadInterruptor& mIntr;
      FdPollGrp& mPollGrp;
};



class EventThreadInterruptor : public SelectInterruptor, public FdPollItemIf
{
   public:
      EventThreadInterruptor(FdPollGrp& pollGrp);
      virtual ~EventThreadInterruptor();
      /*
       * Interface for FdPollItemIf
       */
      virtual void processPollEvent(FdPollEventMask mask) { processCleanup(); }
   protected:
      FdPollGrp& mPollGrp;      // used just to remove ourselves
      FdPollItemHandle mPollItemHandle;
};

}

#endif


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
 *  vi: set shiftwidth=3 expandtab:
 */
