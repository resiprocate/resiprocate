#include "rutil/Socket.hxx"

#include <cassert>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include "rutil/Data.hxx"

#ifndef WIN32
#include <sys/time.h>
#endif

#include <sys/types.h>
#include <time.h>

#include "rutil/Log.hxx"
#include "rutil/Lock.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/ThreadIf.hxx"
#include "rutil/Subsystem.hxx"
#include "rutil/SysLogStream.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

const Data Log::delim(" | ");
Log::ThreadData Log::mDefaultTreadSettings(Log::Cout, Log::Info, NULL, NULL);
Data Log::mAppName;
Data Log::mHostname;
unsigned int Log::MaxLineCount = 0; // no limit by default

#ifdef WIN32
int Log::mPid=0;
#else 
pid_t Log::mPid=0;
#endif

volatile short Log::touchCount = 0;


#ifdef LOG_ENABLE_THREAD_SETTING
#if defined(__APPLE__) || defined(__CYGWIN__)
HashValueImp(ThreadIf::Id, (size_t)data);
#endif
HashMap<ThreadIf::Id, std::pair<Log::ThreadSetting, bool> > Log::mThreadToLevel;
HashMap<int, std::set<ThreadIf::Id> > Log::mServiceToThreads;
ThreadIf::TlsKey* Log::mLevelKey = (Log::mLevelKey ? Log::mLevelKey : new ThreadIf::TlsKey);
#endif

HashMap<int, Log::Level> Log::mServiceToLevel;

const char
Log::mDescriptions[][32] = {"NONE", "EMERG", "ALERT", "CRIT", "ERR", "WARNING", "NOTICE", "INFO", "DEBUG", "STACK", "CERR", ""}; 

Mutex Log::_mutex;

extern "C"
{
   void freeThreadSetting(void* setting)
   {
      delete static_cast<Log::ThreadSetting*>(setting);
   }
}
bool
Log::init()
{
#ifdef LOG_ENABLE_THREAD_SETTING
	if (Log::mLevelKey == 0)
	{
		Log::mLevelKey = new ThreadIf::TlsKey;
      ThreadIf::tlsKeyCreate(*Log::mLevelKey, freeThreadSetting);
	}
#endif
	return true;
}

void
Log::initialize(const char* typed, const char* leveld, const char* appName, const char *logFileName, ExternalLogger* externalLogger)
{
   Log::initialize(Data(typed), Data(leveld), Data(appName), logFileName, externalLogger);
}

void
Log::initialize(const Data& typed, const Data& leveld, const Data& appName, 
                const char *logFileName, ExternalLogger* externalLogger)
{
   Type type = Log::Cout;
   if (isEqualNoCase(typed, "cout")) type = Log::Cout;
   else if (isEqualNoCase(typed, "cerr")) type = Log::Cerr;
   else if (isEqualNoCase(typed, "file")) type = Log::File;
   else type = Log::Syslog;
   
   Level level = Log::Info;
   level = toLevel(leveld);

   Log::initialize(type, level, appName, logFileName, externalLogger);
}

void 
Log::initialize(Type type, Level level, const Data& appName, 
                const char * logFileName,
                ExternalLogger* externalLogger)
{
   Lock lock(_mutex);
   reset();   
   
   mDefaultTreadSettings.set(type, level, logFileName, externalLogger);

   ParseBuffer pb(appName);
   pb.skipToEnd();
#ifdef _WIN32
   pb.skipBackToChar('\\');
#else
   pb.skipBackToChar('/');
#endif
   mAppName = pb.position();
 
   char buffer[1024];  
   gethostname(buffer, sizeof(buffer));
   mHostname = buffer;
#ifdef WIN32 
   mPid = (int)GetCurrentProcess();
#else
   mPid = getpid();
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
   mDefaultTreadSettings.mLevel = level; 
}

void
Log::setLevel(Level level, Subsystem& s)
{
   Lock lock(_mutex);
   s.setLevel(level); 
}

const static Data log_("LOG_");

Data
Log::toString(Level l)
{
   return log_ + mDescriptions[l+1];
}

