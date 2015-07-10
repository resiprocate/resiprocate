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

#include "RWMutex.hxx"
#include "Lock.hxx"
#include "rutil/ResipAssert.h"

using resip::RWMutex;
using resip::Lock;

RWMutex::RWMutex()
   :   Lockable(),
       mReaderCount(0),
       mWriterHasLock(false),
       mPendingWriterCount(0)
{
}


RWMutex::~RWMutex()
{
}


void
RWMutex::readlock()
{
   Lock    lock(mMutex);

   while ( mWriterHasLock || mPendingWriterCount > 0 )
   {
      mReadCondition.wait(mMutex);
   }

   mReaderCount++;
}


void
RWMutex::writelock()
{
   Lock    lock(mMutex);

   mPendingWriterCount++;

   while ( mWriterHasLock || mReaderCount > 0 )
   {
      mPendingWriteCondition.wait(mMutex);
   }

   mPendingWriterCount--;

   mWriterHasLock = true;
}


void
RWMutex::lock()
{
   writelock();
}


void
RWMutex::unlock()
{
   Lock    lock(mMutex);

   // Unlocking a write lock.
   //
   if ( mWriterHasLock )
   {
      resip_assert( mReaderCount == 0 );

      mWriterHasLock = false;

      // Pending writers have priority. Could potentially starve readers.
      //
      if ( mPendingWriterCount > 0 )
      {
         mPendingWriteCondition.signal();
      }

      // No writer, no pending writers, so all the readers can go.
      //
      else
      {
         mReadCondition.broadcast();
      }

   }

   // Unlocking a read lock.
   //
   else
   {
      resip_assert( mReaderCount > 0 );

      mReaderCount--;

      if ( mReaderCount == 0 && mPendingWriterCount > 0 )
      {
         mPendingWriteCondition.signal();
      }
   }
}

unsigned int
RWMutex::readerCount() const
{
   return ( mReaderCount );
}

unsigned int
RWMutex::pendingWriterCount() const
{
   return ( mPendingWriterCount );
}
