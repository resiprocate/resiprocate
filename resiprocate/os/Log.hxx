#if !defined(RESIP_LOG_HXX)
#define RESIP_LOG_HXX 

#define DELIM ' ' << '|' << ' '

#include "resiprocate/os/Data.hxx"

#ifndef WIN32
#include <syslog.h>
#include <unistd.h>
#include <pthread.h>
#endif

#include <map>
#include <set>

#include "resiprocate/os/Subsystem.hxx"
#include "resiprocate/os/Mutex.hxx"
#include <iostream>

namespace resip
{

class Log
{
   public:
      typedef enum 
      {
         COUT = 0,
         SYSLOG, 
         FILE, 
         CERR
      } Type;
      
#ifdef WIN32 
      typedef enum 
      {
         CRIT = 1,
         ERR = 2,
         WARNING = 3,
         INFO = 4,
         DEBUG = 5,
         DEBUG_STACK = 8,
         BOGUS = 666
      } Level;
#else
      typedef enum 
      {
         CRIT = LOG_CRIT,
#ifdef ERR // ncurses defines a macro called ERR 
         SIP2_ERR = LOG_ERR,
#else
         ERR = LOG_ERR,
#endif
         WARNING = LOG_WARNING,
         INFO = LOG_INFO,
         DEBUG = LOG_DEBUG,
         DEBUG_STACK = 8,
         BOGUS = 666
      } Level;
#endif

      class ThreadSetting
      {
         public:
            ThreadSetting()
               : service(-1),
#ifdef ERR
                 level(SIP2_ERR)
#else
                 level(ERR)
#endif
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

      /// Return the loglevel, hostname, appname, pid, tid, subsystem
      static std::ostream& tags(Log::Level level, const Subsystem& subsystem, std::ostream& strm); 
      static Data timestamp();
      static void initialize(Type type, Level level, const Data& appName);
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
      static std::map<int, Level> _serviceToLevel;

#ifndef WIN32
      static std::map<pthread_t, std::pair<ThreadSetting, bool> > _threadToLevel;
      static std::map<int, std::set<pthread_t> > _serviceToThreads;
      static pthread_key_t _levelKey;
#endif
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
