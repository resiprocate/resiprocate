#include "rutil/Socket.hxx"

#include "rutil/ResipAssert.h"
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
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/ThreadIf.hxx"
#include "rutil/Subsystem.hxx"
#include "rutil/SysLogStream.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

const Data Log::delim(" | ");
Log::ThreadData Log::mDefaultLoggerData(0, Log::Cout, Log::Info, NULL, NULL);
Data Log::mAppName;
Data Log::mHostname;
#ifndef WIN32
int Log::mSyslogFacility = LOG_DAEMON;
#else
int Log::mSyslogFacility = -1;
#endif
unsigned int Log::MaxLineCount = 0; // no limit by default
unsigned int Log::MaxByteCount = 0; // no limit by default

#ifdef WIN32
int Log::mPid=0;
#else 
pid_t Log::mPid=0;
#endif

volatile short Log::touchCount = 0;


/// DEPRECATED! Left for backward compatibility - use localLoggers instead
#ifdef LOG_ENABLE_THREAD_SETTING
#if defined(__APPLE__) || defined(__CYGWIN__)
HashValueImp(ThreadIf::Id, (size_t)data);
#endif
HashMap<ThreadIf::Id, std::pair<Log::ThreadSetting, bool> > Log::mThreadToLevel;
HashMap<int, std::set<ThreadIf::Id> > Log::mServiceToThreads;
ThreadIf::TlsKey* Log::mLevelKey;
#endif
HashMap<int, Log::Level> Log::mServiceToLevel;

Log::LocalLoggerMap Log::mLocalLoggerMap;
ThreadIf::TlsKey* Log::mLocalLoggerKey;

const char
Log::mDescriptions[][32] = {"NONE", "EMERG", "ALERT", "CRIT", "ERR", "WARNING", "NOTICE", "INFO", "DEBUG", "STACK", "CERR", ""}; 

Mutex Log::_mutex;

extern "C"
{
   void freeThreadSetting(void* setting)
   {
      delete static_cast<Log::ThreadSetting*>(setting);
   }

   void freeLocalLogger(void* pThreadData)
   {
      if (pThreadData)
      {
         // There was some local logger installed. Decrease its use count before we
         // continue.
         Log::mLocalLoggerMap.decreaseUseCount((static_cast<Log::ThreadData*>(pThreadData))->id());
      }
   }
}

unsigned int LogStaticInitializer::mInstanceCounter=0;
LogStaticInitializer::LogStaticInitializer()
{
   if (mInstanceCounter++ == 0)
   {
#ifdef LOG_ENABLE_THREAD_SETTING
         Log::mLevelKey = new ThreadIf::TlsKey;
         ThreadIf::tlsKeyCreate(*Log::mLevelKey, freeThreadSetting);
#endif

         Log::mLocalLoggerKey = new ThreadIf::TlsKey;
         ThreadIf::tlsKeyCreate(*Log::mLocalLoggerKey, freeLocalLogger);
   }
}
LogStaticInitializer::~LogStaticInitializer()
{
   if (--mInstanceCounter == 0)
   {
#ifdef LOG_ENABLE_THREAD_SETTING
      ThreadIf::tlsKeyDelete(*Log::mLevelKey);
      delete Log::mLevelKey;
#endif

      ThreadIf::tlsKeyDelete(*Log::mLocalLoggerKey);
      delete Log::mLocalLoggerKey;
   }
}

void
Log::initialize(const char* typed, const char* leveld, const char* appName, const char *logFileName, ExternalLogger* externalLogger, const char* syslogFacilityName)
{
   Log::initialize(Data(typed), Data(leveld), Data(appName), logFileName, externalLogger, syslogFacilityName);
}

void
Log::initialize(const Data& typed, const Data& leveld, const Data& appName, 
                const char *logFileName, ExternalLogger* externalLogger,
                const Data& syslogFacilityName)
{
   Type type = Log::Cout;
   if (isEqualNoCase(typed, "cout")) type = Log::Cout;
   else if (isEqualNoCase(typed, "cerr")) type = Log::Cerr;
   else if (isEqualNoCase(typed, "file")) type = Log::File;
   else type = Log::Syslog;
   
   Level level = Log::Info;
   level = toLevel(leveld);

   Log::initialize(type, level, appName, logFileName, externalLogger, syslogFacilityName);
}

