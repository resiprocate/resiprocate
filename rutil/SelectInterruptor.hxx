#ifndef RESIP_SelectInterruptor_HXX
#define RESIP_SelectInterruptor_HXX

#include "rutil/AsyncProcessHandler.hxx"
#include "rutil/FdPoll.hxx"
#include "rutil/Socket.hxx"

namespace resip
{

/**
    Used to 'artificially' interrupt a select call
*/
class SelectInterruptor : public AsyncProcessHandler, public FdPollItemIf
{
   public:
      SelectInterruptor();
      virtual ~SelectInterruptor();

      /**
          Called by the stack when messages are posted to it.
          Calls interrupt.
      */
      virtual void handleProcessNotification();

      /**
          cause the 'artificial' fd to signal
      */
      void interrupt();

      /**
          Used to add the 'artificial' fd to the fdset that
          will be responsible for interrupting a subsequent select
          call.
      */
      void buildFdSet(FdSet& fdset);

      /**
          cleanup signalled fd
      */
      void process(FdSet& fdset);

      virtual void processPollEvent(FdPollEventMask mask);

      /* Get fd of read-side, for use within PollInterruptor,
       * Declared as Socket for easier cross-platform even though pipe fd
       * under linux.
       */
      Socket getReadSocket() const { return mReadThing; }

   protected:

      /* Cleanup the read side of the interruptor
       * If fdset is provided, it will only try cleaning up if our pipe
       * is ready in fdset. If NULL, it will unconditionally try reading.
       * This last feature is for use within PollInterruptor.
       */
      void processCleanup();
   private:
#ifndef WIN32
      int mPipe[2];
#else
      Socket mSocket;
      sockaddr mWakeupAddr;
#endif
      // either mPipe[0] or mSocket
      Socket mReadThing;
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
 * vi: set shiftwidth=3 expandtab:
 */
