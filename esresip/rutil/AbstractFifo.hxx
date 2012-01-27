/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#ifndef RESIP_AbstractFifo_hxx
#define RESIP_AbstractFifo_hxx 

#include <cassert>
#include <deque>

#include "rutil/Mutex.hxx"
#include "rutil/Condition.hxx"
#include "rutil/Lock.hxx"
#include "rutil/CongestionManager.hxx"

#include "rutil/compat.hxx"
#include "rutil/Data.hxx"
#include "rutil/Timer.hxx"
#include "rutil/Random.hxx"

namespace resip
{
/**
  @brief Interface for providing metrics on FIFOs, primarily used by 
   CongestionManager.
   Provides four different types of metrics:
      - size : The number of elements in the queue
      - time-depth : The age of the oldest item in the queue (ie, the front)
      - expected wait-time : A heuristic estimating the amount of time a message
         would take to be serviced if it were added to the queue.
      - average service time : The average time it takes to service a single
         element from the queue (this is helpful in congestion control, but is
         mostly intended for logging).
*/
class FifoStatsInterface
{
   public:
         
     FifoStatsInterface();
     virtual ~FifoStatsInterface();
     
     /**
         Returns the expected time it will take to service all messages 
         currently in the queue (in milli-seconds)
     */
     virtual time_t expectedWaitTimeMilliSec() const =0;

     /**
         Returns the difference in time between the youngest and oldest item in 
         the FIFO in seconds
      */
     virtual time_t timeDepth() const = 0;
     
     /**
         Returns the number of elements in the FIFO
      */
     virtual unsigned int size() const = 0;

     /**
         Returns the average time it takes for individual messages to be 
         serviced (in micro-seconds)
     */
     virtual time_t averageServiceTimeMicroSec() const = 0;

      /**
         Returns what classes of elem should be rejected (ie, not posted to this
         fifo right now). Simply put, anything that posts elements to a fifo 
         should call this before doing so, and act accordingly depending on the 
         return. Effectively, this means that the CongestionManager acts in an
         advisory role only; it is up to the caller to decide how or whether to
         implement the CongestionManager's recommendations.
      */
      CongestionManager::RejectionBehavior getRejectionBehavior() const
      {
         if(mManager)
         {
            return mManager->getRejectionBehavior(this);
         }
         return CongestionManager::NORMAL;
      }
      
      /**
         @internal
         Return this fifo's role-number. The meaning of the return is defined on
         a per-application basis, and will have special meaning to the 
         CongestionManager implementation specific to that app. For instance, 
         1 might be understood to represent the main state machine fifo in 
         resip, 2 might indicate a transport fifo (of which there may be 
         several), 3 might indicate a particular TU's fifo, etc.
         These are intended for use by CongestionManager only.
      */
      inline UInt8 getRole() const {return mRole;}

      /**
         @internal
         Set this fifo's role-number.
         @see getRole()
      */
      inline void setRole(UInt8 role) {mRole=role;}

      /**
         Sets this fifo's CongestionManager. This will allow manager to override
         what is returned by getRejectionBehavior()
         @param manager The CongestionManager that is to decide how congested
            this fifo is. Ownership is not taken.
      */
      inline void setCongestionManager(CongestionManager* manager)
      {
         mManager=manager;
      }

      /**
         Unregisters this fifo from its CongestionManager. 
         getRejectionBehavior() will return CongestionManager::NORMAL from now
         until a new CongestionManager is set.
      */
      inline void unregisterFromCongestionManager()
      {
         if(mManager) mManager->unregisterFifo(this);
         mRole=0;
         mManager=0;
      }

      /**
         Sets the description for this fifo. This is used in the logging for
         this fifo's statistics, and can also be used by the CongestionManager 
         to assign a role-number.
         @param description The description for this fifo.
      */
      inline void setDescription(const resip::Data& description)
      {
         mDescription=description;
      }

      /**
         Gets the description for this fifo.
         @see setDescription()
      */
      virtual const resip::Data& getDescription() const {return mDescription;}