int
Log::parseSyslogFacilityName(const Data& facilityName)
{
#ifndef WIN32
   /* In theory, some platforms may not have all the log facilities
      defined in syslog.h.  Only LOG_USER and LOG_LOCAL[0-7] are considered
      mandatory.
      If the compile fails with errors in this method, then the unsupported
      facility names could be wrapped in conditional logic.
   */
   if(facilityName == "LOG_AUTH")
   {
      return LOG_AUTH;
   }
   else if(facilityName == "LOG_AUTHPRIV")
   {
      return LOG_AUTHPRIV;
   }
   else if(facilityName == "LOG_CRON")
   {
      return LOG_CRON;
   }
   else if(facilityName == "LOG_DAEMON")
   {
      return LOG_DAEMON;
   }
   else if(facilityName == "LOG_FTP")
   {
      return LOG_FTP;
   }
   else if(facilityName == "LOG_KERN")
   {
      return LOG_KERN;
   }
   else if(facilityName == "LOG_LOCAL0")
   {
      return LOG_LOCAL0;
   }
   else if(facilityName == "LOG_LOCAL1")
   {
      return LOG_LOCAL1;
   }
   else if(facilityName == "LOG_LOCAL2")
   {
      return LOG_LOCAL2;
   }
   else if(facilityName == "LOG_LOCAL3")
   {
      return LOG_LOCAL3;
   }
   else if(facilityName == "LOG_LOCAL4")
   {
      return LOG_LOCAL4;
   }
   else if(facilityName == "LOG_LOCAL5")
   {
      return LOG_LOCAL5;
   }
   else if(facilityName == "LOG_LOCAL6")
   {
      return LOG_LOCAL6;
   }
   else if(facilityName == "LOG_LOCAL7")
   {
      return LOG_LOCAL7;
   }
   else if(facilityName == "LOG_LPR")
   {
      return LOG_LPR;
   }
   else if(facilityName == "LOG_MAIL")
   {
      return LOG_MAIL;
   }
   else if(facilityName == "LOG_NEWS")
   {
      return LOG_NEWS;
   }
   else if(facilityName == "LOG_SYSLOG")
   {
      return LOG_SYSLOG;
   }
   else if(facilityName == "LOG_USER")
   {
      return LOG_USER;
   }
   else if(facilityName == "LOG_UUCP")
   {
      return LOG_UUCP;
   }
#endif
   // Nothing matched or syslog not supported on this platform
   return -1;
}

void 
Log::initialize(Type type, Level level, const Data& appName, 
                const char * logFileName,
                ExternalLogger* externalLogger,
                const Data& syslogFacilityName)
{
   Lock lock(_mutex);
   mDefaultLoggerData.reset();   
   
   mDefaultLoggerData.set(type, level, logFileName, externalLogger);

   ParseBuffer pb(appName);
   pb.skipToEnd();
#ifdef _WIN32
   pb.skipBackToChar('\\');
#else
   pb.skipBackToChar('/');
#endif
   mAppName = pb.position();

   if(!syslogFacilityName.empty())
   {
      mSyslogFacility = parseSyslogFacilityName(syslogFacilityName);
      if(mSyslogFacility == -1)
      {
#ifndef WIN32
         mSyslogFacility = LOG_DAEMON;
         if(type == Log::Syslog)
         {
            syslog(LOG_DAEMON | LOG_ERR, "invalid syslog facility name specified (%s), falling back to LOG_DAEMON", syslogFacilityName.c_str());
         }
#endif
         std::cerr << "invalid syslog facility name specified: " << syslogFacilityName.c_str() << std::endl;
      }
   }
 
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
                ExternalLogger& logger,
                const Data& syslogFacilityName)
{
   initialize(type, level, appName, 0, &logger, syslogFacilityName);
}

void
Log::setLevel(Level level)
{
   Lock lock(_mutex);
   getLoggerData().mLevel = level; 
}

