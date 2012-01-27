/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_THREADIF_HXX)
#define RESIP_THREADIF_HXX

#include "rutil/Socket.hxx"

#ifdef WIN32
#  include <BaseTsd.h>
#  include <winbase.h>
#else
#  include <pthread.h>
#  include <sched.h>
#endif

#include "rutil/Mutex.hxx"
#include "rutil/Condition.hxx"

namespace resip
{

/** 
   @brief A wrapper class to create and spawn a thread.  

   It is a base class.
   ThreadIf::thread() is a pure virtual method .

   To use this class, derive from it and override the thread() method.
   To start the thread, call the run() method.  The code in thread() will
   run in a separate thread.

   Call shutdown() from the constructing thread to shut down the
   code.  This will set the bool shutdown_ to true.  The code in
   thread() should react properly to shutdown_ being set, by
   returning.  Call join() to join the code.

   Sample:
   @code
   DerivedThreadIf thread;
   thread.run();
   // ... do stuff ...
   thread.shutdown();
   thread.join();
   @endcode

   @ingroup threading
*/
class ThreadIf
{
   public:
      ThreadIf();
      /** 
        @brief Calls shutdown and join
        */
      virtual ~ThreadIf();

      /** 
        @brief Starts a new thread executing the code in
        thread(). Returns immediately.
        */
      virtual void run();

      /**
        @brief Joins to the thread running thread()
        */
      void join();

      /**
        @brief Guarantees resources consumed by thread are released when 
         thread terminates. After this join can no-longer be used.
        */
      void detach();

      /**
        @brief Request the thread running thread() to return, by
        setting mShutdown..
        */
      virtual void shutdown();

      /**
        @brief Waits for waitMs, or stops waiting and returns true if 
         shutdown was called.
        */
      virtual bool waitForShutdown(int ms) const;

      /**
        @brief Checks status of thread.
        @retval true if the thread has been asked to shutdown or not running
        */
      bool isShutdown() const;

      /**
        @brief Sets the priority of the thread
        @param priority the new priority of the thread
        @retval true only if it was successful in setting the priority
        */
      bool setThreadPriority(int priority);

      /**
        @brief Returns the priority of the thread
        @retval the priority of the thread 
        @retval -1 on failure
        */
      int getThreadPriority();
      
#ifdef WIN32
      typedef DWORD Id;
#else
      typedef pthread_t Id;
#endif
      static Id selfId();

#ifdef WIN32
      typedef DWORD TlsKey;
#else
      typedef pthread_key_t TlsKey;
#endif
      typedef void TlsDestructor(void*);

      /** This function follows pthread_key_create() signature */
      static int tlsKeyCreate(TlsKey &key, TlsDestructor *destructor);
      /** This function follows pthread_key_delete() signature */
      static int tlsKeyDelete(TlsKey key);
      /** This function follows pthread_setspecific() signature */
      static int tlsSetValue(TlsKey key, const void *val);
      /** This function follows pthread_getspecific() signature */
      static void *tlsGetValue(TlsKey key);


      /**
        @brief Thread is a virtual method.  Users should derive and define
        thread() such that it returns when isShutdown() is true.
      */
      virtual void thread() = 0;

   protected:
#ifdef WIN32
      HANDLE mThread;
      /// 1088 is the maximum number of TLS slots under Windows.
      /// Refer to http://msdn.microsoft.com/en-us/library/ms686749(VS.85).aspx
      enum {TLS_MAX_KEYS=1088};
   public:
      /// Free data in TLS slots. For internal use only!
      static void tlsDestroyAll();
   protected:
      /// Array of TLS destructors. We have to emulate TLS destructors under Windows.
      static TlsDestructor **mTlsDestructors;
      /// Mutex to protect access to mTlsDestructors
      static Mutex *mTlsDestructorsMutex;
      friend class TlsDestructorInitializer;
#endif
      Id mId;

      bool mShutdown;
      mutable Mutex mShutdownMutex;
      mutable Condition mShutdownCondition;

   private:
      // Suppress copying
      ThreadIf(const ThreadIf &);
      const ThreadIf & operator=(const ThreadIf &);
};

#ifdef WIN32
/// Class to initialize TLS destructors array under Windows on program startup.
class TlsDestructorInitializer {
public:
   TlsDestructorInitializer();
   ~TlsDestructorInitializer();
protected:
   static unsigned int mInstanceCounter;
};
static TlsDestructorInitializer _staticTlsInit;
#endif

}

#endif

/* Copyright 2007 Estacado Systems */

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
