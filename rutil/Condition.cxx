#include "rutil/ResipAssert.h"
#include <climits>

#ifndef WIN32
#  include <pthread.h>
#  include <errno.h>
#  include <sys/time.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#endif

#include "rutil/compat.hxx"
#include "rutil/Condition.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/Timer.hxx"

#ifdef _RESIP_MONOTONIC_CLOCK
#ifdef __APPLE__
#undef _RESIP_MONOTONIC_CLOCK
#warning Mac OS X does not support POSIX monotonic timers.
#endif
#endif

using namespace resip;

Condition::Condition()
{
   //std::cerr << this << " Condition::Condition" << std::endl;

#ifdef WIN32
#  ifdef RESIP_CONDITION_WIN32_CONFORMANCE_TO_POSIX
   m_blocked = 0;
   m_gone = 0;
   m_waiting = 0;
   m_gate = reinterpret_cast<void*>(CreateSemaphore(0, 1, 1, 0));
   m_queue = reinterpret_cast<void*>(CreateSemaphore(0, 0, LONG_MAX, 0));
   m_mutex = reinterpret_cast<void*>(CreateMutex(0, 0, 0));

   if (!m_gate || !m_queue || !m_mutex)
   {
      int res = 0;
      if (m_gate)
      {
         res = CloseHandle(reinterpret_cast<HANDLE>(m_gate));
         resip_assert(res);
      }
      if (m_queue)
      {
         res = CloseHandle(reinterpret_cast<HANDLE>(m_queue));
         resip_assert(res);
      }
      if (m_mutex)
      {
         res = CloseHandle(reinterpret_cast<HANDLE>(m_mutex));
         resip_assert(res);
      }

      resip_assert(0);
   }
#  else
   mId =  CreateEvent(
      NULL, //LPSECURITY_ATTRIBUTES lpEventAttributes,
      // pointer to security attributes
      FALSE, // BOOL bManualReset,  // flag for manual-reset event
      FALSE, //BOOL bInitialState, // flag for initial state
      NULL //LPCTSTR lpName      // pointer to event-object name
      );
   resip_assert(mId);
#  endif
#else
#ifdef _RESIP_MONOTONIC_CLOCK
   pthread_condattr_t attr;
   struct timespec dummy;
   int ret = pthread_condattr_init( &attr );
   resip_assert( ret == 0 );

//   if((syscall( __NR_clock_getres, CLOCK_MONOTONIC, &dummy ) == 0) &&
     if((clock_getres( CLOCK_MONOTONIC, &dummy ) == 0) &&
       (pthread_condattr_setclock( &attr, CLOCK_MONOTONIC ) == 0))
   {
      ret = pthread_cond_init( &mId, &attr );
      resip_assert( ret == 0 );
      pthread_condattr_destroy( &attr );
      return;
   }
   pthread_condattr_destroy( &attr );
#endif
   int  rc =  pthread_cond_init(&mId,0);
   (void)rc;
   resip_assert( rc == 0 );
#endif
}


Condition::~Condition ()
{
#ifdef WIN32
#  ifdef RESIP_CONDITION_WIN32_CONFORMANCE_TO_POSIX
    int res = 0;
    res = CloseHandle(reinterpret_cast<HANDLE>(m_gate));
    resip_assert(res);
    res = CloseHandle(reinterpret_cast<HANDLE>(m_queue));
    resip_assert(res);
    res = CloseHandle(reinterpret_cast<HANDLE>(m_mutex));
    resip_assert(res);
#  else
   BOOL ok = CloseHandle(mId);
   resip_assert( ok );
#  endif
#else
   if (pthread_cond_destroy(&mId) == EBUSY)
   {
      resip_assert(0);
   }
#endif
}

#if defined(WIN32) && defined(RESIP_CONDITION_WIN32_CONFORMANCE_TO_POSIX)
void
Condition::enterWait ()
{
   int res = 0;
   res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_gate), INFINITE);
   resip_assert(res == WAIT_OBJECT_0);
   ++m_blocked;
   res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0);
   resip_assert(res);
}
#endif