void
Log::setLevel(Level level, Subsystem& s)
{
   Lock lock(_mutex);
   s.setLevel(level); 
}

void
Log::setLevel(Level level, Log::LocalLoggerId loggerId)
{
   if (loggerId)
   {
      ThreadData *pData = mLocalLoggerMap.getData(loggerId);
      if (pData)
      {
         // Local logger found. Set logging level.
         pData->mLevel = level;

         // We don't need local logger instance anymore.
         mLocalLoggerMap.decreaseUseCount(loggerId);
         pData = NULL;
      }
   }
   else
   {
      Lock lock(_mutex);
      mDefaultLoggerData.mLevel = level;
   }
}

Log::Level 
Log::level(Log::LocalLoggerId loggerId)
{
   Level level;
   ThreadData *pData;
   if (loggerId && (pData = mLocalLoggerMap.getData(loggerId)))
   {
      // Local logger found. Set logging level.
      level = pData->mLevel;

      // We don't need local logger instance anymore.
      mLocalLoggerMap.decreaseUseCount(loggerId);
      pData = NULL;
   }
   else
   {
      Lock lock(_mutex);
      level = mDefaultLoggerData.mLevel;
   }
   return level;
}

void 
Log::setMaxLineCount(unsigned int maxLineCount)
{
   Lock lock(_mutex);
   getLoggerData().mMaxLineCount = maxLineCount; 
}

void 
Log::setMaxLineCount(unsigned int maxLineCount, Log::LocalLoggerId loggerId)
{
   if (loggerId)
   {
      ThreadData *pData = mLocalLoggerMap.getData(loggerId);
      if (pData)
      {
         // Local logger found. Set logging level.
         pData->mMaxLineCount = maxLineCount;

         // We don't need local logger instance anymore.
         mLocalLoggerMap.decreaseUseCount(loggerId);
         pData = NULL;
      }
   }
   else
   {
      Lock lock(_mutex);
      mDefaultLoggerData.mMaxLineCount = maxLineCount;
   }
}

void 
Log::setMaxByteCount(unsigned int maxByteCount)
{
   Lock lock(_mutex);
   getLoggerData().mMaxByteCount = maxByteCount; 
}

void 
Log::setMaxByteCount(unsigned int maxByteCount, Log::LocalLoggerId loggerId)
{
   if (loggerId)
   {
      ThreadData *pData = mLocalLoggerMap.getData(loggerId);
      if (pData)
      {
         // Local logger found. Set logging level.
         pData->mMaxByteCount = maxByteCount;

         // We don't need local logger instance anymore.
         mLocalLoggerMap.decreaseUseCount(loggerId);
         pData = NULL;
      }
   }
   else
   {
      Lock lock(_mutex);
      mDefaultLoggerData.mMaxByteCount = maxByteCount;
   }
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
      if (isEqualNoCase(pri, Data(mDescriptions[i])))
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
   if(resip::Log::getLoggerData().type() == Syslog)
   {
      strm // << mDescriptions[level+1] << Log::delim
   //        << timestamp(ts) << Log::delim
   //        << mHostname << Log::delim
   //        << mAppName << Log::delim
           << subsystem << Log::delim
   //        << mPid << Log::delim
           << pthread_self() << Log::delim
           << pfile << ":" << line;
   }
   else
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
   struct tm localTimeResult;
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
#ifdef WIN32
                localtime (&timeInSeconds));  // Thread safe call on Windows
#else
                localtime_r (&timeInSeconds, &localTimeResult));  // Thread safe version of localtime on linux
#endif
   }
   
   char msbuf[5];
   /* Dividing (without remainder) by 1000 rounds the microseconds
      measure to the nearest millisecond. */
   snprintf(msbuf, 5, ".%3.3ld", long(tv.tv_usec / 1000));

   int datebufCharsRemaining = datebufSize - (int)strlen(datebuf);
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
      resip_assert(res != Log::mThreadToLevel.end());
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
   resip_assert(0);
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
   resip_assert(0);
