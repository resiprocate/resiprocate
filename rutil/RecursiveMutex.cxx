#include "rutil/ResipAssert.h"
#include <cerrno>

#include "rutil/RecursiveMutex.hxx"

#if defined(WIN32) 
#  include <windows.h>
#  include <winbase.h>
#else
#  include <pthread.h>
#endif

#if defined(__INTEL_COMPILER)
// !rk! hack, I'm not sure off hand why the Intel compiler can't find this
extern int pthread_mutexattr_settype (pthread_mutexattr_t *__attr, int __kind)
   __THROW;
#endif

using resip::RecursiveMutex;

// !abr! I can't find evidence that the Intel C++ Compiler has any issues
// with the use of recursive mutexes. The need for this exception should
// be re-verified and documented here.

// .abr. .amr. OS X 10.2 is 1020. Prior to this, OS X did not support
// recursive mutexes. The iPhone Macro will be defined for all iPhone SDK based
// compiles which properly supports recursive mutexes

#if (defined( __APPLE__ ) && (defined(MAC_OS_X_VERSION_MIN_REQUIRED) && (MAC_OS_X_VERSION_MIN_REQUIRED < 1020) && !defined(TARGET_OS_IPHONE))) || defined (__INTEL_COMPILER)
// !cj! need to write intel mutex stuff 

#warning "RecursiveMutex is not available on this platform yet."

namespace resip
{

RecursiveMutex::RecursiveMutex()
{
   resip_assert(0); 
}


RecursiveMutex::~RecursiveMutex ()
{
   resip_assert(0);
}


void
RecursiveMutex::lock()
{
   resip_assert(0); 
}

void
RecursiveMutex::unlock()
{
   resip_assert(0); 
}

}

#else

namespace resip
{

RecursiveMutex::RecursiveMutex()
{
#ifndef WIN32
   int rc = pthread_mutexattr_init(&mMutexAttr);
 #if defined(__linux__)
   pthread_mutexattr_settype(&mMutexAttr, PTHREAD_MUTEX_RECURSIVE_NP);
 #else
   pthread_mutexattr_settype(&mMutexAttr, PTHREAD_MUTEX_RECURSIVE);
 #endif

   rc = pthread_mutex_init(&mId, &mMutexAttr);
   resip_assert( rc == 0 );
#else
	InitializeCriticalSection(&mId);
#endif
}


RecursiveMutex::~RecursiveMutex ()
{
#ifndef WIN32
    int  rc = pthread_mutex_destroy(&mId);
    resip_assert( rc != EBUSY );  // currently locked 
    resip_assert( rc == 0 );
    rc = pthread_mutexattr_destroy(&mMutexAttr);
#else
	DeleteCriticalSection(&mId);
#endif
}


void
RecursiveMutex::lock()
{
#ifndef WIN32
    int  rc = pthread_mutex_lock(&mId);
    (void)rc;
    resip_assert( rc != EINVAL );
    resip_assert( rc != EDEADLK );
    resip_assert( rc == 0 );
#else
	EnterCriticalSection(&mId);
#endif
}

void
RecursiveMutex::unlock()
{
#ifndef WIN32
    int  rc = pthread_mutex_unlock(&mId);
    (void)rc;
    resip_assert( rc != EINVAL );
    resip_assert( rc != EPERM );
    resip_assert( rc == 0 );
#else
	LeaveCriticalSection(&mId);
#endif
}

#ifndef WIN32
pthread_mutex_t*
RecursiveMutex::getId() const
{
    return ( &mId );
}
#endif

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
