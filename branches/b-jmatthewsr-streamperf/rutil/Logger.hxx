#ifndef RESIP_Logger_hxx
#define RESIP_Logger_hxx

#include "rutil/Log.hxx"
#include "rutil/Lock.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Data.hxx"

#ifdef WIN32
#include <windows.h>
#endif 

/**
   Defines a set of logging macros, one for each level of logging.

   Example:
#include Logger.hxx
#define RESIPROCATE_SUBSYSTEM resip::Subsystem::SIP
   ...
   DebugLog(<< "hi there " << mix << 4 << types);  // note leading << and no endl
*/


#define DELIM 



// unconditionally output to cerr -- easily change back and forth
#define CerrLog(args_)                                                  \
  resip::Log::tags(resip::Log::StdErr, RESIPROCATE_SUBSYSTEM,           \
                   __FILE__, __LINE__, std::cerr) << ' ' << '|' << ' '  \
          args_ << std::endl;

#define StackLog(args_)                                                         \
GenericLog(RESIPROCATE_SUBSYSTEM, resip::Log::Stack, args_)

#define DebugLog(args_) \
GenericLog(RESIPROCATE_SUBSYSTEM, resip::Log::Debug, args_)

#define InfoLog(args_) \
GenericLog(RESIPROCATE_SUBSYSTEM, resip::Log::Info, args_)

#define WarningLog(args_) \
GenericLog(RESIPROCATE_SUBSYSTEM, resip::Log::Warning, args_)

#define ErrLog(args_) \
GenericLog(RESIPROCATE_SUBSYSTEM, resip::Log::Err, args_)

#define CritLog(args_) \
GenericLog(RESIPROCATE_SUBSYSTEM, resip::Log::Crit, args_)

bool
genericLogCheckLevel(resip::Log::Level level);

// do/while allows a {} block in an expression
#define GenericLog(system_, level_, args_)                                               \
do                                                                                       \
{                                                                                        \
   if (genericLogCheckLevel(level_))                                                     \
   {                                                                                     \
      resip::Log::Guard _resip_log_guard(level_, system_, __FILE__, __LINE__);           \
      _resip_log_guard.asStream()  args_;                                                           \
   }                                                                                     \
} while (false)

#ifdef NO_DEBUG
// Suppress debug logging at compile time
#undef DebugLog
#define DebugLog(args_)
#undef StackLog 
#define StackLog(args_)
#endif

namespace resip
{

class GenericLogImpl :  public Log 
{
   public:
	   static std::ostream& Instance();
      static bool isLogging(Log::Level level) ;
      static unsigned int MaxLineCount;
      static void OutputToWin32DebugWindow(const Data& result);      
      static void reset(); //removes mLogger
   private:
	   static std::ostream* mLogger;
      static unsigned int mLineCount;
};
 
}

#endif

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