#else
   set<ThreadIf::Id>& threads = Log::mServiceToThreads[service];
   for (set<ThreadIf::Id>::iterator i = threads.begin(); i != threads.end(); i++)
   {
      Log::mThreadToLevel[*i].first.mLevel = l;
      Log::mThreadToLevel[*i].second = true;
   }
   Log::touchCount += (short)threads.size();
#endif
//   cerr << "**Log::setServiceLevel:touchCount: " << Log::touchCount << "**" << endl;
}

Log::LocalLoggerId Log::localLoggerCreate(Log::Type type,
                                          Log::Level level,
                                          const char * logFileName,
                                          ExternalLogger* externalLogger)
{
   return mLocalLoggerMap.create(type, level, logFileName, externalLogger);
}

int Log::localLoggerReinitialize(Log::LocalLoggerId loggerId,
                                 Log::Type type,
                                 Log::Level level,
                                 const char * logFileName,
                                 ExternalLogger* externalLogger)
{
   return mLocalLoggerMap.reinitialize(loggerId, type, level, logFileName, externalLogger);
}

int Log::localLoggerRemove(Log::LocalLoggerId loggerId)
{
   return mLocalLoggerMap.remove(loggerId);
}

int Log::setThreadLocalLogger(Log::LocalLoggerId loggerId)
{
   ThreadData* pData = static_cast<ThreadData*>(ThreadIf::tlsGetValue(*Log::mLocalLoggerKey));
   if (pData)
   {
      // There was some local logger installed. Decrease its use count before we
      // continue.
      mLocalLoggerMap.decreaseUseCount(pData->id());
      pData = NULL;
   }
   if (loggerId)
   {
      pData = mLocalLoggerMap.getData(loggerId);
   }
   ThreadIf::tlsSetValue(*mLocalLoggerKey, (void *) pData);
   return (loggerId == 0) || (pData != NULL)?0:1;
}

std::ostream&
Log::Instance(unsigned int bytesToWrite)
{
   return getLoggerData().Instance(bytesToWrite);
}

void 
Log::reset()
{
   getLoggerData().reset();
}

#ifndef WIN32
void
Log::droppingPrivileges(uid_t uid, pid_t pid)
{
   getLoggerData().droppingPrivileges(uid, pid);
}
#endif

