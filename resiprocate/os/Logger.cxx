#include <ostream>
#include <fstream>
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/ThreadIf.hxx"
#include "resiprocate/os/SysLogStream.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"

using namespace resip;

std::ostream* GenericLogImpl::mLogger=0;
unsigned int GenericLogImpl::mLineCount=0;
unsigned int GenericLogImpl::MaxLineCount = 0; // no limit by default

#ifdef WIN32
Data* GenericLogImpl::mWin32DebugData = 0;
DataStream* GenericLogImpl::mWin32DebugStream = 0;
#endif

std::ostream& 
GenericLogImpl::Instance()
{
   switch (Log::_type)
   {
      case Log::Syslog:
         if (mLogger == 0)
         {
            std::cerr << "Creating a syslog stream" << std::endl;
            mLogger = new SysLogStream;
         }
         return *mLogger;
               
      case Log::Cerr:
         return std::cerr;
               
      case Log::Cout:
         return std::cout;

#ifdef WIN32
      case Log::VSDebugWindow:
         if (mWin32DebugStream == 0)
         {
            mWin32DebugData = new Data();
            mWin32DebugStream = new DataStream(*mWin32DebugData);
         }
         return *mWin32DebugStream;
#endif
               
      case Log::File:
         if (mLogger == 0 || (MaxLineCount && mLineCount > MaxLineCount))
         {
            std::cerr << "Creating a file logger" << std::endl;
            if (Log::_logFileName != "")
            {
               mLogger = new std::ofstream(_logFileName.c_str(), std::ios_base::out | std::ios_base::trunc);
               mLineCount = 0;
            }
            else
            {
               mLogger = new std::ofstream("resiprocate.log", std::ios_base::out | std::ios_base::trunc);
               mLineCount = 0;
            }
         }
         mLineCount++;
         return *mLogger;
      default:
         assert(0);
         return std::cout;
   }
}

bool 
GenericLogImpl::isLogging(Log::Level level) 
{
   return (level <= Log::_level);
}

// xkd-2004-11-8
void
GenericLogImpl::OutputToWin32DebugWindow()
{
#ifdef WIN32
   mWin32DebugStream->flush();
   const char *text = mWin32DebugData->c_str();
   OutputDebugStringA(text);
   delete mWin32DebugData;
   delete mWin32DebugStream;
   mWin32DebugData = 0;
   mWin32DebugStream = 0;
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
