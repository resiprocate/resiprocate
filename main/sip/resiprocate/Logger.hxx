#ifndef Logger_hxx
#define Logger_hxx

//#include <sipstack/Singleton.h>
//#include <sipstack/Threads.h>

#include <sipstack/Log.hxx>
#include <sipstack/SysLogStream.hxx>
#include <sipstack/Lock.hxx>

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
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::DEBUG, arg__, ##args__)

#define CritLog(arg__, args__...) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::CRIT, arg__, ##args__)

#define ErrLog(arg__, args__...) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::ERR, arg__, ##args__)

#define WarningLog(arg__, args__...) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::WARNING, arg__, ##args__)

#define InfoLog(arg__, args__...) \
GenericLog(VOCAL_SUBSYSTEM, Vocal2::Log::INFO, arg__, ##args__)

#ifdef NO_DEBUG
// Suppress debug loging at compile time
#define DebugLog(arg__, args__...)
#endif

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
         else if (Log::_type == Log::FILE)
         {
            assert(0);
         }
         else
         {
            return std::cout;
         }
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
