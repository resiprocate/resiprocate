#ifndef Vocal2_Logger_hxx
#define Vocal2_Logger_hxx

#include "sip2/util/Socket.hxx"
#include "sip2/util/Log.hxx"
#include "sip2/util/SysLogStream.hxx"
#include "sip2/util/Lock.hxx"


/**
   Defines a set of logging macros, one for each level of logging.

   Example:
#include Logger.hxx
#define VOCAL_SUBSYSTEM Vocal2::Subsystem::SIP
   ...
   DebugLog(<< "hi there " << mix << 5 << types);  // note leading << and no endl
*/

//#define VOCAL_SUBSYSTEM Vocal2::Subsystem::NONE


#if ( (__GNUC__ < 3) )

// variadic to handle comma in template arguments
#define DebugLog(arg__, args__...)  /* eat the comma if no extra arguments */ \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::DEBUG, arg__, ##args__)

#define CritLog(arg__, args__...) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::CRIT, arg__, ##args__)

#define ErrLog(arg__, args__...) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::ERR, arg__, ##args__)

#define WarningLog(arg__, args__...) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::WARNING, arg__, ##args__)

#define InfoLog(arg__, args__...) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::INFO, arg__, ##args__)

// do/while allows a {} block in an expression
#define GenericLog(system__, level__, arg__, args__...)         \
do                                                              \
{                                                               \
  if (Vocal2::GenericLogImpl::isLogging(level__))               \
  {                                                             \
     Vocal2::Lock lock(Vocal2::Log::_mutex);                    \
     if (Vocal2::GenericLogImpl::isLogging(level__))            \
     {                                                          \
        Vocal2::Log::tags(level__, system__,                    \
                          Vocal2::GenericLogImpl::Instance())   \
          << __FILE__ << ':' << __LINE__ << DELIM               \
                  /* eat the comma if no extra arguments */     \
          arg__ , ##args__ << std::endl;                        \
     }                                                          \
  }                                                             \
} while (0)


#elif ( defined(__SUNPRO_CC) || (__GNUC__ >= 3) )


// variadic to handle comma in template arguments
#define DebugLog(args__)/* eat the comma if no extra arguments */ \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::DEBUG, args__)

#define CritLog(args__) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::CRIT, args__)

#define ErrLog(args__) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::ERR, args__)

#define WarningLog(args__) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::WARNING, args__)

#define InfoLog(args__) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::INFO, args__)

// do/while allows a {} block in an expression
#define GenericLog(system__, level__, args__)         \
do                                                              \
{                                                               \
  if (Vocal2::GenericLogImpl::isLogging(level__))               \
  {                                                             \
     Vocal2::Lock lock(Vocal2::Log::_mutex);                    \
     if (Vocal2::GenericLogImpl::isLogging(level__))            \
     {                                                          \
        Vocal2::Log::tags(level__, system__,                    \
                          Vocal2::GenericLogImpl::Instance())   \
          << __FILE__ << ':' << __LINE__ << DELIM               \
                  /* eat the comma if no extra arguments */     \
          args__ << std::endl;                        \
     }                                                          \
  }                                                             \
} while (0)

#elif ( defined (WIN32) )

#define DebugLog(__VA_ARGS__) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::DEBUG, __VA_ARGS__)

#define CritLog(__VA_ARGS__) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::CRIT, __VA_ARGS__)

#define ErrLog(__VA_ARGS__) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::ERR, __VA_ARGS__)

#define WarningLog(__VA_ARGS__) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::WARNING, __VA_ARGS__)

#define InfoLog(__VA_ARGS__) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::INFO, __VA_ARGS__)

// do/while allows a {} block in an expression
#define GenericLog(system__, level__,  __VA_ARGS__ )                     \
do                                                              \
{                                                               \
  if (Vocal2::GenericLogImpl::isLogging(level__))               \
  {                                                             \
     Vocal2::Lock lock(Vocal2::Log::_mutex);                    \
     if (Vocal2::GenericLogImpl::isLogging(level__))            \
     {                                                          \
        Vocal2::Log::tags(level__, system__,                    \
                          Vocal2::GenericLogImpl::Instance())   \
          << __FILE__ << ':' << __LINE__ << DELIM               \
          __VA_ARGS__  << std::endl;                            \
     }                                                          \
  }                                                             \
} while (0)

#else

#define DebugLog(...) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::DEBUG, __VA_ARGS__ )

#define CritLog(...) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::CRIT, __VA_ARGS__ )

#define ErrLog( ...) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::ERR, __VA_ARGS__ )

#define WarningLog(...) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::WARNING, __VA_ARGS__ )

#define InfoLog(...) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::INFO, __VA_ARGS__ )

// do/while allows a {} block in an expression
#define GenericLog(system__, level__,  ...)                     \
do                                                              \
{                                                               \
  if (Vocal2::GenericLogImpl::isLogging(level__))               \
  {                                                             \
     Vocal2::Lock lock(Vocal2::Log::_mutex);                    \
     if (Vocal2::GenericLogImpl::isLogging(level__))            \
     {                                                          \
        Vocal2::Log::tags(level__, system__,                    \
                          Vocal2::GenericLogImpl::Instance())   \
          << __FILE__ << ':' << __LINE__ << DELIM               \
          __VA_ARGS__  << std::endl;                            \
     }                                                          \
  }                                                             \
} while (0)

#endif


#ifdef NO_DEBUG
// Suppress debug loging at compile time
#define DebugLog(arg__, args__...)
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