   protected:
      Data mDescription;
      CongestionManager* mManager;
      UInt8 mRole;
};

/**
   @brief The base class from which various templated Fifo classes are derived.

   (aka template hoist) 
   AbstractFifo's get operations are all threadsafe; AbstractFifo does not 
   define any put operations (these are defined in subclasses).
   @note Users of the resip stack will not need to interact with this class 
      directly in most cases. Look at Fifo and TimeLimitFifo instead.

   @ingroup message_passing
 */
template <typename T>
class AbstractFifo : public FifoStatsInterface
{
   public:
     /** 
      * @brief Constructor
      * @param maxSize max number of messages to keep
      **/
      AbstractFifo()
         : FifoStatsInterface(),
            mLastMessageGrabbedMicroSec(0),
            mCounter(0),
            mAverageServiceTimeMicroSec(1),
            mNumMessagesGrabbed(0)
      {}

      virtual ~AbstractFifo()
      {
         if(mManager)
         {
            mManager->unregisterFifo(this);
         }
      }

      /** 
         @brief is the queue empty?
         @return true if the queue is empty and false otherwise
       **/
      bool empty() const
      {
         Lock lock(mMutex); (void)lock;
         return mFifo.empty();
      }

      /**
        @brief get the current size of the fifo.
        @note Note you should not use this function to determine
        whether a call to getNext() will block or not. Use
        messageAvailable() instead.
        @return the number of messages in the queue
       */
      virtual unsigned int size() const
      {
         Lock lock(mMutex); (void)lock;
         return mFifo.size();
      }

      /**
      @brief is a message available?
      @retval true if a message is available and false otherwise
         @deprecated Using this to determine when to grab stuff off the fifo
            causes the service-time calculations to fail. Use getNext(0) instead
            if you want to poll.
       */
       
      bool messageAvailable() const
      {
         Lock lock(mMutex); (void)lock;
         onQueuePolled();
         return !mFifo.empty();
      }

      /**
      @brief computes the time delta between the oldest and newest queue members
      @note defaults to zero, overridden by TimeLimitFifo<T>
      @return the time delta between the oldest and newest queue members
      */
      virtual time_t timeDepth() const
      {
         return 0;
      }

      virtual time_t expectedWaitTimeMilliSec() const
      {
         return (mAverageServiceTimeMicroSec*size()+500)/1000;
      }

      virtual time_t averageServiceTimeMicroSec() const
      {
         return mAverageServiceTimeMicroSec;
      }

      /// remove all elements in the queue (or not)
      virtual void clear() {};

   protected:
      /** 
          @brief Returns the first message available.
          @details Returns the first message available. It will wait if no
          messages are available. If a signal interrupts the wait,
          it will retry the wait. Signals can therefore not be caught
          via getNext. If you need to detect a signal, use block
          prior to calling getNext.
          @return the first message available
       */
      T getNext()
      {
         Lock lock(mMutex); (void)lock;

         onQueuePolled();

         // Wait util there are messages available.
         while (mFifo.empty())
         {
            mCondition.wait(mMutex);
         }

         // Return the first message on the fifo.
         //
         T firstMessage(mFifo.front());
         mFifo.pop_front();
         onMessagePopped();
         return firstMessage;
      }


      /**
        @brief Returns the next message available.
        @details Returns the next message available. Will wait up to
        ms milliseconds if no information is available. If
        the specified time passes or a signal interrupts the
        wait, this method returns 0. This interface provides
        no mechanism to distinguish between timeout and
        interrupt.
       */
      bool getNext(int ms, T& toReturn)
      {
      //   if(ms == 0) 
      //   {
      //      return getNext();
      //   }
      
         onQueuePolled();
         const UInt64 begin(Timer::getTimeMs());
         const UInt64 end(begin + (unsigned int)(ms)); // !kh! ms should've been unsigned :(
         Lock lock(mMutex); (void)lock;
      
         // Wait until there are messages available
         while (mFifo.empty())
         {
            if(ms==0)
            {
               return false;
            }
            const UInt64 now(Timer::getTimeMs());
            if(now >= end)
            {
                return false;
            }
      
            unsigned int timeout((unsigned int)(end - now));
                    
            // bail if total wait time exceeds limit
            bool signaled = mCondition.wait(mMutex, timeout);
            if (!signaled)
            {
               return false;
            }
         }
      
         // Return the first message on the fifo.
         //
         toReturn=mFifo.front();
         mFifo.pop_front();
         onMessagePopped();
         return true;
      }

