#include "resiprocate/os/Socket.hxx"

#include <cassert>
#include <iostream>
#include <stdio.h>
#include "resiprocate/os/Data.hxx"

#ifndef WIN32
#include <sys/time.h>
#endif

#include <sys/types.h>
#include <time.h>

#include "resiprocate/os/Log.hxx"
#include "resiprocate/os/Lock.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;
using namespace std;

const Data Log::delim(" | ");
Log::Level Log::_level = Log::Info;
Log::Type Log::_type = Cout;
Data Log::_appName;
Data Log::_hostname;
Data Log::_logFileName;
ExternalLogger* Log::_externalLogger = 0;

#ifdef WIN32
	int Log::_pid=0;
#else 
	pid_t Log::_pid=0;
#endif


volatile short Log::touchCount = 0;

#ifndef WIN32
pthread_key_t Log::_levelKey;
HashMap<pthread_t, std::pair<Log::ThreadSetting, bool> > Log::_threadToLevel;
HashMap<int, std::set<pthread_t> > Log::_serviceToThreads;
#endif

HashMap<int, Log::Level> Log::_serviceToLevel;

const char
Log::_descriptions[][32] = {"NONE", "EMERG", "ALERT", "CRIT", "ERR", "WARNING", "NOTICE", "INFO", "DEBUG", "STACK", "CERR", ""}; 

Mutex Log::_mutex;

extern "C"
{
   void freeThreadSetting(void* setting)
   {
      delete static_cast<Log::ThreadSetting*>(setting);
   }
}

void
Log::initialize(const char* typed, const char* leveld, const char* appName, const char *logFileName)
{
   Log::initialize(Data(typed), Data(leveld), Data(appName), logFileName);
}

void
Log::initialize(const Data& typed, const Data& leveld, const Data& appName, 
                const char *logFileName)
{
   Type type = Log::Cout;
   if (isEqualNoCase(typed, "cout")) type = Log::Cout;
   else if (isEqualNoCase(typed, "cerr")) type = Log::Cerr;
   else if (isEqualNoCase(typed, "file")) type = Log::File;
   else type = Log::Syslog;
   
   Level level = Log::Info;
   level = toLevel(leveld);

   Log::initialize(type, level, appName, logFileName);
}

void 
Log::initialize(Type type, Level level, const Data& appName, 
                const char * logFileName,
                ExternalLogger* externalLogger)
{
   string copy(appName.c_str());
   
   _type = type;
   _level = level;

   if (logFileName)
   {
      _logFileName = logFileName;
   }
    _externalLogger = externalLogger;

   string::size_type pos = copy.find_last_of('/');
   if ( pos == string::npos || pos == copy.size())
   {
      _appName = appName;
   }
   else
   {
      _appName = Data(copy.substr(pos+1).c_str());
   }
 
   char buffer[1024];  
   gethostname(buffer, sizeof(buffer));
   _hostname = buffer;
#ifdef WIN32 
   _pid = (int)GetCurrentProcess();
#else
   _pid = getpid();
#endif
   
#ifndef WIN32
   pthread_key_create(&Log::_levelKey, freeThreadSetting);
#endif
}

void
Log::initialize(Type type,
                Level level,
                const Data& appName,
                ExternalLogger& logger)
{
   initialize(type, level, appName, 0, &logger);
}

void
Log::setLevel(Level level)
{
   Lock lock(_mutex);
   _level = level; 
}

const static Data log_("LOG_");

Data
Log::toString(Level l)
{
   return log_ + _descriptions[l+1];
}

Log::Level
Log::toLevel(const Data& l)
{
   string pri = l.c_str();
   if (pri.find("LOG_", 0) == 0)
   {
      pri.erase(0, 4);
   }
   
   int i=0;
   while (string(_descriptions[i]).size())
   {
      if (pri == string(_descriptions[i])) 
      {
         return Level(i-1);
      }
      i++;
   }

   cerr << "Choosing Debug level since string was not understood: " << l << endl;
   return Log::Debug;
}

Log::Type
Log::toType(const Data& arg)
{
   if (arg == "cout" || arg == "COUT")
   {
      return Log::Cout;
   }
   else if (arg == "cerr" || arg == "CERR")
   {
      return Log::Cerr;
   }
   else if (arg == "file" || arg == "FILE")
   {
      return Log::File;
   }
   else
   {
      return Log::Syslog;
   }
}

