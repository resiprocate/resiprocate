
#if defined(_WIN32)
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
#include "resiprocate/os/Socket.hxx"
#endif

#include <cassert>
#include <iostream>
#include "resiprocate/os/ThreadIf.hxx"
#include "resiprocate/os/Mutex.hxx"
#include "resiprocate/os/Lock.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

using namespace resip;

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
threadWrapper( void* threadParm )
{
   assert( threadParm );
   ThreadIf* t = static_cast < ThreadIf* > ( threadParm );

   assert( t );
#if defined(_WIN32)
   srand(unsigned(time(0)) ^ unsigned(GetCurrentThreadId()) ^ unsigned(GetCurrentProcessId()));
#endif
   t->thread();
#if defined(_WIN32)
# ifdef _WIN32_WCE
   ExitThread( 0 );
# else
   _endthreadex(0);
# endif
#endif
   return 0;
}
}

ThreadIf::ThreadIf() 
: mId(0)
 ,mShutdown(false)
 ,mShutdownMutex()
#if defined(WIN32)
 ,mThread(0)
#endif
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
   assert(mId == 0);
#if defined(WIN32)
   // !kh!
   // Why _beginthreadex() instead of CreateThread():
   //   http://support.microsoft.com/support/kb/articles/Q104/6/41.ASP
   // Example of using _beginthreadex() mixed with WaitForSingleObject() and CloseHandle():
   //   http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vclib/html/_crt__beginthread.2c_._beginthreadex.asp
   
   assert(mThread == 0);   // for debugging: force coder to call join() when reusing ThreadIf.
   mThread =
#ifdef _WIN32_WCE
       // there is no _beginthreadex() for WINCE
       CreateThread(
#else
       (HANDLE)_beginthreadex(
#endif // _WIN32_WCE
           
         NULL, // LPSECURITY_ATTRIBUTES lpThreadAttributes,  // pointer to security attributes
         0, // DWORD dwStackSize,                         // initial thread stack size
         RESIP_THREAD_START_ROUTINE
         (threadWrapper), // LPTHREAD_START_ROUTINE lpStartAddress,     // pointer to thread function
         this, //LPVOID lpParameter,                        // argument for new thread
         0, //DWORD dwCreationFlags,                     // creation flags
         (unsigned *)&mId// LPDWORD lpThreadId                         // pointer to receive thread ID
         );
   assert( mThread != 0 );
#else
   // spawn the thread
   if ( int retval = pthread_create( &mId, 0, threadWrapper, this) )
   {
      std::cerr << "Failed to spawn thread: " << retval << std::endl;
      assert(0);
      // TODO - ADD LOGING HERE
   }
#endif
   mShutdown = false;
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
   mThread = 0;
#else
   void* stat;
   if (mId != pthread_self())
   {
      int r = pthread_join( mId , &stat );
      if ( r != 0 )
      {
         InfoLog( << "pthread_join() returned " << r );
         assert(0);
         // TODO
      }
   }
   
#endif

   mId       = 0;
}
#if !defined(WIN32)
ThreadIf::Id
ThreadIf::selfId()
{
   return pthread_self();
}
#endif

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
   mShutdownCondition.wait(mShutdownMutex, ms);
   return mShutdown;
}

bool
ThreadIf::isShutdown() const
{
   Lock lock(mShutdownMutex);
   (void)lock;
   return ( mShutdown );
}

bool
ThreadIf::isRunning() const
{
#if defined(WIN32)
   DWORD exitCode;
   return (mThread && GetExitCodeThread(mThread,&exitCode) != 0 && exitCode == STILL_ACTIVE);
#else
   int policy;
   sched_param param;
   return (mId != 0 && pthread_getschedparam(mId, &policy, &param) == 0);
#endif
}

// End of File

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
