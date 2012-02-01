#include "resip/stack/InterruptableStackThread.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/SelectInterruptor.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

using namespace resip;

InterruptableStackThread::InterruptableStackThread(SipStack& stack, SelectInterruptor& si)
   : mStack(stack),
     mSelectInterruptor(si)
{}

InterruptableStackThread::~InterruptableStackThread()
{
   //InfoLog (<< "InterruptableStackThread::~InterruptableStackThread()");
}

void
InterruptableStackThread::thread()
{
   while (!isShutdown())
   {
      try
      {
         FdSet fdset;
         mStack.process(fdset); // .dcm. reqd to get send requests queued at transports
         mSelectInterruptor.buildFdSet(fdset);
         mStack.buildFdSet(fdset);
         buildFdSet(fdset);
         int ret = fdset.selectMilliSeconds(resipMin(mStack.getTimeTillNextProcessMS(), 
                                                     getTimeTillNextProcessMS()));
         if (ret >= 0)
         {
            // .dlb. use return value to peak at the message to see if it is a
            // shutdown, and call shutdown if it is
            // .dcm. how will this interact w/ TuSelector?
            mSelectInterruptor.process(fdset);
            mStack.process(fdset);
            process(fdset);
         }
      }
      catch (BaseException& e)
      {
         ErrLog (<< "Unhandled exception: " << e);
      }
   }
   InfoLog (<< "Shutting down stack thread");
}

void
InterruptableStackThread::shutdown()
{
   ThreadIf::shutdown();
   mSelectInterruptor.interrupt();
}

void
InterruptableStackThread::buildFdSet(FdSet& fdset)
{
}

unsigned int
InterruptableStackThread::getTimeTillNextProcessMS() const
{
   //.dcm. --- eventually make infinite
   return 10000;   
}

void 
InterruptableStackThread::process(FdSet& fdset)
{
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
