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
 * 3. The names "VOCA;L", "Vovida Open Communication Application Library",
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

#include <cassert>

#ifndef WIN32
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#endif

#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/Condition.hxx"
#include "resiprocate/os/Mutex.hxx"
#include "resiprocate/os/Timer.hxx"

using namespace resip;

Condition::Condition() : mId()
{
   //std::cerr << this << " Condition::Condition" << std::endl;
   
#ifdef WIN32
   mId =  CreateEvent(
      NULL, //LPSECURITY_ATTRIBUTES lpEventAttributes,
      // pointer to security attributes
      FALSE, // BOOL bManualReset,  // flag for manual-reset event
      FALSE, //BOOL bInitialState, // flag for initial state
      NULL //LPCTSTR lpName      // pointer to event-object name
      );
   assert(mId);
#else
   int  rc =  pthread_cond_init(&mId,0);
   assert( rc == 0 );
#endif
}


Condition::~Condition ()
{
#ifdef WIN32
   BOOL ok = CloseHandle(mId);
   assert( ok );
#else
   if (pthread_cond_destroy(&mId) == EBUSY)
   {
      //WarningLog (<< "Condition variable is busy");
      assert(0);
   }
#endif
}


void
Condition::wait (Mutex* mutex)
{
   //std::cerr << "Condition::wait " << mutex << std::endl;
#ifdef WIN32 
    // FixMe: Race condition between time we get mId and when we
    // re-acquire the mutex.
    mutex->unlock();
    WaitForSingleObject(mId,INFINITE);
    mutex->lock();
#else
   int ret = pthread_cond_wait(&mId, mutex->getId());
   assert( ret == 0 );
#endif
}

bool
Condition::wait (Mutex* mutex, int ms)
{
#ifdef WIN32 
    // FixMe: Race condition between time we get mId and when we
    // re-acquire the mutex.
	//
	// SLG: A Note about the Win32 Implementation of Conditions
	//
	// I have investigated a fix for this.  A solution to this problem is 
	// non-trivial.  Please read http://www.cs.wustl.edu/~schmidt/win32-cv-1.html 
	// for a full explanation.  This is an implementation of the SetEvent solution
	// discussed in that article.  This solution has the following issues:
	// 1.  Unfairness - ie.  First thread to call wait may not be first thread
	//     to be released from condition.
	// 2.  Incorrectness due to a race condition when a broadcast occurs 
	// (see the link for more details on these issues)
	//
	// There is a solution that corrects these two problem, but also introduces 2 more.
	// This solution (also discussed in the link) requires the use of a primitive only
	// available in WinNT and above.  It also requires that the Mutex passed in be
	// implemented using windows Mutexes instead of CriticalSections - they are less
	// efficient.  Thus the problems with this SignalObjectAndWait solution are:
	// 1.  Not portable to all versions of windows - ie.  will not work with Win98/Me
	// 2.  Less efficient than tthe SetEvent solution
	//
	// I have choosen to stick with the SetEvent Solution for the following reasons:
	// 1.  Speed is important.
	// 2.  The Unfairness issue is not really a big problem since the stack currently
	//     does not call a wait function from two different threads.  (assuming the 
	//     hosting application always calls process() from the same thread).  The only
	//     time multi-threading comes into the picture is when the transports queue
	//     messages from the wire onto the stateMacFifo - but they are retrieved off the
	//     Fifo by a single thread.
	// 3.  The Incorrectness issue is also not a big problem, since the stack currently
	//     doesn't use the broadcast member of this class.
	// 
	// Note:  The implementation of broadcast remains incomplete - since it is currently
	//        unused and would require an additional CriticalSection Enter and Leave to 
	//        keep track of a counter (see the above link for more info).  This can be
	//        easily added in the future if required.
    mutex->unlock();
    DWORD ret = WaitForSingleObject(mId, ms);
    mutex->lock();
	assert(ret != WAIT_FAILED);
	return (ret == WAIT_OBJECT_0);
#else
    UInt64 expires64 = Timer::getTimeMs() + ms;
    timespec expiresTS;
    expiresTS.tv_sec = expires64 / 1000;
    expiresTS.tv_nsec = (expires64 % 1000) * 1000000L;
    
    assert( expiresTS.tv_nsec < 1000000000L );
    
    //std::cerr << "Condition::wait " << mutex << "ms=" << ms << " expire=" << expiresTS.tv_sec << " " << expiresTS.tv_nsec << std::endl;
    int ret = pthread_cond_timedwait(&mId, mutex->getId(), &expiresTS);
   
    if (ret == EINTR || ret == ETIMEDOUT)
    {
       return false;
    }
    else
    {
       //std::cerr << this << " pthread_cond_timedwait failed " << ret << " mutex=" << mutex << std::endl;
       assert( ret == 0 );
       return true;
    }
#endif
}

void
Condition::signal ()
{
#ifdef WIN32
   BOOL ret = SetEvent(
      mId // HANDLE hEvent   // handle to event object
      );
   assert(ret);
	
#else
   int ret = pthread_cond_signal(&mId);
   assert( ret == 0);
#endif
}


void
Condition::broadcast()
{
#ifdef WIN32
   assert(0);
#else
   pthread_cond_broadcast(&mId);
#endif
}


#if 0
const vcondition_t*
Condition::getId () const
{
   return ( &myId );
}
#endif
