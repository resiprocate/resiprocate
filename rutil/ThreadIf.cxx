#include "rutil/ThreadIf.hxx"

#if defined(WIN32)
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#ifdef _WIN32_WCE
typedef LPTHREAD_START_ROUTINE RESIP_THREAD_START_ROUTINE;
#else
#include <process.h> // for _beginthreadex()
typedef unsigned(__stdcall *RESIP_THREAD_START_ROUTINE)(void*);
#endif

//from Random.cxx
#include "rutil/Socket.hxx"
#endif

#include "rutil/ResipAssert.h"
#include <iostream>
#include "rutil/ThreadIf.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/Lock.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

using namespace resip;

#ifdef WIN32
ThreadIf::TlsDestructorMap *ThreadIf::mTlsDestructors;
Mutex *ThreadIf::mTlsDestructorsMutex;
#endif

extern "C"
{
static void*
#ifdef WIN32
#ifdef _WIN32_WCE
WINAPI
#else
__stdcall
#endif
#endif
threadIfThreadWrapper( void* threadParm )
{
   resip_assert( threadParm );
   ThreadIf* t = static_cast < ThreadIf* > ( threadParm );

   resip_assert( t );
   t->thread();
#ifdef WIN32
   // Free data in TLS slots.
   ThreadIf::tlsDestroyAll();
#ifdef _WIN32_WCE
   ExitThread( 0 );
#else
   _endthreadex(0);
#endif
#endif
   return 0;
}
}

#ifdef WIN32
unsigned int TlsDestructorInitializer::mInstanceCounter=0;
TlsDestructorInitializer::TlsDestructorInitializer()
{
   if (mInstanceCounter++ == 0)
   {
      ThreadIf::mTlsDestructorsMutex = new Mutex();
      ThreadIf::mTlsDestructors = new ThreadIf::TlsDestructorMap;
   }
}
TlsDestructorInitializer::~TlsDestructorInitializer()
{
   if (--mInstanceCounter == 0)
   {
      delete ThreadIf::mTlsDestructorsMutex;
      ThreadIf::mTlsDestructors->clear();
      delete ThreadIf::mTlsDestructors;
   }
}
#endif


ThreadIf::ThreadIf() : 
#ifdef WIN32
   mThread(0),
#endif
   mId(0), mShutdown(false), mShutdownMutex()
{
}


ThreadIf::~ThreadIf()
{
   shutdown();
   join();
}

void
ThreadIf::run()
{
   resip_assert(mId == 0);

#if defined(WIN32)
   // !kh!
   // Why _beginthreadex() instead of CreateThread():
   //   http://support.microsoft.com/support/kb/articles/Q104/6/41.ASP
   // Example of using _beginthreadex() mixed with WaitForSingleObject() and CloseHandle():
   //   http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vclib/html/_crt__beginthread.2c_._beginthreadex.asp
   
   mThread =
#ifdef _WIN32_WCE
       // there is no _beginthreadex() for WINCE
       CreateThread
#else
       (HANDLE)_beginthreadex 
#endif // _WIN32_WCE
         (
         NULL, // LPSECURITY_ATTRIBUTES lpThreadAttributes,  // pointer to security attributes
         0, // DWORD dwStackSize,                         // initial thread stack size
         RESIP_THREAD_START_ROUTINE
         (threadIfThreadWrapper), // LPTHREAD_START_ROUTINE lpStartAddress,     // pointer to thread function
         this, //LPVOID lpParameter,                        // argument for new thread
         0, //DWORD dwCreationFlags,                     // creation flags
         (unsigned*)&mId// LPDWORD lpThreadId                         // pointer to receive thread ID
         );
   resip_assert( mThread != 0 );
#else
   // spawn the thread
   if ( int retval = pthread_create( &mId, 0, threadIfThreadWrapper, this) )
   {
      std::cerr << "Failed to spawn thread: " << retval << std::endl;
      resip_assert(0);
      // TODO - ADD LOGING HERE
   }
#endif
}

void
ThreadIf::join()
{
   // !kh!
   // perhaps assert instead of returning when join()ed already?
   // programming error?
   //assert(mId == 0);

   if (mId == 0)
   {
      return;
   }

#if defined(WIN32)
   DWORD exitCode;
   while (true)
   {
      if (GetExitCodeThread(mThread,&exitCode) != 0)
      {
         if (exitCode != STILL_ACTIVE)
         {
            break;
         }
         else
         {
            WaitForSingleObject(mThread,INFINITE);
         }
      }
      else
      {
         // log something here
         break;
      }
   }

   //  !kh!
   CloseHandle(mThread);
   mThread=0;
#else
   void* stat;
   if (mId != pthread_self())
   {
      int r = pthread_join( mId , &stat );
      if ( r != 0 )
      {
         WarningLog( << "Internal error: pthread_join() returned " << r );
         resip_assert(0);
         // TODO
      }
   }
   
#endif

   mId = 0;
}

void
ThreadIf::detach()
{
#if !defined(WIN32)
   pthread_detach(mId);
#else
   if(mThread)
   {
      CloseHandle(mThread);
      mThread = 0;
   }
#endif
   mId = 0;
}

ThreadIf::Id
ThreadIf::selfId()
{
#if defined(WIN32)
   return GetCurrentThreadId();
#else
   return pthread_self();
#endif
}

int
ThreadIf::tlsKeyCreate(TlsKey &key, TlsDestructor *destructor)
{
#if defined(WIN32)
   key = TlsAlloc();
   if (key!=TLS_OUT_OF_INDEXES)
   {
      Lock lock(*mTlsDestructorsMutex);
      (*mTlsDestructors)[key] = destructor;
      return 0;
   }
   else
   {
      return GetLastError();
   }
#else
   return pthread_key_create(&key, destructor);
#endif
}

int
ThreadIf::tlsKeyDelete(TlsKey key)
{
#if defined(WIN32)
   if (TlsFree(key)>0)
   {
      Lock lock(*mTlsDestructorsMutex);
      mTlsDestructors->erase(key);
      return 0;
   }
   else
   {
      return GetLastError();
   }
#else
   return pthread_key_delete(key);
#endif
}

int
ThreadIf::tlsSetValue(TlsKey key, const void *val)
{
#if defined(WIN32)
   return TlsSetValue(key, (LPVOID)val)>0?0:GetLastError();
#else
   return pthread_setspecific(key, val);
#endif
}

void *
ThreadIf::tlsGetValue(TlsKey key)
{
#if defined(WIN32)
   return TlsGetValue(key);
#else
   return pthread_getspecific(key);
#endif
}


void
ThreadIf::shutdown()
{
   Lock lock(mShutdownMutex);
   if (!mShutdown)
   {
      mShutdown = true;
      mShutdownCondition.signal();
   }
}

bool
ThreadIf::waitForShutdown(int ms) const
{
   Lock lock(mShutdownMutex);
   if(!mShutdown)
   {
      mShutdownCondition.wait(mShutdownMutex, ms);
   }
   return mShutdown;
}

bool
ThreadIf::isShutdown() const
{
   Lock lock(mShutdownMutex);
   (void)lock;
   return ( mShutdown );
}

#ifdef WIN32
void
ThreadIf::tlsDestroyAll()
{
   Lock lock(*mTlsDestructorsMutex);
   ThreadIf::TlsDestructorMap::const_iterator i = mTlsDestructors->begin();
   while(i != mTlsDestructors->end())
   {
      void *val = TlsGetValue(i->first);
      if (val != NULL)
      {
         (*(i->second))(val);
      }
      i++;
   }
}
#endif

// End of File

/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2000-2008 Vovida Networks, Inc.  All rights reserved.
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
