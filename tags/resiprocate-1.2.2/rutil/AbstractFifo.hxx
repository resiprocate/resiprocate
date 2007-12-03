#ifndef RESIP_AbstractFifo_hxx
#define RESIP_AbstractFifo_hxx 

#include <deque>

#include "rutil/Mutex.hxx"
#include "rutil/Condition.hxx"
#include "rutil/Lock.hxx"

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

/** First in first out queue template hoist.
 */
class AbstractFifo : public FifoStatsInterface
{
   public:
	  /** 
	   * Constructor : 
	   * @param maxSize (int value to specify a max number of messages to keep)
	   **/
      AbstractFifo(unsigned int maxSize);
      virtual ~AbstractFifo();

      /** 
       * @retval bool (Check if the queue of messages is empty ?)                   
       **/
      bool empty() const;
            
      /** Get the current size of the fifo. Note you should not use this function
       *  to determine whether a call to getNext() will block or not.
       *  Use messageAvailable() instead.
       */
      unsigned int size() const;

      /** @retval true (is a message is available ?)
       */
      bool messageAvailable() const;

      /// defaults to zero, overridden by TimeLimitFifo<T>
      virtual time_t timeDepth() const;

      /// remove all elements in the queue (or not)
      virtual void clear() {};

      virtual size_t getCountDepth() const;
      virtual time_t getTimeDepth() const;

   protected:
      /** Returns the first message available. It will wait if no
       *  messages are available. If a signal interrupts the wait,
       *  it will retry the wait. Signals can therefore not be caught
       *  via getNext. If you need to detect a signal, use block
       *  prior to calling getNext.
       */
      void* getNext();


      /** Returns the next message available. Will wait up to
       *  ms milliseconds if no information is available. If
       *  the specified time passes or a signal interrupts the
       *  wait, this method returns 0. This interface provides
       *  no mechanism to distinguish between timeout and
       *  interrupt.
       */
      void* getNext(int ms);

      enum {NoSize = 0UL -1};

      std::deque<void*> mFifo;
      unsigned long mSize;
      unsigned long mMaxSize;
      
      mutable Mutex mMutex;
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
