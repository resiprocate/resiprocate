#include <cassert>

#include <util/ThreadIf.hxx>
#include <util/Lock.hxx>


static const char* const ThreadIf_cxx_Version = "$Id: ThreadIf.cxx,v 1.2 2002/09/28 16:41:18 fluffy Exp $";

using namespace Vocal2;

static void*
threadWrapper( void* threadParm )
{
   assert( threadParm );
   ThreadIf* t = static_cast < ThreadIf* > ( threadParm );
   
   assert( t );
   t->thread();
   return 0;
}

ThreadIf::ThreadIf() : mId(0), mShutdown(false)
{
}


ThreadIf::~ThreadIf()
{
}

void
ThreadIf::run()
{
   // spawn the thread
   pthread_attr_t attributes;
   pthread_create( &mId, &attributes, threadWrapper, this);
}

void
ThreadIf::join()
{
   assert (mId != 0);
   pthread_join(mId, 0);
   mId = 0;
}

void
ThreadIf::exit()
{
   assert(mId != 0);
   pthread_cancel(mId);
   mId = 0;
}

pthread_t
ThreadIf::selfId() const
{
   return pthread_self();
}

void
ThreadIf::shutdown()
{
   Lock lock(mShutdownMutex);
   (void)lock;
   mShutdown = true;
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

