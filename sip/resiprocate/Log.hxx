#ifndef Log_hxx
#define Log_hxx

#define DELIM " | "

#include <sipstack/Data.hxx>
#include <unistd.h>
#include <syslog.h>
#include <sipstack/Subsystem.hxx>
#include <sipstack/Mutex.hxx>
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
      
      
      typedef enum 
      {
         CRIT = LOG_CRIT,
         ERR = LOG_ERR,
         WARNING = LOG_WARNING,
         INFO = LOG_INFO,
         DEBUG = LOG_DEBUG,
         DEBUG_STACK = 8,
      }Level;

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
      static pid_t _pid;
      static const char _descriptions[][32];
};

} // namespace Vocal2

#endif
