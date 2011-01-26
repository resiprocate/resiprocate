#ifndef DISPATCHER_HXX
#define DISPATCHER_HXX 1

#include "repro/WorkerThread.hxx"
#include "repro/Worker.hxx"
#include "resip/stack/ApplicationMessage.hxx"
#include "rutil/TimeLimitFifo.hxx"
#include "rutil/RWMutex.hxx"
#include "rutil/Lock.hxx"
#include <vector>

namespace resip
{ 
   class SipStack; 
   class ApplicationMessage; 
};

namespace repro
{

/**
   @class Dispatcher
   
   @brief A generic thread-bank class.
   
   This is intended to be a generic thread-bank class that operates under
   the paradigm of accepting messages, modifying those messages, and posting
   the messages back to the originator through the SipStack. (Since this uses
   ApplicationMessages, the message will automatically get back to the correct
   TransactionUser; what happens from there is the coder's business.)
   
   This class gets
   generality by using prototyping of a very simple class, Worker. All the
   user must do is subclass Worker to do the work needed, and pass an instance
   of this class when constructing the Dispatcher. Dispatcher will clone this
   Worker as many times as needed to fill the thread bank. 
   
   @note The functions in this class are intended to be thread-safe.
*/

class Dispatcher
{
   public:
   
      /**
         @param prototype The prototypical instance of Worker.
         
         @param stack The stack to post messages to.
         
         @param workers The number of threads in this bank.
         
         @param startImmediately Whether to start this thread bank on 
            construction.
      */
      Dispatcher(std::auto_ptr<Worker> prototype, 
                  resip::SipStack* stack,
                  int workers=2, 
                  bool startImmediately=true);

      virtual ~Dispatcher();
      
      /**
         Posts a message to this thread bank.
         
         @param work The message that conveys the work that needs to be done.
            It is understood that any information that needs to be conveyed
            back to the originator will be placed in the message by the Workers
            in the thread bank.
            
         @returns true iff this message was successfully posted. (This may not
            be the case if this Dispatcher is in the process of shutting down)
      */
      virtual bool post(std::auto_ptr<resip::ApplicationMessage>& work);

      /**
         @returns The number of messages in this Dispatcher's queue
      */
      size_t fifoCountDepth() const;
      
      /**
         @returns The time between which the front of the queue was posted and
            the back of the queue was posted.
      */ 
      time_t fifoTimeDepth() const;
      
      /**
         @returns The number of workers in this thread bank.
      */
      int workPoolSize() const;
      
      /**
         This Dispatcher will stop accepting new
         work, but processing will continue normally on the messages already
         in the queue.
      */
      void stop();
      
      /**
         Resumes accepting work.
      */
      void resume();
      
      /**
         Shuts down the thread-bank.
      */
      void shutdownAll();
      
      
      /**
         Starts the thread bank.
      */
      void startAll();

      resip::SipStack* mStack;

      
   protected:

      resip::TimeLimitFifo<resip::ApplicationMessage> mFifo;
      bool mAcceptingWork;
      bool mShutdown;
      bool mStarted;
      Worker* mWorkerPrototype;

      resip::RWMutex mMutex;

      std::vector<WorkerThread*> mWorkerThreads;

   private:
      //No copying!
      Dispatcher(const Dispatcher& toCopy);
      Dispatcher& operator=(const Dispatcher& toCopy);
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
