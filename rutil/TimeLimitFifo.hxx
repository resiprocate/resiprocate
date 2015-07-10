#ifndef RESIP_TimeLimitFifo_hxx
#define RESIP_TimeLimitFifo_hxx 

#include "rutil/ResipAssert.h"
#include <memory>
#include "rutil/AbstractFifo.hxx"
#include <iostream>
#if defined( WIN32 )
#include <time.h>
#endif

// efficiency note: use a circular buffer to avoid list node allocation

// what happens to timers that can't be queued?

namespace resip
{

/**
@brief wraps an object with a timestamp
@internal
*/
template<typename Payload>
class Timestamped
{
   public:
      Timestamped(const Payload& msg, time_t n)
         : mMsg(msg),
           mTime(n)
      {}

      inline const Payload& getMsg() const { return mMsg;} 
      inline void setMsg(const Payload& pMsg) { mMsg = pMsg;}
      inline const time_t& getTime() const { return mTime;} 

   private:
      Payload mMsg;
      time_t mTime;
};

/**
   @brief A templated, threadsafe message-queue class that enforces constraints
      on time-depth.
   @ingroup data_structures
   @example
   @code
      TimeLimitFifo<Foo> tlf(5, 10); // 5 seconds or 10 count limit
      //
      // where Foo is defined as:
      // class Foo
      // {
      //    public:
      //       Foo(const Data& val) : mVal(val) {}
      //       Data mVal;
      // };
      //

      assert(tlf.empty());
      assert(tlf.size() == 0);
      assert(tlf.timeDepth() == 0);

      c = tlf.add(new Foo("first"), TimeLimitFifo<Foo>::EnforceTimeDepth);
      assert(c);

      // no longer empty
      assert(!tlf.empty());
      assert(tlf.size() == 1);
      assert(tlf.timeDepth() == 0);

      sleepMs(2000);

      // still not empty
      assert(!tlf.empty());
      assert(tlf.size() == 1);
      assert(tlf.timeDepth() > 1);

      if (tlf.messageAvailable())
        delete tlf.getNext();

      // Foo* fp = tlf.getNext();
      // assert (fp->mVal == "first")
      // delete fp ;

      // now its empty
      assert(tlf.empty());
      assert(tlf.size() == 0);
      assert(tlf.timeDepth() == 0);
   @endcode
*/
template <class Msg>
class TimeLimitFifo : public AbstractFifo< Timestamped<Msg*> >
{
   public:
      typedef enum {EnforceTimeDepth, IgnoreTimeDepth, InternalElement} DepthUsage;

      /// After it runs out of the lesser of these limits it will start to refuse messages
      TimeLimitFifo(unsigned int maxDurationSecs,
                    unsigned int maxSize);

      virtual ~TimeLimitFifo();

      using AbstractFifo< Timestamped<Msg*> >::mFifo;
      using AbstractFifo< Timestamped<Msg*> >::mMutex;
      using AbstractFifo< Timestamped<Msg*> >::mCondition;
      using AbstractFifo< Timestamped<Msg*> >::empty;
      using AbstractFifo< Timestamped<Msg*> >::size;
      using AbstractFifo< Timestamped<Msg*> >::onMessagePushed;

      /// @brief Add a message to the fifo.
      /// return true iff succeeds
      /// @param Msg* 'Message pointer'
      /// @param DepthUsage : (Needs work...)
      ///    EnforceTimeDepth -- external (non ACK) requests
      ///    IgnoreTimeDepth -- external reponse and ACK
      ///    InternalElement -- internal messages (timers, application postbacks..); use reserved queue space
      ///
      ///    +------------------------------------------------------+
      ///    |                                |          |          |
      ///    +------------------------------------------------------+
      ///    <-----enforce------------------->
      ///    <---------------ignoreTimeDepth------------>
      ///    <--------------------- internalElement---------------->
      ///
      ///    enforce will drop things that exceed the queue 
      ///    ignore will go past that limit to the extent of the queue (eg. 
      ///    internal will basically not drop anything
      ///
      bool add(Msg* msg, DepthUsage usage);

      /** 
         @brief Returns the first message available. 

         @note It will wait if no messages are available. If a signal interrupts 
         the wait, it will retry the wait. Signals can therefore not be 
         caught via getNext. If you need to detect a signal, use block
         prior to calling getNext.
         
         @return the next message in the queue
       **/
      Msg* getNext();
      /** 
         @brief Returns the first message available within a time limit
         @param ms the maximum amount of time to wait in milliseconds for a message
         @note It will wait if no messages are available. If a signal interrupts 
         the wait, it will retry the wait. Signals can therefore not be 
         caught via getNext. If you need to detect a signal, use block
         prior to calling getNext.
         
         @return the next message in the queue, or NULL if the time limit elapses
       **/
      Msg* getNext(int ms);
      
      /** 
         @brief Return the time depth of the queue
         @return the time delta between the youngest and oldest queue members
      */
      virtual time_t timeDepth() const;

      
      /** 
         @brief would an add called now work?
         @param usage what depth policy should be used to determine if the call would work
         @return true if an add called now would work
         @sa DepthUsage
      */
      bool wouldAccept(DepthUsage usage) const;

      /**
        @brief removes all items from the FIFO
      */
      void clear();