Log::Level
Log::toLevel(const Data& l)
{
   Data pri( l.prefix("LOG_") ? l.substr(4) : l);

   int i=0;
   while (strlen(mDescriptions[i]))
   {
      if (strcmp(pri.c_str(), mDescriptions[i]) == 0)
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

EncodeStream &
Log::tags(Log::Level level,
          const Subsystem& subsystem,
          const char* pfile,
          int line,
          EncodeStream& strm)
{
   char buffer[256];
   Data ts(Data::Borrow, buffer, sizeof(buffer));
#if defined( __APPLE__ )
  strm << mDescriptions[level+1] << Log::delim
        << timestamp(ts) << Log::delim  
        << mAppName << Log::delim
        << subsystem << Log::delim 
        << pthread_self() << Log::delim
        << pfile << ":" << line;
#elif defined( WIN32 )
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
   strm << mDescriptions[level+1] << Log::delim
        << timestamp(ts) << Log::delim  
        << mAppName << Log::delim
        << subsystem << Log::delim 
        << GetCurrentThreadId() << Log::delim
        << file << ":" << line;
#else // #if defined( WIN32 ) || defined( __APPLE__ )
   strm << mDescriptions[level+1] << Log::delim
        << timestamp(ts) << Log::delim  
//        << mHostname << Log::delim  
        << mAppName << Log::delim
        << subsystem << Log::delim 
//        << mPid << Log::delim
        << pthread_self() << Log::delim
        << pfile << ":" << line;
#endif
   return strm;
}

Data
Log::timestamp()
{
   char buffer[256];
   Data result(Data::Borrow, buffer, sizeof(buffer));
   return timestamp(result);
}

Data&
Log::timestamp(Data& res) 
{
   char* datebuf = const_cast<char*>(res.data());
   const unsigned int datebufSize = 256;
   res.clear();
   
#ifdef WIN32 
   int result = 1; 
   SYSTEMTIME systemTime;
   struct { time_t tv_sec; int tv_usec; } tv = {0,0};
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

   // ugh, resize the Data
   res.at(strlen(datebuf)-1);
   return res;
}

Log::Level 
Log::getServiceLevel(int service)
{
   Lock lock(_mutex);
   HashMap<int, Level>::iterator res = Log::mServiceToLevel.find(service);
   if(res == Log::mServiceToLevel.end())
   {
      //!dcm! -- should perhaps throw an exception here, instead of setting a
      //default level of LOG_ERROR, but nobody uses this yet
      Log::mServiceToLevel[service] = Err;
      return Err;
   }
   return res->second;
}
   
const Log::ThreadSetting*
Log::getThreadSetting()
{
#ifndef LOG_ENABLE_THREAD_SETTING
   return 0;
#else
   ThreadSetting* setting = static_cast<ThreadSetting*>(ThreadIf::tlsGetValue(*Log::mLevelKey));
   if (setting == 0)
   {
      return 0;
   }
   if (Log::touchCount > 0)
   {
      Lock lock(_mutex);
      ThreadIf::Id thread = ThreadIf::selfId();
      HashMap<ThreadIf::Id, pair<ThreadSetting, bool> >::iterator res = Log::mThreadToLevel.find(thread);
      assert(res != Log::mThreadToLevel.end());
      if (res->second.second)
      {
         setting->mLevel = res->second.first.mLevel;
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
#ifndef LOG_ENABLE_THREAD_SETTING
   assert(0);
#else
   //cerr << "Log::setThreadSetting: " << "service: " << info.service << " level " << toString(info.level) << " for " << pthread_self() << endl;
   ThreadIf::Id thread = ThreadIf::selfId();
   ThreadIf::tlsSetValue(*mLevelKey, (void *) new ThreadSetting(info));
   Lock lock(_mutex);

   if (Log::mThreadToLevel.find(thread) != Log::mThreadToLevel.end())
   {
      if (Log::mThreadToLevel[thread].second == true)
      {
         touchCount--;
      }
   }
   Log::mThreadToLevel[thread].first = info;
   Log::mThreadToLevel[thread].second = false;
   Log::mServiceToThreads[info.mService].insert(thread);
#endif
}
   
void 
Log::setServiceLevel(int service, Level l)
{
   Lock lock(_mutex);
   Log::mServiceToLevel[service] = l;
#ifndef LOG_ENABLE_THREAD_SETTING
   assert(0);
#else
   set<ThreadIf::Id>& threads = Log::mServiceToThreads[service];
   for (set<ThreadIf::Id>::iterator i = threads.begin(); i != threads.end(); i++)
   {
      Log::mThreadToLevel[*i].first.mLevel = l;
      Log::mThreadToLevel[*i].second = true;
   }
   Log::touchCount += threads.size();
#endif
//   cerr << "**Log::setServiceLevel:touchCount: " << Log::touchCount << "**" << endl;
}

std::ostream&
Log::Instance()
{
   return mDefaultTreadSettings.Instance();
}

void 
Log::reset()
{
   mDefaultTreadSettings.reset();
}

bool
Log::isLogging(Log::Level level, const resip::Subsystem& sub)
{
   if (sub.getLevel() != Log::None)
   {
      return level <= sub.getLevel();
   }
   else
   {
      return (level <= Log::mDefaultTreadSettings.mLevel);
   }
}

void
Log::OutputToWin32DebugWindow(const Data& result)
{
#ifdef WIN32
   const char *text = result.c_str();
#ifdef UNDER_CE
   LPWSTR lpwstrText = resip::ToWString(text);
   OutputDebugStringW(lpwstrText);
   FreeWString(lpwstrText);
#else
   OutputDebugStringA(text);
#endif
#endif
}

ExternalLogger::~ExternalLogger()
{}

Log::Guard::Guard(resip::Log::Level level,
                  const resip::Subsystem& subsystem,
                  const char* file,
                  int line) :
   mLevel(level),
   mSubsystem(subsystem),
   mFile(file),
   mLine(line),
   mData(Data::Borrow, mBuffer, sizeof(mBuffer)),
   mStream(mData.clear())
{
	
   if (resip::Log::mDefaultTreadSettings.mType != resip::Log::OnlyExternalNoHeaders)
   {
      Log::tags(mLevel, mSubsystem, mFile, mLine, mStream);
      mStream << resip::Log::delim;
      mStream.flush();

      mHeaderLength = mData.size();
   }
   else
   {
      mHeaderLength = 0;
   }
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
   // !dlb! implement VSDebugWindow as an external logger
   if (resip::Log::mDefaultTreadSettings.mType == resip::Log::VSDebugWindow)
   {
      mData += "\r\n";
      OutputToWin32DebugWindow(mData);
   }
   else if(resip::Log::mDefaultTreadSettings.mType == resip::Log::OnlyExternal ||
	   resip::Log::mDefaultTreadSettings.mType == resip::Log::OnlyExternalNoHeaders) 
   {
      return;
   }
   else 
   {
      // endl is magic in syslog -- so put it here
      Instance() << mData << std::endl;
   }
}

std::ostream&
Log::ThreadData::Instance()
{
   switch (mType)
   {
      case Log::Syslog:
         if (mLogger == 0)
         {
            std::cerr << "Creating a syslog stream" << std::endl;
            mLogger = new SysLogStream;
         }
         return *mLogger;

      case Log::Cerr:
         return std::cerr;

      case Log::Cout:
         return std::cout;

      case Log::File:
         if (mLogger == 0 ||
             (MaxLineCount && mLineCount > MaxLineCount))
         {
            std::cerr << "Creating a file logger" << std::endl;
            if (mLogger)
            {
               delete mLogger;
            }
            if (mLogFileName != "")
            {
               mLogger = new std::ofstream(mLogFileName.c_str(), std::ios_base::out | std::ios_base::trunc);
               mLineCount = 0;
            }
            else
            {
               mLogger = new std::ofstream("resiprocate.log", std::ios_base::out | std::ios_base::trunc);
               mLineCount = 0;
            }
         }
         mLineCount++;
         return *mLogger;
      default:
         assert(0);
         return std::cout;
   }
}

void 
Log::ThreadData::reset()
{
   delete mLogger;
   mLogger = NULL;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005
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
