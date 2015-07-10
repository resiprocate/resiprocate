#include <cstdio>
#include "rutil/ResipAssert.h"
#include "rutil/SysLogBuf.hxx"
#include "rutil/Log.hxx"

#ifndef EOF
# define EOF (-1)
#endif

using resip::SysLogBuf;

SysLogBuf::SysLogBuf ()
 : mLevel(Log::Debug),
   mAppName(""),
   mFacility(LOG_DAEMON)
{
   init();
}

SysLogBuf::SysLogBuf (const resip::Data& ident, int facility)
 : mLevel(Log::Debug),
   mAppName(ident),
   mFacility(facility)
{
   init();
}

void
SysLogBuf::init()
{
#if !defined(WIN32)
   setp(buffer,buffer+Size);
   const char* _ident = 0;
   if(!mAppName.empty())
   {
      _ident = mAppName.c_str();
   }
   openlog (_ident, LOG_NDELAY | LOG_PID, mFacility);
#endif
}
      
SysLogBuf::~SysLogBuf()
{
}
      
int 
SysLogBuf::sync()
{
#if !defined(WIN32)
   // Default to debug level for Stack, Debug and unrecognised values
   int _level = LOG_DEBUG;
   // For efficiency, we check mLevel in decreasing order of frequency,
   // anticipating that Stack is the most common and Crit is the least
   // common.
   switch(mLevel)
   {
      case Log::Stack:
      case Log::Debug:
         // This is just here to avoid traversing the rest
         // of the switch block for every Stack or Debug message.
         // They will just be logged with the default LOG_DEBUG
         // specified above.
         break;
      case Log::Info:
         _level = LOG_INFO;
         break;
      case Log::Warning:
         _level = LOG_WARNING;
         break;
      case Log::Err:
         _level = LOG_ERR;
         break;
      case Log::Crit:
         _level = LOG_CRIT;
         break;
      default:
         // just let it use the default value defined above
         break;
   }
   *(pptr()) = 0;
   syslog (mFacility | _level, "%s", pbase());
   // Set mLevel back to the default level for the next log entry
   // in case it is not explicitly specified next time.
   mLevel = Log::Debug;
   setp(buffer, buffer+Size);
#else
   resip_assert(0);
#endif
   return 0;
}
     
int 
SysLogBuf::overflow (int c)
{
   sync();
   if (c != EOF) 
   {
      *pptr() = static_cast<unsigned char>(c);
      pbump(1);
   }
   return c;
}

std::ostream& resip::operator<< (std::ostream& os, const resip::Log::Level& level)
{
   // FIXME - we should probably find a more precise way to make sure
   // that this can never be done for the wrong type of stream.
   // For now, we rely on Log.cxx checking if mLogger is of type Syslog
   // and not sending Level to the stream otherwise.
   static_cast<SysLogBuf *>(os.rdbuf())->mLevel = level;
   return os;
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
