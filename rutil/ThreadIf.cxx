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

std::thread::id noThread;

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
   mThreadObj(nullptr),
   mId(),
   mShutdown(false),
   mShutdownMutex()
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
   resip_assert(mId == noThread);

   mThreadObj = std::make_shared<std::thread>([this]
   {
      thread();
#ifdef WIN32
      // Free data in TLS slots.
      ThreadIf::tlsDestroyAll();
#endif
   });
   mId = mThreadObj->get_id();
   return;
}

void
ThreadIf::join()
{
   // !kh!
   // perhaps assert instead of returning when join()ed already?
   // programming error?
   //assert(mId == 0);

   if (!mThreadObj)
   {
      return;
   }

   if(!mThreadObj->joinable())
   {
      return;
   }

   mThreadObj->join();

   mThreadObj.reset();
   mId = noThread;
}

void
ThreadIf::detach()
{
   mThreadObj->detach();
   mThreadObj.reset();
   mId = noThread;
}

ThreadIf::Id
ThreadIf::selfId()
{
   return std::this_thread::get_id();
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
      mShutdownCondition.notify_one();
   }
}

bool
ThreadIf::waitForShutdown(int ms) const
{
   Lock lock(mShutdownMutex);
   if(!mShutdown)
   {
      mShutdownCondition.wait_for(lock, std::chrono::milliseconds(ms));
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
