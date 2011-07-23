#ifndef RESIP_AbstractFifo_hxx
#define RESIP_AbstractFifo_hxx 

#include <cassert>
#include <deque>

#include "rutil/Mutex.hxx"
#include "rutil/Condition.hxx"
#include "rutil/Lock.hxx"

#include "rutil/compat.hxx"
#include "rutil/Timer.hxx"

namespace resip
{


/// for statistics gathering
class FifoStatsInterface
{
   public:
      virtual ~FifoStatsInterface() {}
      virtual size_t getCountDepth() const = 0;
      virtual time_t getTimeDepth() const = 0;
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
         : FifoStatsInterface()
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
         return mFifo.size();
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

      virtual size_t getCountDepth() const
      {
         return mFifo.size();
      }

      virtual time_t getTimeDepth() const
      {
         return 0;
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


         // Wait util there are messages available.
         while (mFifo.empty())
         {
            mCondition.wait(mMutex);
         }

         // Return the first message on the fifo.
         //
         T firstMessage(mFifo.front());
         mFifo.pop_front();
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
            if (mFifo.empty())	// WATCHOUT: Do not test mSize instead
              return false;
            toReturn = mFifo.front();
            mFifo.pop_front();
            return true;
         }

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
         return true;
      }

      typedef std::deque<T> Messages;

      void getMultiple(Messages& other, unsigned int max)
      {
         Lock lock(mMutex); (void)lock;
         assert(other.empty());
         while (mFifo.empty())
         {
            mCondition.wait(mMutex);
         }

         if(mFifo.size() <= max)
         {
            std::swap(mFifo, other);
         }
         else
         {
            while( 0 != max-- && !mFifo.empty() )
            {
               other.push_back(mFifo.front());
               mFifo.pop_front();
            }
         }
      }

      bool getMultiple(int ms, Messages& other, unsigned int max)
      {
         if(ms==0)
         {
            getMultiple(other,max);
            return true;
         }

         assert(other.empty());
         const UInt64 begin(Timer::getTimeMs());
         const UInt64 end(begin + (unsigned int)(ms)); // !kh! ms should've been unsigned :(
         Lock lock(mMutex); (void)lock;

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
         }
         else
         {
            while( 0 != max-- && !mFifo.empty() )
            {
               other.push_back(mFifo.front());
               mFifo.pop_front();
            }
         }
         return true;
      }

      size_t add(const T& item)
      {
         Lock lock(mMutex); (void)lock;
         mFifo.push_back(item);
         mCondition.signal();
         return mFifo.size();
      }

      size_t addMultiple(Messages& items)
      {
         Lock lock(mMutex); (void)lock;
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
         return mFifo.size();
      }

      /** @brief container for FIFO items */
      Messages mFifo;
      /** @brief access serialization lock */
      mutable Mutex mMutex;
      /** @brief condition for waiting on new queue items */
      Condition mCondition;

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
