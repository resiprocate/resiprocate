// Copyright 2002 Cathay Networks, Inc. 

#ifndef Logger_hxx
#define Logger_hxx

#include <loki/Singleton.h>
#include <loki/Threads.h>
//#include <loki/static_check.h>


#include <sip2/Log.hxx>
#include <sip2/SysLogStream.hxx>
#include <sip2/Lock.hxx>

/**
   Defines a set of logging macros, one for each level of logging.

   Example:
#include Logger.hxx
#define VOCAL_SUBSYSTEM Vocal2::Subsystem::SIP
   ...
   DebugLog(<< "hi there " << mix << 5 << types);  // note leading << and no endl
*/

//#define VOCAL_SUBSYSTEM Vocal2::Subsystem::NONE

// variadic to handle comma in template arguments
#define DebugLog(arg__, args__...)                                                 \
                                         /* eat the comma if no extra arguments */ \
GenericLog(VOCAL_SUBSYSTEM, Vocal::Log::DEBUG, arg__, ##args__)

#define CritLog(arg__, args__...) \
GenericLog(VOCAL_SUBSYSTEM, Vocal::Log::CRIT, arg__, ##args__)

#define ErrLog(arg__, args__...) \
GenericLog(VOCAL_SUBSYSTEM, Vocal::Log::ERR, arg__, ##args__)

#define WarningLog(arg__, args__...) \
GenericLog(VOCAL_SUBSYSTEM, Vocal::Log::WARNING, arg__, ##args__)

#define InfoLog(arg__, args__...) \
GenericLog(VOCAL_SUBSYSTEM, Vocal::Log::INFO, arg__, ##args__)

#ifdef NO_DEBUG
// Suppress debug loging at compile time
#define DebugLog(arg__, args__...)
#endif

// do/while allows a {} block in an expression
#define GenericLog(system__, level__, arg__, args__...)          \
do                                                               \
{                                                                \
  if (Vocal::GenericLogImpl::isLogging(level__))                 \
  {                                                              \
     Vocal::Threads::Lock lock(Vocal::Log::_mutex);              \
     if (Vocal::GenericLogImpl::isLogging(level__))              \
     {                                                           \
        Vocal::Log::tags(level__, system__,                      \
                          Vocal::GenericLogImpl::Instance())     \
          << __FILE__ << ':' << __LINE__ << DELIM                \
                  /* eat the comma if no extra arguments */      \
          arg__ , ##args__ << endl;                              \
     }                                                           \
  }                                                              \
} while (0)

namespace Vocal2
{

class GenericLogImpl : public Loki::SingletonHolder <SysLogStream,
                                                     Loki::CreateUsingNew,
                                                     Loki::PhoenixSingleton,
                                                     Loki::ClassLevelLockable>,
                       public Log 
{
   public:
      static ostream& Instance()
      {
         if (Log::_type == Log::SYSLOG)
         {
            return Loki::SingletonHolder<SysLogStream, Loki::CreateUsingNew, 
                                         Loki::PhoenixSingleton, Loki::ClassLevelLockable>::Instance();
         }
         else if (Log::_type == Log::FILE)
         {
            assert(0);
         }
         else
         {
            return cout;
         }
      }
      
      static bool isLogging(Log::Level level) 
      {
         return (level <= Log::_level);
      }
};
 
} // namespace Vocal

#endif
