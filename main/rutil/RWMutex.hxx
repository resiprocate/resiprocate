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

   @note A readers/writers mutex is a mutex that can be locked in two differing
      ways:
         - A read-lock: Prevents other threads from obtaining a write-lock, but
            not other read-locks.
         - A write-lock: Prevents other threads from obtaining either a 
            read-lock or write-lock.

      Usually, if a thread attempts to aquire a write-lock while read-locks are
      being held, this will prevent any further read-locks from being obtained,
      until the write-lock is aquired and released. This prevents the "writer
      starvation" problem.
*/
class RWMutex : public Lockable
{
    public:
      RWMutex();
      ~RWMutex();
      void readlock();
      void writelock();
      void lock();
      void unlock();
      unsigned int readerCount() const;
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