ostream&
Log::tags(Log::Level level, 
          const Subsystem& subsystem, 
          const char* pfile,
          int line,
          ostream& strm) 
{
#if defined( __APPLE__ )
   strm << _descriptions[level+1] << Log::delim 
        << time(0) << Log::delim 
        << _appName << Log::delim
        << subsystem << Log::delim
        << pfile << ":" << line;
#else   
   char buffer[256] = { 0 };
   Data tstamp(Data::Borrow, buffer, sizeof(buffer) - 1);
#if defined( WIN32 )
   const char* file = pfile + strlen(pfile);
   while (file != pfile &&
          *file != '\\')
   {
      --file;
   }
   if (file != pfile)
   {
      ++file;
   }
   strm << _descriptions[level+1] << Log::delim
        << timestamp(tstamp) << Log::delim  
        << _appName << Log::delim
        << subsystem << Log::delim 
        << GetCurrentThreadId() << Log::delim
        << file << ":" << line;
#else
   strm << _descriptions[level+1] << Log::delim
        << timestamp(tstamp) << Log::delim  
        << _hostname << Log::delim  
        << _appName << Log::delim
        << subsystem << Log::delim 
        << _pid << Log::delim
        << pthread_self() << Log::delim
        << pfile << ":" << line;
#endif // #if defined( WIN32 ) 
#endif // #if defined( __APPLE__ )
  return strm;
}

Data&
Log::tags(Log::Level level, 
          const Subsystem& subsystem, 
          const char* pfile,
          int line,
          Data& out) 
{
   char obuffer[128];
   
#if defined( __APPLE__ )
   out += _descriptions[level+1] ;
   out += Log::delim;
   sprintf(obuffer, "%d", time(0));
   out += obuffer;
   out += Log::delim;
   out += _appName;
   out += Log::delim;
   out += subsystem;
   out += Log::delim;
   out += pfile;
   out += ':';
   sprintf(obuffer, "%d", line);
   out += obuffer;
#else   
   char buffer[256] = {0};
   Data tstamp(Data::Borrow, buffer, sizeof(buffer) - 1);
#if defined( WIN32 )
   const char* file = pfile + strlen(pfile);
   while (file != pfile &&
          *file != '\\')
   {
      --file;
   }
   if (file != pfile)
   {
      ++file;
   }
   out += _descriptions[level+1];
   out += Log::delim;
   out += timestamp(tstamp);
   out += Log::delim;
   out += _appName;
   out += Log::delim;
   out += subsystem.getSubsystem();
   out += Log::delim;
   sprintf(obuffer, "%d", GetCurrentThreadId());
   out += obuffer;
   out += Log::delim;
   out += file;
   out += ':';
   sprintf(obuffer, "%d", line);
   out += obuffer;
#else
   out += _descriptions[level+1];
   out += Log::delim;
   out += timestamp(tstamp);
   out += Log::delim;
   out += _hostname;
   out += Log::delim;
   out += _appName;
   out += Log::delim;
   out += subsystem.getSubsystem();
   out += Log::delim;
   sprintf(obuffer, "%d", _pid);
   out += buffer;
   out += Log::delim;
   sprintf(obuffer, "%u", pthread_self()); // .dlb. dicey - consider Data::from();
   out += obuffer;
   out += Log::delim;
   out += pfile;
   out += ':';
   sprintf(obuffer, "%d", line);
   out += obuffer;
#endif // #if defined( WIN32 ) 
#endif // #if defined( __APPLE__ )
   return out;
}

Data
Log::timestamp()
{
   char buffer[256] = { 0 };
   Data result(Data::Borrow, buffer, sizeof(buffer) - 1);
   return timestamp(result);
}

Data&
Log::timestamp(Data& res) 
{
   res.reserve(255);
   char* datebuf = const_cast<char*>(res.data());
   const unsigned int datebufSize = 255;
   res.clear();
   
#ifdef WIN32 
   int result = 1; 
   SYSTEMTIME systemTime;
   struct timeval tv = {0,0};
   time(&tv.tv_sec);
   GetLocalTime(&systemTime);
   tv.tv_usec = systemTime.wMilliseconds * 1000; 
#else 
   struct timeval tv; 
   int result = gettimeofday (&tv, NULL);
#endif   

   if (result == -1)
   {
      /* If we can't get the time of day, don't print a timestamp.
         Under Unix, this will never happen:  gettimeofday can fail only
         if the timezone is invalid which it can't be, since it is
         uninitialized]or if tv or tz are invalid pointers. */
      datebuf [0] = 0;
   }
   else
   {
      /* The tv_sec field represents the number of seconds passed since
         the Epoch, which is exactly the argument gettimeofday needs. */
      const time_t timeInSeconds = (time_t) tv.tv_sec;
      strftime (datebuf,
                datebufSize,
                "%Y%m%d-%H%M%S", /* guaranteed to fit in 256 chars,
                                    hence don't check return code */
                localtime (&timeInSeconds));
   }
   
   char msbuf[5];
   /* Dividing (without remainder) by 1000 rounds the microseconds
      measure to the nearest millisecond. */
   sprintf(msbuf, ".%3.3ld", long(tv.tv_usec / 1000));

   int datebufCharsRemaining = datebufSize - strlen (datebuf);
   strncat (datebuf, msbuf, datebufCharsRemaining - 1);

   datebuf[datebufSize - 1] = '\0'; /* Just in case strncat truncated msbuf,
                                        thereby leaving its last character at
                                        the end, instead of a null terminator */

   res = datebuf;
   // ugh, resize the Data
   //res.at(strlen(datebuf)-1);
   return res;
}

