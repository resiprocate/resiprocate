#ifndef resip_ThreadIf_hxx
#define resip_ThreadIf_hxx

#include "resiprocate/os/Socket.hxx"

#ifdef WIN32
#  include <BaseTsd.h>
#  include <winbase.h>
#else
#  include <pthread.h>
#endif

#include "resiprocate/os/Mutex.hxx"
#include "resiprocate/os/Condition.hxx"

namespace resip
{

/* A wrapper class to create and spawn a thread.  It is a base class.
   ThreadIf::thread() is a pure virtual method .
   
   <P>Usage:
   To use this class, derive from it and override the thread() method.
   To start the thread, call the run() method.  The code in thread() will
   run in a separate thread.
   
   <P>Call shutdown() from the constructing thread to shut down the
   code.  This will set the bool shutdown_ to true.  The code in
   thread() should react properly to shutdown_ being set, by
   returning.  Call join() to join the code.
   <P>Sample:
   <PRE>
   ...
   DerivedThreadIf thread;
   thread.run();
   ... do stuff ...
   thread.shutdown();
   thread.join();
   </PRE>
*/
class ThreadIf
{
   public:
      ThreadIf();
      virtual ~ThreadIf();

      // runs the code in thread() .  Returns immediately
      virtual void run();

      // joins to the thread running thread()
      virtual void join();

      // forces the thread running to exit()
      virtual void exit();

      // request the thread running thread() to return, by setting  mShutdown 
      void shutdown();

      //waits for waitMs, or stops waiting and returns true if shutdown was
      //called
      bool waitForShutdown(int ms) const;

      // returns true if the thread has been asked to shutdown or not running
      bool isShutdown() const;

#ifdef WIN32
      typedef DWORD Id;
#else
      typedef pthread_t Id;
      static Id selfId();
#endif

      /* thread is a virtual method.  Users should derive and define
        thread() such that it returns when isShutdown() is true.
      */
      virtual void thread() = 0;

   protected:
#ifdef WIN32
      HANDLE mThread;
#endif
      Id mId;
      
      bool mShutdown;
      mutable Mutex mShutdownMutex;
      mutable Condition mShutdownCondition;

   private:
      // Suppress copying
      ThreadIf(const ThreadIf &);
      const ThreadIf & operator=(const ThreadIf &);
};
 
}


#endif // THREADIF_HXX


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