void
Condition::wait (Mutex& mutex)
{
   //std::cerr << "Condition::wait " << mutex << std::endl;
#ifdef WIN32
#  ifdef RESIP_CONDITION_WIN32_CONFORMANCE_TO_POSIX
   enterWait();

   // Release the mutex
   mutex.unlock();

   // do wait
   {
      int res = 0;
      res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_queue), INFINITE);
      resip_assert(res == WAIT_OBJECT_0);

      unsigned was_waiting=0;
      unsigned was_gone=0;

      res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), INFINITE);
      resip_assert(res == WAIT_OBJECT_0);
      was_waiting = m_waiting;
      was_gone = m_gone;
      if (was_waiting != 0)
      {
         if (--m_waiting == 0)
         {
            if (m_blocked != 0)
            {
               res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0); // open m_gate
               resip_assert(res);
               was_waiting = 0;
            }
            else if (m_gone != 0)
                m_gone = 0;
         }
      }
      else if (++m_gone == (ULONG_MAX / 2))
      {
         // timeout occured, normalize the m_gone count
         // this may occur if many calls to wait with a timeout are made and
         // no call to notify_* is made
         res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_gate), INFINITE);
         resip_assert(res == WAIT_OBJECT_0);
         m_blocked -= m_gone;
         res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0);
         resip_assert(res);
         m_gone = 0;
      }
      res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
      resip_assert(res);

      if (was_waiting == 1)
      {
         for (/* */ ; was_gone; --was_gone)
         {
            // better now than spurious later
            res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_queue),
                  INFINITE);
            resip_assert(res == WAIT_OBJECT_0);
         }
         res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0);
         resip_assert(res);
      }
   }

   // Reacquire the mutex
   mutex.lock();

#   else
   // FixMe: Race condition between time we get mId and when we
   // re-acquire the mutex.
   mutex.unlock();
   WaitForSingleObject(mId,INFINITE);
   mutex.lock();
#   endif
#else
   int ret = pthread_cond_wait(&mId, mutex.getId());
   (void)ret;
   resip_assert( ret == 0 );
#endif
}

void
Condition::wait (Mutex* mutex)
{
   this->wait(*mutex);
}

bool
Condition::wait(Mutex& mutex, 
                unsigned int ms)
{
   if (ms == 0)
   {
      wait(mutex);
      return true;
   }

#ifdef WIN32
#   ifdef RESIP_CONDITION_WIN32_CONFORMANCE_TO_POSIX
   enterWait();

   // Release the mutex
   mutex.unlock();

   //  do timed wait
   bool ret = false;
   unsigned int res = 0;

#if 0  /*  unnecessary time stuff - used in BOOST implementation because expiry time is provided to do_timed_wait - we pass in an interval */
   UInt64  start = Timer::getTimeMs();

   for (;;)
   {
       res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_queue),
             ms);
       resip_assert(res != WAIT_FAILED && res != WAIT_ABANDONED);
       ret = (res == WAIT_OBJECT_0);
       if (res == WAIT_TIMEOUT)
       {
          UInt64  now = Timer::getTimeMs();
          unsigned int elapsed = (unsigned int)(now - start);
          if (ms > elapsed)
          {
             ms -= elapsed;
             continue;
          }
       }

       break;
   }
#endif

   res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_queue),ms);
   resip_assert(res != WAIT_FAILED && res != WAIT_ABANDONED);
   ret = (res == WAIT_OBJECT_0);

   unsigned was_waiting=0;
   unsigned was_gone=0;

   res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), INFINITE);
   resip_assert(res == WAIT_OBJECT_0);
   was_waiting = m_waiting;
   was_gone = m_gone;
   if (was_waiting != 0)
   {
      if (!ret) // timeout
      {
         if (m_blocked != 0)
            --m_blocked;
         else
            ++m_gone; // count spurious wakeups
      }
      if (--m_waiting == 0)
      {
         if (m_blocked != 0)
         {
            res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0); // open m_gate
            resip_assert(res);
            was_waiting = 0;
         }
         else if (m_gone != 0)
            m_gone = 0;
      }
   }
   else if (++m_gone == (ULONG_MAX / 2))
   {
      // timeout occured, normalize the m_gone count
      // this may occur if many calls to wait with a timeout are made and
      // no call to notify_* is made
      res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_gate), INFINITE);
      resip_assert(res == WAIT_OBJECT_0);
      m_blocked -= m_gone;
      res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0);
      resip_assert(res);
      m_gone = 0;
   }
   res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
   resip_assert(res);

   if (was_waiting == 1)
   {
      for (/* */ ; was_gone; --was_gone)
      {
         // better now than spurious later
         res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_queue), INFINITE);
         resip_assert(res ==  WAIT_OBJECT_0);
      }
      res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0);
      resip_assert(res);
   }

   // Reacquire the mutex
   mutex.lock();

   return ret;

