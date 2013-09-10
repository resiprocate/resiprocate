
#ifdef __ANDROID__

#include <android/log.h>

#include "rutil/AndroidLogger.hxx"

using namespace resip;
using namespace std;

const char *AndroidLogger::logTag = "reSIProcate";

bool
AndroidLogger::operator()(Log::Level level,
                  const Subsystem& subsystem,
                  const Data& appName,
                  const char* file,
                  int line,
                  const Data& message,
                  const Data& messageWithHeaders)
{
   android_LogPriority prio;
   switch(level)
   {
   case Log::Crit:
      prio = ANDROID_LOG_FATAL;
      break;
   case Log::Err:
      prio = ANDROID_LOG_ERROR;
      break;
   case Log::Warning:
      prio = ANDROID_LOG_WARN;
      break;
   case Log::Info:
      prio = ANDROID_LOG_INFO;
      break;
   case Log::Debug:
      prio = ANDROID_LOG_DEBUG;
      break;
   case Log::Stack:
      prio = ANDROID_LOG_VERBOSE;
      break;
   case Log::StdErr:
      prio = ANDROID_LOG_ERROR;
      break;
   default:
      prio = ANDROID_LOG_WARN;
   }

   __android_log_write(prio, logTag, messageWithHeaders.c_str());
   return true;
}


#endif

/* ====================================================================
 *
 * Copyright (c) 2007 Daniel Pocock  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

