#if !defined(RESIP_FIFO_HXX)
#define RESIP_FIFO_HXX 

static const char* const resipFifo_h_Version =
   "$Id: Fifo.hxx,v 1.15 2003/06/03 15:15:05 ryker Exp $";

#include "resiprocate/os/Mutex.hxx"
#include "resiprocate/os/Condition.hxx"
#include "resiprocate/os/Lock.hxx"
#include <list>

#include <errno.h>


/** Infrastructure common to VOCAL.
 */
namespace resip
{

/** First in first out list interface, with the added functionality of 
 *  being able to handle timed entries.
 */
template < class Msg >
class Fifo
{
   public:
      Fifo();
      virtual ~Fifo();
      
      // Add a message to the fifo.
      void add(Msg* msg);

      /** Returns the first message available. It will wait if no
       *  messages are available. If a signal interrupts the wait,
       *  it will retry the wait. Signals can therefore not be caught
       *  via getNext. If you need to detect a signal, use block
       *  prior to calling getNext.
       */
      Msg* getNext();
      
      /** Get the current size of the fifo. Note you should not use this function
       *  to determine whether a call to getNext() will block or not.
       *  Use messageAvailable() instead.
       */
      unsigned int size() const;

      /** Returns true if a message is available.
       */
      bool messageAvailable() const;

   protected:
      typedef std::list < Msg* > MessageContainer;
      MessageContainer mFifo;
      unsigned long mSize;
      
      mutable Mutex mMutex;
      Condition mCondition;
};


template <class Msg>
Fifo<Msg>::Fifo() : mSize(0)
{
}

template <class Msg>
Fifo<Msg>::~Fifo()
{
   Lock lock(mMutex); (void)lock;
   while ( ! mFifo.empty() )
   {
      delete mFifo.front();
      mFifo.pop_front();
   }
   mSize = -1UL;
}

template <class Msg>
void
Fifo<Msg>::add(Msg* msg)
{
   Lock lock(mMutex); (void)lock;
   mFifo.push_back(msg);
   mSize++;
   mCondition.signal();
}


template <class Msg>
Msg*
Fifo<Msg> ::getNext()
{
   Lock lock(mMutex); (void)lock;

   // Wait while there are messages available.
   //
   while ( mFifo.empty() )
   {
      mCondition.wait(&mMutex);
   }

   // Return the first message on the fifo.
   //
   Msg* firstMessage = mFifo.front();
   mFifo.pop_front();
   mSize--;

   return ( firstMessage );
}


template <class Msg>
unsigned int
Fifo<Msg> ::size() const
{
   Lock lock(mMutex); (void)lock;
   return mSize; 
}


template <class Msg>
bool
Fifo<Msg>::messageAvailable() const
{
   Lock lock(mMutex); (void)lock;
   
   return ( !mFifo.empty() );
}

} // namespace resip

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
