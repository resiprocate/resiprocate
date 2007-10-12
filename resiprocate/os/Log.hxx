#ifndef RESIP_Log_hx
#define RESIP_Log_hx

#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Win32Export.hxx"

#ifndef WIN32
#include <syslog.h>
#include <unistd.h>
#include <pthread.h>
#endif

#include <set>

#include "resiprocate/os/Subsystem.hxx"
#include "resiprocate/os/Mutex.hxx"
#include "resiprocate/os/HashMap.hxx"
#include <iostream>

namespace resip
{

class ExternalLogger;

class RESIP_API Log
{
   public:
      typedef enum 
      {
         Cout = 0,
         Syslog, 
         File, 
         Cerr,
         VSDebugWindow = 1 << 7,   // Use only for Visual Studio Debug Window logging - WIN32 must be defined
      } Type;
      
      typedef enum 
      {
         None = -1,
#ifdef WIN32
         Crit = 2,
         Err = 3,
         Warning = 4,
         Info = 6,
         Debug = 7,
#else
         Crit = LOG_CRIT,
// #ifdef ERR // ncurses defines a macro called ERR 
//          SIP2_ERR = LOG_ERR,
// #else
//          ERR = LOG_ERR,
// #endif
         Err,
         Warning = LOG_WARNING,
         Info = LOG_INFO,
         Debug = LOG_DEBUG,
#endif
         Stack = 8,
         StdErr = 9,
         Bogus = 666
      } Level;

      /**
	 Implementation for logging macros.
	 Log::Guard(Log::Info, Subsystem::TEST, __FILE__, __LINE__) << ... ;
      */
      class RESIP_API Guard
      {
	 public:
	    /** Remember the logging values and be a a stream to receive
		the log contents. */
	    Guard(Level level,
		  const Subsystem& system,
		  const char* file,
		  int line);

	    /** Commit logging */
	    ~Guard();

	    std::ostream& asStream() {return mStream;}
	    operator std::ostream&() {return mStream;}
	    
	 private:
	    resip::Log::Level mLevel;
	    resip::Subsystem mSubsystem;
	    resip::Data::size_type mHeaderLength;
	    const char* mFile;
	    int mLine;
	    char mBuffer[128];
	    Data mData;
	    oDataStream mStream;
	    Guard& operator=(const Guard&);
      };

      class RESIP_API ThreadSetting
      {
         public:
            ThreadSetting()
               : service(-1),
               level(Err)
            {}

            ThreadSetting(int serv, Level l)
               : service(serv),
                 level(l)
            {}

            ThreadSetting(const ThreadSetting& rhs)
               : service(rhs.service),
                 level(rhs.level)
            {}
            
            int service;
            Level level;
      };

      /// output the loglevel, hostname, appname, pid, tid, subsystem
      static std::ostream& tags(Log::Level level, 
                                const Subsystem& subsystem, 
                                const char* file,
                                int line,
                                std::ostream& strm);
      /// output the loglevel, hostname, appname, pid, tid, subsystem
      static Data& tags(Log::Level level, 
                        const Subsystem& subsystem, 
                        const char* file,
                        int line,
                        Data& out);
      static Data& timestamp(Data& result);
      static Data timestamp();
      static ExternalLogger* getExternal()
      {
         return _externalLogger;
      }
      static Data getAppName()
      {
         return _appName;
      }

      static void initialize(Type type,
                             Level level,
                             const Data& appName,
                             const char * logFileName = 0,
                             ExternalLogger* externalLogger = 0);
      static void initialize(const Data& type,
                             const Data& level,
                             const Data& appName,
                             const char * logFileName = 0);
      static void initialize(const char* type,
                             const char* level,
                             const char* appName,
                             const char * logFileName = 0);
      static void initialize(Type type,
                             Level level,
                             const Data& appName,
                             ExternalLogger& logger);

      static void setLevel(Level level);
      static Level level() { return _level; }
      static Level toLevel(const Data& l);
      static Type toType(const Data& t);
      static Data toString(Level l);
      static Mutex _mutex;

      static void setServiceLevel(int service, Level l);
      static Level getServiceLevel(int service);

      static const ThreadSetting* getThreadSetting();
      static void setThreadSetting(ThreadSetting info);
      static void setThreadSetting(int serv, Level l);
      static void setThreadSetting(int serv);
      static volatile short touchCount;
      static int _type; //(enum Type)
      static const Data delim;
   protected:
      static Level _level;
      static Data _appName;
      static Data _hostname;
      static Data _logFileName;
      static ExternalLogger* _externalLogger;
#ifndef WIN32
      static pid_t _pid;
#else   
      static int _pid;
#endif
      static const char _descriptions[][32];
      static HashMap<int, Level> _serviceToLevel;

#ifndef WIN32
      static HashMap<pthread_t, std::pair<ThreadSetting, bool> > _threadToLevel;
      static HashMap<int, std::set<pthread_t> > _serviceToThreads;
      static pthread_key_t _levelKey;
#endif
      ExternalLogger* mExternalLogger;
};

/** Interface functor for external logging. */
class RESIP_API ExternalLogger
{
   public:
      /** return true to also do default logging, false to supress default logging. */
      virtual bool operator()(Log::Level level,
                              const Subsystem& subsystem, 
                              const Data& appName,
                              const char* file,
                              int line,
                              const Data& message,
			      const Data& messageWithHeaders) = 0;
};

}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
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
