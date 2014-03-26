
#include <os/OsIntTypes.h>
#include <os/OsSysLog.h>

#include "rutil/Logger.hxx"

#include "SipXHelper.hxx"

#include "ReconSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

using namespace resip;
using namespace recon;

//
// There is one little annoyance with this: in the reSIProcate logs, the source file
// and line number of the log entry will always be this file rather than the
// name of the file in the sipXtapi source tree where the log message originated
//
void
sipXlogHandler(const char* szPriority, const char* szSource, const char* szMsg)
{
   // we avoid parsing debug messages if not explicitly required to do so
   if(szPriority[0] == 'D' && Log::level() < Log::Debug)
   {
      return;
   }

   // The log string from sipX includes all the different tags, we have to split them out again
   UtlString date, eventCount, facility, priority, hostname, taskname, taskId, processId, content;
   OsSysLog::parseLogString(szMsg, date, eventCount, facility, priority, hostname, taskname, taskId, processId, content);

   switch(szPriority[0])
   {
      case 'D':
         DebugLog(<< szSource << ":" << taskname << ":" << taskId << ": " << content);
         break;
      case 'I':
      case 'N':
         InfoLog(<< szSource << ":" << taskname << ":" << taskId << ": " << content);
         break;
      case 'W':
         WarningLog(<< szSource << ":" << taskname << ":" << taskId << ": " << content);
         break;
      default:
         ErrLog(<< szSource << ":" << taskname << ":" << taskId << ": " << content);
   }
}


void
SipXHelper::setupLoggingBridge(const Data& appName)
{
   OsSysLog::initialize(0, appName.c_str());
   OsSysLog::setCallbackFunction(sipXlogHandler);
   // Enable all logging types, it will be filtered by the reSIProcate logger
   OsSysLog::setLoggingPriority(PRI_DEBUG);
}

/* ====================================================================
 *
 * Copyright 2014 Daniel Pocock http://danielpocock.com  All rights reserved.
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