#   else
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
   mutex.unlock();
   DWORD ret = WaitForSingleObject(mId, ms);
   mutex.lock();
   resip_assert(ret != WAIT_FAILED);
   return (ret == WAIT_OBJECT_0);
#   endif
#else	// WIN32
   UInt64 expires64 = Timer::getTimeMs() + ms;
   timespec expiresTS;
   expiresTS.tv_sec = expires64 / 1000;
   expiresTS.tv_nsec = (expires64 % 1000) * 1000000L;

   resip_assert( expiresTS.tv_nsec < 1000000000L );

   //std::cerr << "Condition::wait " << mutex << "ms=" << ms << " expire=" << expiresTS.tv_sec << " " << expiresTS.tv_nsec << std::endl;
   int ret = pthread_cond_timedwait(&mId, mutex.getId(), &expiresTS);

   if (ret == EINTR || ret == ETIMEDOUT)
   {
      return false;
   }
   else
   {
      //std::cerr << this << " pthread_cond_timedwait failed " << ret << " mutex=" << mutex << std::endl;
      (void)ret;
      resip_assert( ret == 0 );
      return true;
   }
#endif	// not WIN32
}

bool
Condition::wait (Mutex* mutex, unsigned int ms)
{
   return this->wait(*mutex, ms);
}

void
Condition::signal ()
{
#ifdef WIN32
#  ifdef RESIP_CONDITION_WIN32_CONFORMANCE_TO_POSIX
    unsigned signals = 0;

   int res = 0;
   res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), INFINITE);
   resip_assert(res == WAIT_OBJECT_0);

   if (m_waiting != 0) // the m_gate is already closed
   {
      if (m_blocked == 0)
      {
         res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
         resip_assert(res);
         return;
      }

      ++m_waiting;
      --m_blocked;
      signals = 1;
   }
   else
   {
      res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_gate), INFINITE);
      resip_assert(res == WAIT_OBJECT_0);
      if (m_blocked > m_gone)
      {
         if (m_gone != 0)
         {
            m_blocked -= m_gone;
            m_gone = 0;
         }
         signals = m_waiting = 1;
         --m_blocked;
      }
      else
      {
         res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0);
         resip_assert(res);
      }
   }

   res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
   resip_assert(res);

   if (signals)
   {
      res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_queue), signals, 0);
      resip_assert(res);
   }
#  else
   BOOL ret = SetEvent(
      mId // HANDLE hEvent   // handle to event object
      );
   resip_assert(ret);
#  endif
#else
   int ret = pthread_cond_signal(&mId);
   (void)ret;
   resip_assert( ret == 0 );
#endif
}


void
Condition::broadcast()
{
#ifdef WIN32
#  ifdef RESIP_CONDITION_WIN32_CONFORMANCE_TO_POSIX
   unsigned signals = 0;

   int res = 0;
   res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), INFINITE);
   resip_assert(res == WAIT_OBJECT_0);

   if (m_waiting != 0) // the m_gate is already closed
   {
      if (m_blocked == 0)
      {
         res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
         resip_assert(res);
         return;
      }

      m_waiting += (signals = m_blocked);
      m_blocked = 0;
   }
   else
   {
      res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_gate), INFINITE);
      resip_assert(res == WAIT_OBJECT_0);
      if (m_blocked > m_gone)
      {
         if (m_gone != 0)
         {
            m_blocked -= m_gone;
            m_gone = 0;
         }
         signals = m_waiting = m_blocked;
         m_blocked = 0;
      }
      else
      {
         res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0);
         resip_assert(res);
      }
   }

   res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
   resip_assert(res);

   if (signals)
   {
      res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_queue), signals, 0);
      resip_assert(res);
   }
#  else
   resip_assert(0);
#  endif
#else
   pthread_cond_broadcast(&mId);
#endif
}

/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
