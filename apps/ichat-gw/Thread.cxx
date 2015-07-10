#include "rutil/ResipAssert.h"

#include "Thread.hxx"

using namespace gateway;
using namespace std;

#if defined(WIN32)
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <process.h> // for _beginthreadex()
typedef unsigned(__stdcall *THREAD_START_ROUTINE)(void*);
#endif

#include "rutil/ResipAssert.h"
#include <iostream>

extern "C"
{
static void* 
#ifdef WIN32
__stdcall
#endif
threadWrapper( void* parm )
{
   resip_assert( parm );
   Thread* t = static_cast < Thread* > ( parm );

   resip_assert( t );
   t->thread();
#ifdef WIN32
   _endthreadex(0);
#endif
   return 0;
}
}

Thread::Thread() : 
#ifdef WIN32
   mThread(0),
#endif
   mId(0), mShutdown(false)
{
}


Thread::~Thread()
{
   shutdown();
   join();
}

void
Thread::run()
{
   resip_assert(mId == 0);

#if defined(WIN32)
   mThread =
       (HANDLE)_beginthreadex 
         (
         NULL, // LPSECURITY_ATTRIBUTES lpThreadAttributes,  // pointer to security attributes
         0, // DWORD dwStackSize,                         // initial thread stack size
         THREAD_START_ROUTINE
         (threadWrapper), // LPTHREAD_START_ROUTINE lpStartAddress,     // pointer to thread function
         this, //LPVOID lpParameter,                        // argument for new thread
         0, //DWORD dwCreationFlags,                     // creation flags
         &mId// LPDWORD lpThreadId                         // pointer to receive thread ID
         );
   resip_assert( mThread != 0 );
#else
   // spawn the thread
   if ( int retval = pthread_create( &mId, 0, threadWrapper, this) )
   {
      std::cerr << "Failed to spawn thread: " << retval << std::endl;
      resip_assert(0);
   }
#endif
}

void
Thread::join()
{
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
         break;
      }
   }

   CloseHandle(mThread);
   mThread=0;
#else
   void* stat;
   if (mId != pthread_self())
   {
      int r = pthread_join( mId , &stat );
      if ( r != 0 )
      {
         cerr << "Internal error: pthread_join() returned " << r << endl;
         resip_assert(0);
      }
   }
   
#endif

   mId = 0;
}

void
Thread::shutdown()
{
   mShutdown = true;
}

bool
Thread::isShutdown() const
{
   return mShutdown;
}


/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */

