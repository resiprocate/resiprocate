#if !defined(RESIP_ASYNC_PROCESSOR_HXX)
#define RESIP_ASYNC_PROCESSOR_HXX 

#include <iosfwd>
#include <vector>
#include "repro/Processor.hxx"
#include "repro/ProcessorMessage.hxx"
#include "repro/Dispatcher.hxx"

namespace repro
{
class AsyncProcessorMessage;

// This class is used to create Processors that need to accomplish blocking
// tasks, such as database access.  The request/event is delivered to this 
// Processor, like any other via the virtual method:
// virtual processor_action_t process(RequestContext &)
//
// When a blocking task is required, it can be queued up for the thread pool 
// of workers, that are implemented in the constructor provided asyncDispatcher, by
// constructing a new AsyncProcessorMessage and calling mAsyncDispatcher->post
// AsyncProcessorMessage implementations should contain any data needed by the 
// threads and contain any members required to hold the blocking functions
// return data or results.
//
// When a worker thread picks up the message it will call the virtual method:
// virtual void asyncProcess(AsyncProcessorMessage* msg)
// The implementation of this method should perform the actual blocking function
// calls - ie. database access call.  If there is any data to return it should be
// set in the provided AsyncProcessorMessage before returning from asyncProcess.
//
// The dispatcher will then queue this message up to be sent back to the 
// AsyncProcessor, via the virtual method:
// virtual processor_action_t process(RequestContext &)
// This virtual method will need to examine the event type to see if the event is
// a new request or an asynchronous result message (AsyncProcessorMessage).
// 

/* Example:
class MyAsyncProcessorAsyncMessage : public AsyncProcessorMessage 
{
public:
   MyAsyncProcessorAsyncMessage(AsyncProcessor& proc,
                         const resip::Data& tid,
                         resip::TransactionUser* passedtu):
      AsyncProcessorMessage(proc,tid,passedtu)  { }

   virtual EncodeStream& encode(EncodeStream& strm) const 
   { 
      strm << "MyAsyncProcessorAsyncMessage(tid="<<mTid<<")"; return strm; 
   }

   Data mDataRequiredToCallBlockingFunction;
   Data mDataReturnedFromBlockingFunction;
};

class MyAsyncProcessor : public AsyncProcessor
{
   public:
      MyAsyncProcessor(ProxyConfig& config, Dispatcher* asyncDispatcher) :
         AsyncProcessor("MyAsyncProcessor", asyncDispatcher) {}
      ~MyAsyncProcessor() {}

      // Processor virtual method
      virtual processor_action_t process(RequestContext &rc)
      {
         Message *message = rc.getCurrentEvent();

         MyAsyncProcessorAsyncMessage *async = dynamic_cast<MyAsyncProcessorAsyncMessage*>(message);
         if (async)
         {
            // Async Function is complete - do something with results and continue
            InfoLog(<< "Async function is complete, results are: " << async->mDataReturnedFromBlockingFunction);
            return Continue;
         }
         else
         {
            // Control enters here when request arrives and is passed through process chain
            // Dispatch async request to worker thread pool
            MyAsyncProcessorAsyncMessage* async = new MyAsyncProcessorAsyncMessage(*this, rc.getTransactionId(), &rc.getProxy());
            async->mDataRequiredToCallBlockingFunction = "foo";
            mAsyncDispatcher->post(std::auto_ptr<ApplicationMessage>(async));
            return WaitingForEvent;
         }
      }

      // Virtual method called from WorkerThreads - return true to queue to stack when complete,
      // false when no response is required
      virtual bool asyncProcess(AsyncProcessorMessage* msg)
      {
         MyAsyncProcessorAsyncMessage* async = dynamic_cast<MyAsyncProcessorAsyncMessage*>(msg);
         if(async)
         {
            // Running inside a worker thread here, do blocking work here
            // set any results in MyAsyncProcessorAsyncMessage and return control to Dispatcher
            // that will queue this message back to this processor.
            async->mDataReturnedFromBlockingFunction = "bar";
         }
      }

   private:
};
*/


class AsyncProcessor : public Processor
{
   public:
      AsyncProcessor(const resip::Data& name, Dispatcher* asyncDispatcher, ChainType type=NO_TYPE) :
         Processor(name, type),
         mAsyncDispatcher(asyncDispatcher) {}
      virtual ~AsyncProcessor() {}

      // Processor virtual method
      virtual processor_action_t process(RequestContext &)=0;

      // Virtual method called from WorkerThreads
      // return true to queue to stack when complete, false when no response is required
      virtual bool asyncProcess(AsyncProcessorMessage* msg)=0;

   protected:
      Dispatcher* mAsyncDispatcher;
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
 */
