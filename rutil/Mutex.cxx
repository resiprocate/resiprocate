/*
namespace std
{
typedef wchar_t wint_t;
typedef unsigned int size_t;
}

*/
#include "rutil/ResipAssert.h"
#include <cerrno>
#include "rutil/Mutex.hxx"

#if defined(WIN32)
#  include <windows.h>
#  include <winbase.h>
#else
#  include <pthread.h>
#endif

using namespace resip;

Mutex::Mutex()
{
#ifndef WIN32
    int  rc = pthread_mutex_init(&mId,0);
    (void)rc;
    resip_assert( rc == 0 );
#else
	// Note:  Windows Critical sections are recursive in nature and perhaps
	//        this implementation calls for a non-recursive implementation
	//        (since there also exists a RecursiveMutex class).  The effort
	//        to make this non-recursive just doesn't make sense though. (SLG)
	InitializeCriticalSection(&mId);
#endif
}


Mutex::~Mutex ()
{
#ifndef WIN32
    int  rc = pthread_mutex_destroy(&mId);
    (void)rc;
    resip_assert( rc != EBUSY );  // currently locked 
    resip_assert( rc == 0 );
#else
	DeleteCriticalSection(&mId);
#endif
}


void
Mutex::lock()
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
Mutex::unlock()
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
Mutex::getId() const
{
    return ( &mId );
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