Log::Level 
Log::getServiceLevel(int service)
{
   Lock lock(_mutex);
#ifdef WIN32
	assert(0);
	return Bogus;
#else
   HashMap<int, Level>::iterator res = Log::_serviceToLevel.find(service);
   if(res == Log::_serviceToLevel.end())
   {
      //!dcm! -- should perhaps throw an exception here, instead of setting a
      //default level of LOG_ERROR, but nobody uses this yet
      Log::_serviceToLevel[service] = Err;
      return Err;
   }
   return res->second;
#endif
}
   
const Log::ThreadSetting*
Log::getThreadSetting()
{
#ifdef WIN32
   return 0;
#else
   ThreadSetting* setting = static_cast<ThreadSetting*>(pthread_getspecific(Log::_levelKey));
   if (setting == 0)
   {
      return 0;
   }
   if (Log::touchCount > 0)
   {
      Lock lock(_mutex);
      pthread_t thread = pthread_self();
      HashMap<pthread_t, pair<ThreadSetting, bool> >::iterator res = Log::_threadToLevel.find(thread);
      assert(res != Log::_threadToLevel.end());
      if (res->second.second)
      {
         setting->level = res->second.first.level;
         res->second.second = false;
         touchCount--;
//         cerr << "**Log::getThreadSetting:touchCount: " << Log::touchCount << "**" << endl;

         //cerr << "touchcount decremented" << endl;
      }
   }
   return setting;
#endif
}

void 
Log::setThreadSetting(int serv)
{
   Log::setThreadSetting(ThreadSetting(serv, getServiceLevel(serv)));
}

void 
Log::setThreadSetting(int serv, Log::Level l)
{
   Log::setThreadSetting(ThreadSetting(serv, l));
}

void 
Log::setThreadSetting(ThreadSetting info)
{
#ifdef WIN32
	assert(0);
#else
   //cerr << "Log::setThreadSetting: " << "service: " << info.service << " level " << toString(info.level) << " for " << pthread_self() << endl;
   pthread_t thread = pthread_self();
   pthread_setspecific(_levelKey, (void *) new ThreadSetting(info));
   Lock lock(_mutex);

   if (Log::_threadToLevel.find(thread) != Log::_threadToLevel.end())
   {
      if (Log::_threadToLevel[thread].second == true)
      {
         touchCount--;
      }
   }
   Log::_threadToLevel[thread].first = info;
   Log::_threadToLevel[thread].second = false;
   Log::_serviceToThreads[info.service].insert(thread);
#endif
}
   
void 
Log::setServiceLevel(int service, Level l)
{
   Lock lock(_mutex);
   Log::_serviceToLevel[service] = l;
#ifdef WIN32
	assert(0);
#else
   set<pthread_t>& threads = Log::_serviceToThreads[service];
   for (set<pthread_t>::iterator i = threads.begin(); i != threads.end(); i++)
   {
      Log::_threadToLevel[*i].first.level = l;
      Log::_threadToLevel[*i].second = true;
   }
   Log::touchCount += threads.size();
#endif
//   cerr << "**Log::setServiceLevel:touchCount: " << Log::touchCount << "**" << endl;
}

Log::Guard::Guard(resip::Log::Level level,
                  const resip::Subsystem& subsystem,
                  const char* file,
                  int line) :
   mLevel(level),
   mSubsystem(subsystem),
   mFile(file),
   mLine(line),
   mData(Data::Borrow, mBuffer, sizeof(mBuffer) - 1),
   mStream(mData.clear())
{
   mBuffer[0] = 0;
   mData.clear();
   Log::tags(mLevel, mSubsystem, mFile, mLine, mStream);
   mStream << resip::Log::delim;
   mStream.flush();
   
   mHeaderLength = mData.size();
}

Log::Guard::~Guard()
{
   mStream.flush();

   if (resip::Log::getExternal())
   {
      const resip::Data rest(resip::Data::Share,
                             mData.data() + mHeaderLength,
                             mData.size() - mHeaderLength);
      if (!(*resip::Log::getExternal())(mLevel, 
                                        mSubsystem, 
                                        resip::Log::getAppName(),
                                        mFile,
                                        mLine, 
                                        rest, 
                                        mData))
      {
         return;
      }
   }
    
   resip::Lock lock(resip::Log::_mutex);
   // !dlb! imlement VSDebugWindow as an external logger
   if (resip::Log::_type == resip::Log::VSDebugWindow)
   {
      mData += "\r\n";
      resip::GenericLogImpl::OutputToWin32DebugWindow(mData);
   }
   else
   {
      // endl is magic in syslog -- so put it here
      resip::GenericLogImpl::Instance() << mData << std::endl;
   }
}


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