      void swapOut(std::deque<T>& other)
      {
         assert(other.empty());
         Lock lock(mMutex); (void)lock;
         onQueuePolled();
         onMessagePopped(mFifo.size());
         std::swap(mFifo, other);
      }

      bool swapOut(int ms, std::deque<T>& other)
      {
         assert(other.empty());
         const UInt64 begin(Timer::getTimeMs());
         const UInt64 end(begin + (unsigned int)(ms)); // !kh! ms should've been unsigned :(
         Lock lock(mMutex); (void)lock;

         // Wait until there are messages available
         while (mFifo.empty())
         {
            if(ms==0)
            {
               return false;
            }
            const UInt64 now(Timer::getTimeMs());
            if(now >= end)
            {
                return false;
            }

            unsigned int timeout((unsigned int)(end - now));
                    
            // bail if total wait time exceeds limit
            bool signaled = mCondition.wait(mMutex, timeout);
            if (!signaled)
            {
               return false;
            }
         }

         onQueuePolled();
         onMessagePopped(mFifo.size());
         std::swap(mFifo, other);
         return true;
      }

      void add(const T& item)
      {
         Lock lock(mMutex); (void)lock;
         mFifo.push_back(item);
         mCondition.signal();
         onMessagePushed(1);
      }

      void addMultiple(std::deque<T>& items)
      {
         Lock lock(mMutex); (void)lock;
         int size=items.size();
         if(mFifo.empty())
         {
            std::swap(mFifo, items);
         }
         else
         {
            // I suppose it is possible to optimize this as a push_front() from
            // mFifo to items, and then do a swap, if items is larger.
            while(!items.empty())
            {
               mFifo.push_back(items.front());
               items.pop_front();
            }
         }
         mCondition.signal();
         onMessagePushed(size);
      }

      /** @brief container for FIFO items */
      std::deque<T> mFifo;
      /** @brief access serialization lock */
      mutable Mutex mMutex;
      /** @brief condition for waiting on new queue items */
      Condition mCondition;

      mutable UInt64 mLastMessageGrabbedMicroSec;
      mutable UInt8 mCounter;
      mutable UInt64 mAverageServiceTimeMicroSec;
      mutable UInt32 mNumMessagesGrabbed;
      virtual void onQueuePolled() const
      {
         if(mCounter==1 && mLastMessageGrabbedMicroSec!=0)
         {
            UInt64 diff = Timer::getTimeMicroSec()-mLastMessageGrabbedMicroSec;
            // .bwc. This is a moving average with period 64, round to nearest int
            // Ensure that we don't divide by zero later(+1)
            mAverageServiceTimeMicroSec=resipDiv(resipDiv(diff, (UInt64)mNumMessagesGrabbed)+ 63*mAverageServiceTimeMicroSec,64ULL)+1;
            // .bwc. So we don't reuse this figure.
            mLastMessageGrabbedMicroSec=0;
         }
      }

      virtual void onMessagePopped(unsigned int num=1)
      {
         mCounter+=num;
         // !bwc! TODO allow this sampling frequency to be tweaked
         if( mCounter >= 64 )
         {
            mCounter=1;
            mLastMessageGrabbedMicroSec=Timer::getTimeMicroSec();
            mNumMessagesGrabbed=num;
         }
      }

      virtual void onMessagePushed(int num){}
   private:
      // no value semantics
      AbstractFifo(const AbstractFifo&);
      AbstractFifo& operator=(const AbstractFifo&);
};

} // namespace resip

#endif

/* Copyright 2007 Estacado Systems */

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
 * The Vovida Software License, Version 1.0 
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
