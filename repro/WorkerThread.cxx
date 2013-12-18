#include "repro/WorkerThread.hxx"

#include "resip/stack/SipStack.hxx"
#include "resip/stack/ApplicationMessage.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

namespace repro
{

WorkerThread::WorkerThread(Worker* worker,
                        resip::TimeLimitFifo<resip::ApplicationMessage>& fifo,
                        resip::SipStack* stack):
   mWorker(worker),
   mFifo(fifo),
   mStack(stack)
{}

WorkerThread::~WorkerThread()
{
   shutdown();
   join();
   delete mWorker;
}

void
WorkerThread::thread()
{
   resip::ApplicationMessage* msg;
   bool queueToStack;
   if(mWorker && !isShutdown())
   {
      mWorker->onStart();
      while(mWorker && !isShutdown())
      {
         if( (msg=mFifo.getNext(100)) != 0 )
         {
            queueToStack = mWorker->process(msg);

            if(queueToStack && mStack)
            {
               StackLog(<<"async work done, posting to stack");
               // Post to stack instead of directly to TU, since stack does
               // some safety checks to ensure the TU still exists before posting
               mStack->post(std::auto_ptr<resip::ApplicationMessage>(msg));
            }
            else
            {
               StackLog(<<"discarding a message");
               if(!mStack)
               {
                  WarningLog(<<"mStack == 0");
               }
               delete msg;
            }
         }
      }
   }
}

}//namespace repro

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
