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

#include "util/Condition.hxx"
#include "util/Mutex.hxx"


using namespace Vocal2;


Condition::Condition() : mId()
{
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
	assert(0);
#else
    int  rc = pthread_cond_destroy(&mId);
    assert( rc == 0 );
#endif
}


void
Condition::wait (Mutex* mutex)
{
#ifdef WIN32 
	WaitForSingleObject(mId,INFINITE);
#else
   int ret = pthread_cond_wait(&mId, mutex->getId());
   assert( ret == 0 );
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


#if 0 
void
Condition::broadcast()
{
#ifdef WIN32
	assert(0);
#else
    return ( pthread_cond_broadcast(&mId) );
#endif
}
#endif


#if 0
const vcondition_t*
Condition::getId () const
{
    return ( &myId );
}
#endif

