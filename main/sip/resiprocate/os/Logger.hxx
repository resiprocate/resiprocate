#ifndef Vocal2_Logger_hxx
#define Vocal2_Logger_hxx

#include "resiprocate/util/Socket.hxx"
#include "resiprocate/util/Log.hxx"
#include "resiprocate/util/SysLogStream.hxx"
#include "resiprocate/util/Lock.hxx"

/**
   Defines a set of logging macros, one for each level of logging.

   Example:
#include Logger.hxx
#define VOCAL_SUBSYSTEM Vocal2::Subsystem::SIP
   ...
   DebugLog(<< "hi there " << mix << 4 << types);  // note leading << and no endl
*/

// unconditionally output to cerr -- easily change back and forth
#define CerrLog(args_)                                                          \
  Vocal2::Log::tags(Vocal2::Log::DEBUG_STACK, VOCAL_SUBSYSTEM, std::cerr)       \
          << __FILE__ << ':' << __LINE__ << DELIM                               \
          args_ << std::endl;

#define DebugLog(args_) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::DEBUG, args_)

#define CritLog(args_) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::CRIT, args_)

#define ErrLog(args_) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::ERR, args_)

#define WarningLog(args_) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::WARNING, args_)

#define InfoLog(args_) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::INFO, args_)

#define CHECK_RECURSIVE_LOG
class AssertOnRecursiveLock
{
   public:
      AssertOnRecursiveLock();
      void set();
      ~AssertOnRecursiveLock();
   private:
      // no object semantics
      AssertOnRecursiveLock(const AssertOnRecursiveLock &);
      const AssertOnRecursiveLock & operator=(const AssertOnRecursiveLock &);
};

// do/while allows a {} block in an expression
#define GenericLog(system_, level_, args_)                                      \
do                                                                              \
{                                                                               \
   const Vocal2::Log::ThreadSetting* setting = Vocal2::Log::getThreadSetting(); \
   if (setting)                                                                 \
   {                                                                            \
      if (level_ <= setting->level)                                             \
      {                                                                         \
         AssertOnRecursiveLock check;                                           \
         Vocal2::Lock lock(Vocal2::Log::_mutex);                                \
         check.set();                                                           \
         Vocal2::Log::tags(level_, system_,                                     \
                           Vocal2::GenericLogImpl::Instance())                  \
                              << __FILE__ << ':' << __LINE__ << DELIM           \
            args_ << std::endl;                                                 \
      }                                                                         \
   }                                                                            \
   else                                                                         \
   {                                                                            \
      if (Vocal2::GenericLogImpl::isLogging(level_))                            \
      {                                                                         \
         AssertOnRecursiveLock check;                                           \
         Vocal2::Lock lock(Vocal2::Log::_mutex);                                \
         check.set();                                                           \
         if (Vocal2::GenericLogImpl::isLogging(level_))                         \
         {                                                                      \
            Vocal2::Log::tags(level_, system_,                                  \
                              Vocal2::GenericLogImpl::Instance())               \
                                 << __FILE__ << ':' << __LINE__ << DELIM        \
               args_ << std::endl;                                              \
         }                                                                      \
      }                                                                         \
   }                                                                            \
} while (0)

#ifdef NO_DEBUG
#undefine DebugLog
// Suppress debug logging at compile time
#define DebugLog(args_)
#endif

namespace Vocal2
{

class GenericLogImpl :  public Log 
{
   public:
      static std::ostream& Instance()
      {
         if (Log::_type == Log::SYSLOG)
         {
            if (mLogger == 0)
            {
               std::cerr << "Creating a syslog stream" << std::endl;
               mLogger = new SysLogStream;
            }
            return *mLogger;
         }
         else 
         {
            if (Log::_type == Log::FILE)
            {
               assert(0);
            }
         }

         return std::cout;
      }
      
      static bool isLogging(Log::Level level) 
      {
         return (level <= Log::_level);
      }

   private:
      static std::ostream* mLogger;
};
 
} // namespace Vocal

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
