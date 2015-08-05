#if !defined(RESIP_FIFO_HXX)
#define RESIP_FIFO_HXX 

#include "rutil/ResipAssert.h"
#include "rutil/AbstractFifo.hxx"
#include "rutil/SelectInterruptor.hxx"

namespace resip
{

/**
   @brief A templated, threadsafe message-queue class.
*/
template < class Msg >
class Fifo : public AbstractFifo<Msg*>
{
   public:
      Fifo(AsyncProcessHandler* interruptor=0);
      virtual ~Fifo();
      
      using AbstractFifo<Msg*>::mFifo;
      using AbstractFifo<Msg*>::mMutex;
      using AbstractFifo<Msg*>::mCondition;
      using AbstractFifo<Msg*>::empty;
      using AbstractFifo<Msg*>::size;

      /// Add a message to the fifo.
      size_t add(Msg* msg);

      typedef typename AbstractFifo<Msg*>::Messages Messages;
      size_t addMultiple(Messages& msgs);

      /** Returns the first message available. It will wait if no
       *  messages are available. If a signal interrupts the wait,
       *  it will retry the wait. Signals can therefore not be caught
       *  via getNext. If you need to detect a signal, use block
       *  prior to calling getNext.
       */
      Msg* getNext();


      /** Returns the next message available. Will wait up to
       *  ms milliseconds if no information is available. If
       *  the specified time passes or a signal interrupts the
       *  wait, this method returns 0. This interface provides
       *  no mechanism to distinguish between timeout and
       *  interrupt.
       */
      Msg* getNext(int ms);

      void getMultiple(Messages& other, unsigned int max);
      bool getMultiple(int ms, Messages& other, unsigned int max);

      /// delete all elements in the queue
      virtual void clear();
      void setInterruptor(AsyncProcessHandler* interruptor);

   private:
      AsyncProcessHandler* mInterruptor;
      Fifo(const Fifo& rhs);
      Fifo& operator=(const Fifo& rhs);
};


template <class Msg>
Fifo<Msg>::Fifo(AsyncProcessHandler* interruptor) : 
   AbstractFifo<Msg*>(),
   mInterruptor(interruptor)
{
}

template <class Msg>
Fifo<Msg>::~Fifo()
{
   clear();
}

template <class Msg>
void 
Fifo<Msg>::setInterruptor(AsyncProcessHandler* interruptor)
{
   Lock lock(mMutex); (void)lock;
   mInterruptor=interruptor;
}


template <class Msg>
void
Fifo<Msg>::clear()
{
   Lock lock(mMutex); (void)lock;
   while ( ! mFifo.empty() )
   {
      delete mFifo.front();
      mFifo.pop_front();
   }
   resip_assert(mFifo.empty());
}

template <class Msg>
size_t
Fifo<Msg>::add(Msg* msg)
{
   size_t size = AbstractFifo<Msg*>::add(msg);
   if(size==1 && mInterruptor)
   {
      // Only do this when the queue goes from empty to not empty.
      mInterruptor->handleProcessNotification();
   }
   return size;
}

template <class Msg>
size_t
Fifo<Msg>::addMultiple(Messages& msgs)
{
   size_t inSize = msgs.size();
   size_t size = AbstractFifo<Msg*>::addMultiple(msgs);
   if(size==inSize && inSize != 0 && mInterruptor)
   {
      // Only do this when the queue goes from empty to not empty.
      mInterruptor->handleProcessNotification();
   }
   return size;
}

template <class Msg>
Msg*
Fifo<Msg> ::getNext()
{
   return AbstractFifo<Msg*>::getNext();
}

template <class Msg>
Msg*
Fifo<Msg> ::getNext(int ms)
{
   Msg* result(0);
   AbstractFifo<Msg*>::getNext(ms, result);
   return result;
}

template <class Msg>
void
Fifo<Msg>::getMultiple(Messages& other, unsigned int max)
{
   AbstractFifo<Msg*>::getMultiple(other, max);
}

template <class Msg>
bool
Fifo<Msg>::getMultiple(int ms, Messages& other, unsigned int max)
{
   return AbstractFifo<Msg*>::getMultiple(ms, other, max);
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
