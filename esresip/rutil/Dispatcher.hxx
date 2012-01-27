#ifndef Dispatcher_Include_Guard
#define Dispatcher_Include_Guard

#include "rutil/CongestionManager.hxx"
#include "rutil/Lock.hxx"
#include "rutil/RWMutex.hxx"
#include "rutil/TimeLimitFifo.hxx"
#include "rutil/Worker.hxx"
#include "rutil/WorkerThread.hxx"

#include <vector>

namespace resip
{

/**
   @ingroup threading
   @brief A generic thread-bank class.

   This class gets generality by using prototyping of a very simple class, 
   Worker. All the user must do is subclass Worker to do the work needed, and 
   pass an instance of this class when constructing the Dispatcher. Dispatcher 
   will clone this Worker as many times as needed to fill the thread bank. 

   @note The functions in this class are intended to be thread-safe.
   @note Implementors should note that this will be a thread-bank of finite 
      size; you should ensure that you never post something that will cause your 
      subclass of Worker to block for an unreasonable amount of time.
*/
class Dispatcher
{
   public:
      /**
         @param prototype The prototypical instance of Worker.
         @param workers The number of threads in this bank.
         @param startImmediately Whether to start this thread bank on 
            construction.
      */
      explicit Dispatcher(std::auto_ptr<Worker> prototype, 
                           int workers=2, 
                           bool startImmediately=true);

      virtual ~Dispatcher();

      /**
         Posts a task to this thread bank.

         @param task The object that conveys the task that needs to be done.
            It is understood that any information that needs to be conveyed
            back to the originator will be placed in the Task by the Workers
            in the thread bank. If the Dispatcher does not accept the work, it 
            will not retain a reference to the Task. In no event does Dispatcher 
            take ownership of any Task.

         @returns true iff this message was successfully posted. (This may not
            be the case if this Dispatcher is in the process of shutting down)
      */
      virtual bool post(Task& task);

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

      inline CongestionManager::RejectionBehavior getRejectionBehavior() const
      {
         return mFifo.getRejectionBehavior();
      }

      inline void setCongestionManager(CongestionManager* manager)
      {
         mFifo.unregisterFromCongestionManager();
         if(manager)
         {
            manager->registerFifo(&mFifo,CongestionManager::WAIT_TIME,200);
         }
      }

      inline void setFifoDescription(const Data& description)
      {
         mFifo.setDescription(description);
      }

   protected:
      TimeLimitFifo<Task> mFifo;
      bool mAcceptingWork;
      bool mShutdown;
      bool mStarted;
      std::auto_ptr<Worker> mWorkerPrototype;

      RWMutex mMutex;

      std::vector<WorkerThread*> mWorkerThreads;
   private:

      // disabled
      Dispatcher();
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
