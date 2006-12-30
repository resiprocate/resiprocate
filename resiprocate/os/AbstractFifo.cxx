#include <cassert>
#include "resiprocate/os/AbstractFifo.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Timer.hxx"

using namespace resip;

AbstractFifo::AbstractFifo(unsigned int maxSize)
   : mSize(0),
     mMaxSize(maxSize)
{
}

AbstractFifo::~AbstractFifo()
{}

void*
AbstractFifo ::getNext()
{
   Lock lock(mMutex); 

   // Wait util there are messages available.
   while (mFifo.empty())
   {
      mCondition.wait(mMutex);
   }

   // Return the first message on the fifo.
   //
   void* firstMessage = mFifo.front();
   mFifo.pop_front();
   assert(mSize != 0);
   mSize--;
   return firstMessage;
}

void*
AbstractFifo::getNext(int ms)
{
   const UInt64 begin(Timer::getTimeMs());
   const UInt64 end(begin + (unsigned int)(ms)); // !kh! the parameter ms should've been unsigned :(

   Lock lock(mMutex); 
   
   // Wait until there are messages available
   while (mFifo.empty())
   {
      const Int64 interval = end - Timer::getTimeMs();
      if(interval <= 0)
      {
         return 0;
      }
      bool signaled = mCondition.wait(mMutex, (unsigned int)interval);
      if (!signaled)
      {
         return 0;
      }
   }

   // Return the first message on the fifo.
   //
   void* firstMessage = mFifo.front();
   mFifo.pop_front();
   assert(mSize != 0);
   mSize--;
   return firstMessage;
}

void*
AbstractFifo::getNext(bool& hasNext)
{
   Lock lock(mMutex); 

   // Wait util there are messages available.
   if (mFifo.empty())
   {
      hasNext = false;
      return 0;
   }

   // Return the first message on the fifo.
   void* firstMessage = mFifo.front();
   mFifo.pop_front();
   assert(mSize != 0);
   mSize--;
   hasNext = !mFifo.empty();
   return firstMessage;
}

bool
AbstractFifo::empty() const
{
   Lock lock(mMutex); 
   return mSize == 0;
}

unsigned int
AbstractFifo ::size() const
{
   Lock lock(mMutex); 
   return mSize;
}

time_t
AbstractFifo::timeDepth() const
{
   return 0;
}

bool
AbstractFifo::messageAvailable() const
{
   Lock lock(mMutex); 
   assert(mSize != NoSize);
   return !mFifo.empty();
}

size_t 
AbstractFifo::getCountDepth() const
{
   return mSize;
}

time_t 
AbstractFifo::getTimeDepth() const
{
   return 0;
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
