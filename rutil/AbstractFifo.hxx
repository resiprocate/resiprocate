#ifndef RESIP_AbstractFifo_hxx
#define RESIP_AbstractFifo_hxx 

#include "rutil/ResipAssert.h"
#include <deque>

#include "rutil/Mutex.hxx"
#include "rutil/Condition.hxx"
#include "rutil/Lock.hxx"
#include "rutil/CongestionManager.hxx"

#include "rutil/compat.hxx"
#include "rutil/Timer.hxx"

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
      virtual time_t getTimeDepth() const = 0;
     
     /**
         Returns the number of elements in the FIFO
      */
      virtual size_t getCountDepth() const = 0;

     /**
         Returns the average time it takes for individual messages to be 
         serviced (in micro-seconds)
     */
     virtual time_t averageServiceTimeMicroSec() const = 0;

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
      UInt8 mRole;
};

/**
  * The getNext() method takes an argument {ms} that normally
  * the number of milliseconds to wait. There are two special values:
  * NOWAIT
  *     Don't wait/block/sleep. If no message to retrieve, return NULL.
  * FOREVER
  *     Wait forever until a message is available.
  * Note that the encoding (0 vs -1) is the oppositive convention
  * of standard APIs such as epoll_wait(). This is for historical reasons.
  */
#define RESIP_FIFO_NOWAIT	-1
#define RESIP_FIFO_FOREVER	0

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
            mLastSampleTakenMicroSec(0),
            mCounter(0),
            mAverageServiceTimeMicroSec(0),
            mSize(0)
      {}

      virtual ~AbstractFifo()
      {
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
         return (unsigned int)mFifo.size();
      }

      /**
      @brief is a message available?
      @retval true if a message is available and false otherwise
       */
       
      bool messageAvailable() const
      {
         Lock lock(mMutex); (void)lock;
         return !mFifo.empty();
      }

      /**
      @brief computes the time delta between the oldest and newest queue members
      @note defaults to zero, overridden by TimeLimitFifo<T>
      @return the time delta between the oldest and newest queue members
      */
      virtual time_t getTimeDepth() const
      {
         return 0;
      }

      virtual size_t getCountDepth() const
      {
         return mSize;
      }

      virtual time_t expectedWaitTimeMilliSec() const
      {
         return ((mAverageServiceTimeMicroSec*mSize)+500)/1000;
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
         onFifoPolled();

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
         if(ms == 0) 
         {
            toReturn = getNext();
            return true;
         }

         if(ms < 0)
         {
            Lock lock(mMutex); (void)lock;
            onFifoPolled();
            if (mFifo.empty())	// WATCHOUT: Do not test mSize instead
              return false;
            toReturn = mFifo.front();
            mFifo.pop_front();
            return true;
         }

         const UInt64 begin(Timer::getTimeMs());
         const UInt64 end(begin + (unsigned int)(ms)); // !kh! ms should've been unsigned :(
         Lock lock(mMutex); (void)lock;
         onFifoPolled();

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

      typedef std::deque<T> Messages;

      void getMultiple(Messages& other, unsigned int max)
      {
         Lock lock(mMutex); (void)lock;
         onFifoPolled();
         resip_assert(other.empty());
         while (mFifo.empty())
         {
            mCondition.wait(mMutex);
         }

         if(mFifo.size() <= max)
         {
            std::swap(mFifo, other);
            onMessagePopped(mSize);
         }
         else
         {
            size_t num=max;
            while( 0 != max-- )
            {
               other.push_back(mFifo.front());
               mFifo.pop_front();
            }
            onMessagePopped((unsigned int)num);
         }
      }

      bool getMultiple(int ms, Messages& other, unsigned int max)
      {
         if(ms==0)
         {
            getMultiple(other,max);
            return true;
         }

         resip_assert(other.empty());
         const UInt64 begin(Timer::getTimeMs());
         const UInt64 end(begin + (unsigned int)(ms)); // !kh! ms should've been unsigned :(
         Lock lock(mMutex); (void)lock;
         onFifoPolled();

         // Wait until there are messages available
         while (mFifo.empty())
         {
            if(ms < 0)
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

         if(mFifo.size() <= max)
         {
            std::swap(mFifo, other);
            onMessagePopped(mSize);
         }
         else
         {
            size_t num=max;
            while( 0 != max-- )
            {
               other.push_back(mFifo.front());
               mFifo.pop_front();
            }
            onMessagePopped((unsigned int)num);
         }
         return true;
      }

      size_t add(const T& item)
      {
         Lock lock(mMutex); (void)lock;
         mFifo.push_back(item);
         mCondition.signal();
         onMessagePushed(1);
         return mFifo.size();
      }

      size_t addMultiple(Messages& items)
      {
         Lock lock(mMutex); (void)lock;
         size_t size=items.size();
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
         onMessagePushed((int)size);
         return mFifo.size();
      }

      /** @brief container for FIFO items */
      Messages mFifo;
      /** @brief access serialization lock */
      mutable Mutex mMutex;
      /** @brief condition for waiting on new queue items */
      Condition mCondition;

      mutable UInt64 mLastSampleTakenMicroSec;
      mutable UInt32 mCounter;
      mutable UInt32 mAverageServiceTimeMicroSec;
      // std::deque has to perform some amount of traversal to calculate its 
      // size; we maintain this count so that it can be queried without locking, 
      // in situations where it being off by a small amount is ok.
      UInt32 mSize;

      virtual void onFifoPolled()
      {
         // !bwc! TODO allow this sampling frequency to be tweaked
         if(mLastSampleTakenMicroSec &&
            mCounter &&
            (mCounter >= 64 || mFifo.empty()))
         {
            UInt64 now(Timer::getTimeMicroSec());
            UInt64 diff = now-mLastSampleTakenMicroSec;

            if(mCounter >= 4096)
            {
               mAverageServiceTimeMicroSec=(UInt32)resipIntDiv(diff, mCounter);
            }
            else // fifo got emptied; merge into a rolling average
            {
               // .bwc. This is a moving average with period 64, round to 
               // nearest int.
               mAverageServiceTimeMicroSec=(UInt32)resipIntDiv(
                     diff+((4096-mCounter)*mAverageServiceTimeMicroSec),
                     4096U);
            }
            mCounter=0;
            if(mFifo.empty())
            {
               mLastSampleTakenMicroSec=0;
            }
            else
            {
               mLastSampleTakenMicroSec=now;
            }
         }
      }

      /**
         Called when a message (or messages) are removed from this fifo. Used to
         drive service time calculations.
      */
      virtual void onMessagePopped(unsigned int num=1)
      {
         mCounter+=num;
         mSize-=num;
      }

      virtual void onMessagePushed(int num)
      {
         if(mSize==0)
         {
            // Fifo went from empty to non-empty. Take a timestamp, and record
            // how long it takes to process some messages.
            mLastSampleTakenMicroSec=Timer::getTimeMicroSec();
         }
         mSize+=num;
      }
   private:
      // no value semantics
      AbstractFifo(const AbstractFifo&);
      AbstractFifo& operator=(const AbstractFifo&);
};

} // namespace resip

#endif

/* ====================================================================
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