bool
Log::isLogging(Log::Level level, const resip::Subsystem& sub)
{
   if (sub.getLevel() != Log::None)
   {
      return level <= sub.getLevel();
   }
   else
   {
      return (level <= Log::getLoggerData().mLevel);
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

Log::LocalLoggerId Log::LocalLoggerMap::create(Log::Type type,
                                                    Log::Level level,
                                                    const char * logFileName,
                                                    ExternalLogger* externalLogger)
{
   Lock lock(mLoggerInstancesMapMutex);
   Log::LocalLoggerId id = ++mLastLocalLoggerId;
   Log::ThreadData *pNewData = new Log::ThreadData(id, type, level, logFileName,
                                                   externalLogger);
   mLoggerInstancesMap[id].first = pNewData;
   mLoggerInstancesMap[id].second = 0;
   return id;
}

int Log::LocalLoggerMap::reinitialize(Log::LocalLoggerId loggerId,
                                      Log::Type type,
                                      Log::Level level,
                                      const char * logFileName,
                                      ExternalLogger* externalLogger)
{
   Lock lock(mLoggerInstancesMapMutex);
   LoggerInstanceMap::iterator it = mLoggerInstancesMap.find(loggerId);
   if (it == mLoggerInstancesMap.end())
   {
      // No such logger ID
      std::cerr << "Log::LocalLoggerMap::remove(): Unknown local logger id=" << loggerId << std::endl;
      return 1;
   }
   it->second.first->reset();
   it->second.first->set(type, level, logFileName, externalLogger);
   return 0;
}

int Log::LocalLoggerMap::remove(Log::LocalLoggerId loggerId)
{
   Lock lock(mLoggerInstancesMapMutex);
   LoggerInstanceMap::iterator it = mLoggerInstancesMap.find(loggerId);
   if (it == mLoggerInstancesMap.end())
   {
      // No such logger ID
      std::cerr << "Log::LocalLoggerMap::remove(): Unknown local logger id=" << loggerId << std::endl;
      return 1;
   }
   if (it->second.second > 0)
   {
      // Non-zero use-count.
      std::cerr << "Log::LocalLoggerMap::remove(): Use count is non-zero (" << it->second.second << ")!" << std::endl;
      return 2;
   }
   delete it->second.first;  // delete ThreadData
   mLoggerInstancesMap.erase(it);
   return 0;
}

Log::ThreadData *Log::LocalLoggerMap::getData(Log::LocalLoggerId loggerId)
{
   Lock lock(mLoggerInstancesMapMutex);
   LoggerInstanceMap::iterator it = mLoggerInstancesMap.find(loggerId);
   if (it == mLoggerInstancesMap.end())
   {
      // No such logger ID
      return NULL;
   }
   it->second.second++;
   return it->second.first;
}

void Log::LocalLoggerMap::decreaseUseCount(Log::LocalLoggerId loggerId)
{
   Lock lock(mLoggerInstancesMapMutex);
   LoggerInstanceMap::iterator it = mLoggerInstancesMap.find(loggerId);
   if (it != mLoggerInstancesMap.end())
   {
      it->second.second--;
      resip_assert(it->second.second >= 0);
   }
}


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
	
   if (resip::Log::getLoggerData().mType != resip::Log::OnlyExternalNoHeaders)
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
                             (int)mData.size() - mHeaderLength);
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
    
   Type logType = resip::Log::getLoggerData().mType;

   if(logType == resip::Log::OnlyExternal ||
      logType == resip::Log::OnlyExternalNoHeaders) 
   {
      return;
   }

   resip::Lock lock(resip::Log::_mutex);
   // !dlb! implement VSDebugWindow as an external logger
   if (logType == resip::Log::VSDebugWindow)
   {
      mData += "\r\n";
      OutputToWin32DebugWindow(mData);
   }
   else 
   {
      // endl is magic in syslog -- so put it here
      std::ostream& _instance = Instance((int)mData.size()+2);
      if (logType == resip::Log::Syslog)
      {
         _instance << mLevel;
      }
      _instance << mData << std::endl;  
   }
}

std::ostream&
Log::ThreadData::Instance(unsigned int bytesToWrite)
{
//   std::cerr << "Log::ThreadData::Instance() id=" << mId << " type=" << mType <<  std::endl;
   switch (mType)
   {
      case Log::Syslog:
         if (mLogger == 0)
         {
            mLogger = new SysLogStream(mAppName, mSyslogFacility);
         }
         return *mLogger;

      case Log::Cerr:
         return std::cerr;

      case Log::Cout:
         return std::cout;

      case Log::File:
         if (mLogger == 0 ||
             (maxLineCount() && mLineCount >= maxLineCount()) ||
             (maxByteCount() && ((unsigned int)mLogger->tellp()+bytesToWrite) >= maxByteCount()))
         {
            Data logFileName(mLogFileName != "" ? mLogFileName : "resiprocate.log");
            if (mLogger)
            {
               Data oldLogFileName(logFileName + ".old");
               delete mLogger;
               // Keep one backup file: Delete .old file, Rename log file to .old
               // Could be expanded in the future to keep X backup log files
               remove(oldLogFileName.c_str());
               rename(logFileName.c_str(), oldLogFileName.c_str());
            }
            mLogger = new std::ofstream(logFileName.c_str(), std::ios_base::out | std::ios_base::app);
            mLineCount = 0;
         }
         mLineCount++;
         return *mLogger;
      default:
         resip_assert(0);
         return std::cout;
   }
}

void 
Log::ThreadData::reset()
{
   delete mLogger;
   mLogger = NULL;
}

#ifndef WIN32
void
Log::ThreadData::droppingPrivileges(uid_t uid, pid_t pid)
{
   if(mType == Log::File)
   {
      Data logFileName(mLogFileName != "" ? mLogFileName : "resiprocate.log");
      if(chown(logFileName.c_str(), uid, pid) < 0)
      {
         // Some error occurred
         std::cerr << "ERROR: chown failed on " << logFileName << std::endl;
      }
   }
}
#endif

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