      /**
        @brief gets the depth of the FIFO
        @return the depth of the FIFO
      */
      virtual size_t getCountDepth() const;
      /** 
         @brief Return the time depth of the queue
         @return the time delta between the youngest and oldest queue members
         @note equivalent to timeDepth()
      */
      virtual time_t getTimeDepth() const;
      /**
      @brief returns the FIFO count depth tolerance
      @return the FIFO count depth tolerance
      */
      virtual size_t getCountDepthTolerance() const;
      /**
      @brief returns the FIFO time depth tolerance
      @return the FIFO time depth tolerance
      */      
      virtual time_t getTimeDepthTolerance() const;
      /**
      @brief set the count depth tolerance of the FIFO
      @todo getCountDepthTolerance returns a size_t and this takes an
      unsigned int, f.setCountDepthTolerance(f.getCountDepthTolerance())
      will fail to compile on many systems [!]
      @param max the new count depth tolerance of the FIFO
      */
      virtual void setCountDepthTolerance(unsigned int max);
      /**
      @brief set the time depth tolerance of the FIFO
      @param maxSec the maximum time depth tolerance
      @todo examine parallelism in units [!]
      */
      virtual void setTimeDepthTolerance(unsigned int maxSecs);

   private:
      time_t timeDepthInternal() const;
      inline bool wouldAcceptInteral(DepthUsage usage) const;
      TimeLimitFifo(const TimeLimitFifo& rhs);
      TimeLimitFifo& operator=(const TimeLimitFifo& rhs);

      time_t mMaxDurationSecs;
      unsigned int mMaxSize;
      unsigned int mUnreservedMaxSize;
};

template <class Msg>
TimeLimitFifo<Msg>::TimeLimitFifo(unsigned int maxDurationSecs,
                                  unsigned int maxSize)
   : AbstractFifo< Timestamped<Msg*> >(),
     mMaxDurationSecs(maxDurationSecs),
     mMaxSize(maxSize),
     mUnreservedMaxSize((int)((maxSize*8)/10)) // !dlb! random guess
{}

template <class Msg>
TimeLimitFifo<Msg>::~TimeLimitFifo()
{
   clear();
   resip_assert(empty());
}

template <class Msg>
bool
TimeLimitFifo<Msg>::add(Msg* msg,
                        DepthUsage usage)
{
   Lock lock(mMutex); (void)lock;

   if (wouldAcceptInteral(usage))
   {
      time_t n = time(0);
      mFifo.push_back(Timestamped<Msg*>(msg, n));
      onMessagePushed(1);
      mCondition.signal();
      return true;
   }
   else
   {
      return false;
   }
}

template <class Msg>
bool
TimeLimitFifo<Msg>::wouldAccept(DepthUsage usage) const
{
   Lock lock(mMutex); (void)lock;

   return wouldAcceptInteral(usage);
}

template <class Msg>
Msg*
TimeLimitFifo<Msg>::getNext()
{
   Timestamped<Msg*> tm(AbstractFifo< Timestamped<Msg*> >::getNext());
   return tm.getMsg();
}

template <class Msg>
Msg*
TimeLimitFifo<Msg>::getNext(int ms)
{
   Timestamped<Msg*> tm(0,0);
   if(AbstractFifo< Timestamped<Msg*> >::getNext(ms, tm))
   {
      return tm.getMsg();
   }
   return 0;
}

template <class Msg>
time_t
TimeLimitFifo<Msg>::timeDepthInternal() const
{
   if(mFifo.empty())
   {
      return 0;
   }

   return time(0) - mFifo.front().getTime();
}

template <class Msg>
bool
TimeLimitFifo<Msg>::wouldAcceptInteral(DepthUsage usage) const
{
   if ((mMaxSize != 0 &&
        mFifo.size() >= mMaxSize))
   {
      return false;
   }

   if (usage == InternalElement)
   {
      return true;
   }

   if (mUnreservedMaxSize != 0 &&
       mFifo.size() >= mUnreservedMaxSize)
   {
      return false;
   }

   if (usage == IgnoreTimeDepth)
   {
      return true;
   }

   resip_assert(usage == EnforceTimeDepth);

   if (mFifo.size() == 0 ||
       mMaxDurationSecs == 0 ||
       timeDepthInternal() < mMaxDurationSecs)
   {
      return true;
   }

   return false;
}

template <class Msg>
time_t
TimeLimitFifo<Msg>::timeDepth() const
{
   Lock lock(mMutex); (void)lock;
   return timeDepthInternal();
}

template <class Msg>
void
TimeLimitFifo<Msg>::clear()
{
   Lock lock(mMutex); (void)lock;

   while (!mFifo.empty())
   {
      delete mFifo.front().getMsg();
      mFifo.pop_front();
   }
}

template <class Msg>
size_t
TimeLimitFifo<Msg>::getCountDepth() const
{
   return size();
}

template <class Msg>
size_t
TimeLimitFifo<Msg>::getCountDepthTolerance() const
{
   return mUnreservedMaxSize;
}

template <class Msg>
time_t 
TimeLimitFifo<Msg>::getTimeDepth() const
{
   return timeDepth();
}

template <class Msg>
time_t
TimeLimitFifo<Msg>::getTimeDepthTolerance() const
{
   return mMaxDurationSecs;
}

template <class Msg>
void
TimeLimitFifo<Msg>::setCountDepthTolerance(unsigned int maxCount)
{
   Lock lock(mMutex); (void)lock;
   mUnreservedMaxSize=int(maxCount * 0.8);
}

template <class Msg>
void
TimeLimitFifo<Msg>::setTimeDepthTolerance(unsigned int maxSecs)
{
   Lock lock(mMutex); (void)lock;

   mMaxDurationSecs=maxSecs;
}


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
