
#include "PyRouteProcessor.hxx"
#include "PyRouteWorker.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;

PyRouteProcessor::PyRouteProcessor(Dispatcher& dispatcher) :
   Processor("PyRoute"),
   mDispatcher(dispatcher)
{
}

PyRouteProcessor::~PyRouteProcessor()
{
}

Processor::processor_action_t
PyRouteProcessor::process(RequestContext &context)
{
   DebugLog(<< "Monkey handling request: PyRoute");

   // Has the work been done already?
   PyRouteWork* work = dynamic_cast<PyRouteWork*>(context.getCurrentEvent());
   if(work)
   {
      if(work->hasResponse())
      {
         resip::SipMessage response;
         if(work->mResponseMessage.size() == 0)
         {
            Helper::makeResponse(response, context.getOriginalRequest(), work->mResponseCode);
         }
         else
         {
            Helper::makeResponse(response, context.getOriginalRequest(), work->mResponseCode, work->mResponseMessage);
         }
         context.sendResponse(response);
         return Processor::SkipThisChain;
      }
      for(
         std::vector<Data>::iterator i = work->mTargets.begin();
         i != work->mTargets.end();
         i++)
      {
         context.getResponseContext().addTarget(NameAddr(*i));
      }
      if(work->mTargets.size() > 0)
      {
         return Processor::SkipThisChain;
      }
      return Processor::Continue;
   }

   SipMessage& msg = context.getOriginalRequest();
   if(msg.method() != INVITE && msg.method() != MESSAGE)
   {
      // We only route INVITE and MESSAGE, otherwise we ignore
      return Processor::Continue;
   }
   work = new PyRouteWork(*this, context.getTransactionId(), &(context.getProxy()), msg);
   std::auto_ptr<ApplicationMessage> app(work);
   mDispatcher.post(app);

   return Processor::WaitingForEvent;
}

/* ====================================================================
 *
 * Copyright 2014 Daniel Pocock http://danielpocock.com  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

