#ifndef Log_hxx
#define Log_hxx

#define DELIM " | "

#include <util/Data.hxx>

#ifndef WIN32
#include <syslog.h>
#include <unistd.h>
#endif

#include <util/Subsystem.hxx>
#include <util/Mutex.hxx>
#include <iostream>


namespace Vocal2
{

class Log
{
   public:
      typedef enum 
      {
         COUT=0,
         SYSLOG, 
         FILE,
      }Type;
      
      
#if WIN32
  typedef enum 
      {
         CRIT = 1,
         ERR = 2,
         WARNING = 3,
         INFO = 4,
         DEBUG = 5,
         DEBUG_STACK = 8,
      }Level;
#else
      typedef enum 
      {
         CRIT = LOG_CRIT,
         ERR = LOG_ERR,
         WARNING = LOG_WARNING,
         INFO = LOG_INFO,
         DEBUG = LOG_DEBUG,
         DEBUG_STACK = 8,
      }Level;
#endif

      /// Return the loglevel, hostname, appname, pid, tid, subsystem
      static std::ostream& tags(Log::Level level, const Subsystem& subsystem, std::ostream& strm); 
      static Data timestamp();
      static void initialize(Type type, Level level, const Data& appName);
      static void setLevel(Level level);
      static Level level() { return _level; }
      static Level toLevel(const Data& l);
      static Data toString(Level l);
      static Mutex _mutex;

   protected:
      static Level _level;
      static Type _type;
      static Data _appName;
      static Data _hostname;
#ifndef WIN32
      static pid_t _pid;
#else   
      static int _pid;
#endif
      static const char _descriptions[][32];
};

} // namespace Vocal2

#endif
