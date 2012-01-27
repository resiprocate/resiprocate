/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_LOCK_HXX)
#define RESIP_LOCK_HXX  

#include "rutil/Lockable.hxx"

namespace resip
{

/** Flags to indicate the type of lock being attempted */
enum LockType
{
   VOCAL_LOCK = 0,
   VOCAL_READLOCK,
   VOCAL_WRITELOCK
};

/**
  @brief A convenience class to lock a Lockable (such as a Mutex) on object 
  creation, and unlock on destruction. (ie, a scoped lock)

  @see Mutex
  @ingroup threading
*/
class Lock
{
   public:
     /**
       @brief Calls the appropriate locking function on Lockable based on the 
        LockType flag it receives.
       @param Lockable& The object to lock
       @param LockType one of VOCAL_LOCK, VOCAL_READLOCK, VOCAL_WRITELOCK
      */
      inline Lock(Lockable & lockable)
         : myLockable(lockable)
      {
          myLockable.lock();
      }
     /**
       @brief Calls the unlock function on Lockable 
      */
      inline ~Lock()
      {
          myLockable.unlock();
      }

      bool isMyLockable(const resip::Lockable& l) const
      {
         return &l == &myLockable;
      }

   private:
      Lockable&   myLockable;
};

/**
  @brief A convenience class to lock out writes from occurring 
   in the critical section that will follow it's creation, 
   and unlock on destruction. (ie, a scoped lock)
  @see Lockable
  @ingroup threading
*/
class ReadLock
{
   public:
     /**
       @brief Used to create a readlock based on scoping.
        So sample usage would be: 
        @code
        {
          ReadLock r(yourMutex); (void) r;  
          ... do stuff under the shelter of the readlock
        } // allow the readlock to lapse by going out of scope
        @endcode
       @param Lockable& The object to lock
       */
      inline ReadLock(Lockable & lockable)
         : myLockable(lockable)
      {
          myLockable.readlock();
      }
     /**
       @brief Calls the unlock function on Lockable 
      */
      inline ~ReadLock()
      {
          myLockable.unlock();
      }

   private:
      Lockable&   myLockable;
};

/**
  @brief A convenience class to lock out reads and writes from occurring 
   in the critical section that will follow it's creation, 
   and unlock on destruction. (ie, a scoped lock)
  @see Lockable
  @ingroup threading
*/
class WriteLock
{
   public:
     /**
       @brief Used to create a complete lock (read and write) based on scoping.
        So sample usage would be: 
        @code
        {
          WriteLock r(yourMutex); (void) r;  
          ... do stuff under the shelter of the writelock
        } // allow the writelock to lapse by going out of scope
        @endcode
       @param Lockable& The object to lock
       */
      inline WriteLock(Lockable & lockable)
         : myLockable(lockable)
      {
          myLockable.writelock();
      }
     /**
       @brief Calls the unlock function on Lockable 
      */
      inline ~WriteLock()
      {
          myLockable.unlock();
      }

   private:
      Lockable&   myLockable;
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
