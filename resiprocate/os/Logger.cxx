#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/ThreadIf.hxx"

using namespace resip;

std::ostream* 
GenericLogImpl::mLogger=0;

struct LogState
{
      LogState()
         : id(0),
           count(0)
      {}

      Mutex mutex;
      ThreadIf::Id id;
      bool isDebug;
      int count;
};

static LogState logState;

bool
checkAndSetLogState(Log::Level level)
{
   Lock lock(logState.mutex);

   // first lock
   if (logState.count == 0 ||
       logState.id != ThreadIf::selfId())
   {
      Log::_mutex.lock();
      logState.id = ThreadIf::selfId();
      logState.isDebug = (level >= Log::DEBUG);
      logState.count = 1;
      return true;
   }

   // recursive lock
   logState.count++;
   if (level >= Log::DEBUG)
   {
      // nested debug logs permitted
      return true;
   }
   else
   {
      // nested non-debug logs not permitted
      if (!logState.isDebug)
      {
         return false;
      }
      else
      {
         // note that lock is non-debug
         logState.isDebug = false;
      }
      return true;
   }
}

RecursiveLogLock::RecursiveLogLock(Log::Level level)
{
#ifdef WIN32
#else
   assert(checkAndSetLogState(level));
#endif
}

RecursiveLogLock::~RecursiveLogLock()
{
#ifdef WIN32
#else
   Lock lock(logState.mutex);

   assert(logState.count);

   logState.count--;

   if (logState.count == 0)
   {
      Log::_mutex.unlock();
   }
#endif
}

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
