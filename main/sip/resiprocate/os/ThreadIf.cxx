
#if defined(WIN32)
#include <stdio.h>
#include <tchar.h>
#endif

#include <cassert>
#include <iostream>
#include "resiprocate/util/ThreadIf.hxx"
#include "resiprocate/util/Mutex.hxx"
#include "resiprocate/util/Lock.hxx"
#include "resiprocate/util/Socket.hxx"



using namespace Vocal2;

extern "C"
{
static void*
threadWrapper( void* threadParm )
{
   assert( threadParm );
   ThreadIf* t = static_cast < ThreadIf* > ( threadParm );
   
   assert( t );
   t->thread();
   return 0;
}
}

ThreadIf::ThreadIf() : mId(0), mShutdown(false), mShutdownMutex()
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
#if defined(WIN32)
   mThread = CreateThread(
      NULL, // LPSECURITY_ATTRIBUTES lpThreadAttributes,  // pointer to security attributes
      0, // DWORD dwStackSize,                         // initial thread stack size
      (LPTHREAD_START_ROUTINE)threadWrapper, // LPTHREAD_START_ROUTINE lpStartAddress,     // pointer to thread function
      this, //LPVOID lpParameter,                        // argument for new thread
      0, //DWORD dwCreationFlags,                     // creation flags
      &mId// LPDWORD lpThreadId                         // pointer to receive thread ID
      );
   assert( thread != NULL );
#else
   // spawn the thread
   if ( int retval = pthread_create( &mId, 0, threadWrapper, this) )
   {
      std::cerr << "Failed to spawn thread: " << retval << std::endl;
      assert(0);
      // TODO - ADD LOGING HERE 
   }
#endif  
}

void
ThreadIf::join()
{
   if (mId == 0)
   {
      return;
   }

#if defined(WIN32)
   DWORD exitCode;
   while ( !GetExitCodeThread(mThread,&exitCode) )
   {
      WaitForSingleObject(mThread,INFINITE);
   }
#else
   void* stat;
   int r = pthread_join( mId , &stat );
   if ( r!= 0 )
   {
      assert(0);
      // TODO 
   }
#endif

   mId = 0;
}

void
ThreadIf::exit()
{
   assert(mId != 0);

#if defined(WIN32)
   ExitThread( 0 );	 
#else
   pthread_exit(0);
#endif

   mId = 0;
}

#if !defined(WIN32)
pthread_t
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
   mShutdownCondition.wait(&mShutdownMutex, ms);
   return mShutdown;
}

bool
ThreadIf::isShutdown() const
{
   Lock lock(mShutdownMutex);
   (void)lock;
   return ( mShutdown );
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
