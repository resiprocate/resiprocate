/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_RWMUTEX_HXX)
#define RESIP_RWMUTEX_HXX 

static const char* const resipRWMutex_hxx_Version =
   "$Id: RWMutex.hxx,v 1.3 2003/06/02 20:52:32 ryker Exp $";

#include "Lockable.hxx"
#include "Mutex.hxx"
#include "Condition.hxx"

namespace resip
{

/**
   @brief Wraps the readers/writers mutex implementation on your platform.
   @note A readers/writers mutex is a mutex that can be locked in two ways:
         - A read-lock: Prevents other threads from obtaining a write-lock, but
            not other read-locks.
         - A write-lock: Prevents other threads from obtaining either a 
            read-lock or write-lock.

      If a thread attempts to aquire a write-lock while read-locks are being 
      held, this will prevent any further read-locks from being obtained,
      until the write-lock is acquired and released. This prevents the "writer
      starvation" problem.
   @ingroup threading
   @see Lockable
*/
class RWMutex : public Lockable
{
    public:
      /**
        @brief initializes reader and writer counts to zero and sets writerlock
         status as unlocked
        */
      RWMutex();
      ~RWMutex();
      /**
        @brief Wait for writers and pending writers to finish and then put the
         lock in to prevent writing while the critical section is in progress.
        */
      void readlock();
      /**
        @brief Wait for writers, pending writers or current readers to finish 
         and then put this lock in to prevent reading as well as writing 
         while the critical section is in progress.
        */
      void writelock();
      /**
        @brief calls writelock
        */
      void lock();
      /**
        @brief Unlock read or write locks
         If writelock exists, then unlock it, check for pending writers, and 
         allow them to write. If no pending writers exist, then let the readers
         have a field day.
         If writelock does NOT exist, this allows pending writers to proceed
         if the number of readers are now down to zero.
         But if there are no pending writers, this just reduces the count for
         the number of readers.
        */
      void unlock();
      /**
        @brief getter for number of readers
        */
      unsigned int readerCount() const;
      /**
        @brief getter for number of pending writers
        */
      unsigned int pendingWriterCount() const;
      
   private:
      Mutex mMutex;
      Condition mReadCondition;
      Condition mPendingWriteCondition;
      unsigned int mReaderCount;
      bool mWriterHasLock;
      unsigned int mPendingWriterCount;
};

}

#endif 

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
