#ifndef RESIP_Log_hxx
#define RESIP_Log_hxx

#include "rutil/Data.hxx"

#ifndef WIN32
#include <syslog.h>
#include <unistd.h>
#endif

#include <set>

#include "rutil/Mutex.hxx"
#include "rutil/Lock.hxx"
#include "rutil/HashMap.hxx"
#include "rutil/ThreadIf.hxx"
#include <iostream>

// !ipse! I think that this remark is no more valid with recent changes,
// but I don't have MacOs X to test with. Someone should take this duty.
//
// NOTE: disabling thread setting code for native mac os applications.
// since some logging takes place during static initialization we can't
// be sure all the pthread stuff is ready to go. this eventually causes
// crashes in the Mac OS native API.
#if !defined(TARGET_OS_MAC)
#define LOG_ENABLE_THREAD_SETTING
// defining hash function in mac os (non-sdk api) and cygwin because
// ThreadIf::Id is a pointer,  (this assumes it's always the same pointer)
#if defined(__APPLE__) || defined(__CYGWIN__)
HashValue(resip::ThreadIf::Id);
#endif
#endif

extern "C"
{
   // Forward declaration to make it friend of Log class.
   void freeLocalLogger(void* pThreadData);
};


namespace resip
{

class ExternalLogger;
class Subsystem;

/**
   @brief Singleton that handles logging calls.

   @see Logger for usage details
*/
class Log
{
   public:
      enum Type
      {
         Cout = 0,
         Syslog, 
         File, 
         Cerr,
         VSDebugWindow,        ///< Use only for Visual Studio Debug Window logging - WIN32 must be defined
         OnlyExternal,         ///< log messages are only written to external logger
         OnlyExternalNoHeaders ///< same as OnlyExternal, only the messageWithHeaders param of the ExternalLogger
                               ///< will be empty.  This parameter usually contains a pre-formatted log entry.
      };
      
      enum Level
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
      };

      /// Thread Local logger ID type.
      typedef int LocalLoggerId;

      /**
         @brief Implementation for logging macros.

         Log::Guard(Log::Info, Subsystem::TEST, __FILE__, __LINE__) << ... ;
      */
      class Guard
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

            EncodeStream& asStream() {return mStream;}
            operator EncodeStream&() {return mStream;}

         private:
            resip::Log::Level mLevel;
            const resip::Subsystem& mSubsystem;
            resip::Data::size_type mHeaderLength;
            const char* mFile;
            int mLine;
            char mBuffer[128];
            Data mData;
            oDataStream mStream;
            Guard& operator=(const Guard&);
      };

      class ThreadSetting
      {
         public:
            ThreadSetting()
               : mService(-1),
                 mLevel(Err)
            {}

            ThreadSetting(int serv, Level level)
               : mService(serv),
                 mLevel(level)
            {
            }
            
            int mService;
            Level mLevel;
      };

      /// output the loglevel, hostname, appname, pid, tid, subsystem
      static EncodeStream& tags(Log::Level level,
                                const Subsystem& subsystem, 
                                const char* file,
                                int line,
                                EncodeStream& strm);

      static Data& timestamp(Data& result);
      static Data timestamp();
      static ExternalLogger* getExternal()
      {
         return getLoggerData().mExternalLogger;
      }
      static Data getAppName()
      {
         return mAppName;
      }

      static int parseSyslogFacilityName(const Data& facilityName);

      static void initialize(Type type,
                             Level level,
                             const Data& appName,
                             const char * logFileName = 0,
                             ExternalLogger* externalLogger = 0,
                             const Data& syslogFacility = "LOG_DAEMON");
      static void initialize(const Data& type,
                             const Data& level,
                             const Data& appName,
                             const char * logFileName = 0,
                             ExternalLogger* externalLogger = 0,
                             const Data& syslogFacility = "LOG_DAEMON");
      static void initialize(const char* type,
                             const char* level,
                             const char* appName,
                             const char * logFileName = 0,
                             ExternalLogger* externalLogger = 0,
                             const char* syslogFacility = "LOG_DAEMON");
      static void initialize(Type type,
                             Level level,
                             const Data& appName,
                             ExternalLogger& logger,
                             const Data& syslogFacility = "LOG_DAEMON");

      /** @brief Set logging level for current thread.
      * If thread has no local logger attached, then set global logging level.
      */
      static void setLevel(Level level);
      /** @brief Set logging level for given subsystem. */
      static void setLevel(Level level, Subsystem& s);
      /** Set logging level for given local logger. Use 0 to set global logging level. */
      static void setLevel(Level level, LocalLoggerId loggerId);
      /** @brief Return logging level for current thread.
      * If thread has no local logger attached, then return global logging level.
      */
      static Level level() { Lock lock(_mutex); return getLoggerData().mLevel; }
      /** Return logging level for given local logger. Use 0 to set global logging level. */
      static Level level(LocalLoggerId loggerId);
      static LocalLoggerId id() { Lock lock(_mutex); return getLoggerData().id(); }
      static void setMaxLineCount(unsigned int maxLineCount);
      static void setMaxLineCount(unsigned int maxLineCount, LocalLoggerId loggerId);
      static void setMaxByteCount(unsigned int maxByteCount);
      static void setMaxByteCount(unsigned int maxByteCount, LocalLoggerId loggerId);
      static Level toLevel(const Data& l);
      static Type toType(const Data& t);
      static Data toString(Level l);

      /// DEPRECATED! Left for backward compatibility - use localLoggers instead
      static void setServiceLevel(int service, Level l);
      static Level getServiceLevel(int service);

      /// DEPRECATED! Left for backward compatibility - use localLoggers instead
      static const ThreadSetting* getThreadSetting();
      static void setThreadSetting(ThreadSetting info);
      static void setThreadSetting(int serv, Level l);
      static void setThreadSetting(int serv);

      /// Create new logger instance and return its ID (zero on error)
      static LocalLoggerId localLoggerCreate(Type type,
                                             Level level,
                                             const char * logFileName = NULL,
                                             ExternalLogger* externalLogger = NULL);

      /** Reinitialize all new setting for a local logger instance
      * @retval 0 on success
      * @retval 1 if logger does not exist
      */
      static int localLoggerReinitialize(LocalLoggerId loggerId,
                                         Type type,
                                         Level level,
                                         const char * logFileName = NULL,
                                         ExternalLogger* externalLogger = NULL);						

      /** Destroy existing logger instance.
      * @retval 0 on success
      * @retval 1 if logger does not exist
      * @retval 2 if logger is still in use
      * @retval >2 on other failures
      */
      static int localLoggerRemove(LocalLoggerId loggerId);

      /** Set logger instance with given ID as a thread local logger.
      * Pass zero \p loggerId to remove thread local logger.
      * @retval 0 on success
      * @retval 1 if logger does not exist
      * @retval >1 on other failures
      */
      static int setThreadLocalLogger(LocalLoggerId loggerId);


      static std::ostream& Instance(unsigned int bytesToWrite);
      static bool isLogging(Log::Level level, const Subsystem&);
      static void OutputToWin32DebugWindow(const Data& result);      
      static void reset(); ///< Frees logger stream
#ifndef WIN32
      static void droppingPrivileges(uid_t uid, pid_t pid);
#endif

   public:
      static unsigned int MaxLineCount; 
      static unsigned int MaxByteCount; 

   protected:
      static Mutex _mutex;
      static volatile short touchCount;
      static const Data delim;

      class ThreadData
      {
         public:
            ThreadData(LocalLoggerId id, Type type=Cout, Level level=Info,
                       const char *logFileName=NULL,
                       ExternalLogger *pExternalLogger=NULL)
               : mLevel(level),
                 mMaxLineCount(0),
                 mMaxByteCount(0),
                 mExternalLogger(pExternalLogger),
                 mId(id),
                 mType(type),
                 mLogger(NULL),
                 mLineCount(0)
            {
               if (logFileName)
               {
                  mLogFileName = logFileName;
               }
            }
            ~ThreadData() { reset(); }

            void set(Type type=Cout, Level level=Info,
                     const char *logFileName=NULL,
                     ExternalLogger *pExternalLogger=NULL)
            {
               mType = type;
               mLevel = level;

               if (logFileName)
               {
                  mLogFileName = logFileName;
               }
               mExternalLogger = pExternalLogger;
            }

            LocalLoggerId id() const {return mId;}
            unsigned int maxLineCount() { return mMaxLineCount ? mMaxLineCount : MaxLineCount; }  // return local max, if not set use global max
            unsigned int maxByteCount() { return mMaxByteCount ? mMaxByteCount : MaxByteCount; }  // return local max, if not set use global max
            Type type() const {return mType;}

            std::ostream& Instance(unsigned int bytesToWrite); ///< Return logger stream instance, creating it if needed.
            void reset(); ///< Frees logger stream
#ifndef WIN32
            void droppingPrivileges(uid_t uid, pid_t pid);
#endif

            volatile Level mLevel;
            volatile unsigned int mMaxLineCount;
            volatile unsigned int mMaxByteCount;
            ExternalLogger* mExternalLogger;

         protected:
            friend class Guard;
            const LocalLoggerId mId;
            Type mType;
            Data mLogFileName;
            std::ostream* mLogger;
            unsigned int mLineCount;
      };

      static ThreadData mDefaultLoggerData; ///< Default logger settings.
      static Data mAppName;
      static Data mHostname;
      static int mSyslogFacility;
#ifndef WIN32
      static pid_t mPid;
#else   
      static int mPid;
#endif
      static const char mDescriptions[][32];

      static ThreadData &getLoggerData()
      {
         ThreadData* pData = static_cast<ThreadData*>(ThreadIf::tlsGetValue(*Log::mLocalLoggerKey));
         return pData?*pData:mDefaultLoggerData;
      }

      /// Thread Local logger settings storage
      class LocalLoggerMap
      {
      public:
         LocalLoggerMap()
            : mLastLocalLoggerId(0) {};

         /// Create new logger instance and return its ID (zero on error)
         LocalLoggerId create(Type type,
                              Level level,
                              const char * logFileName = NULL,
                              ExternalLogger* externalLogger = NULL);

         /** Reinitialize all new setting for a local logger instance
          * @retval 0 on success
          * @retval 1 if logger does not exist
          */
         int reinitialize(LocalLoggerId loggerId,
                          Type type,
                          Level level,
                          const char * logFileName = NULL,
                          ExternalLogger* externalLogger = NULL);						

         /** Remove existing logger instance from map and destroy.
         * @retval 0 on success
         * @retval 1 if logger does not exist
         * @retval 2 if logger is still in use
         * @retval >2 on other failures
         */
         int remove(LocalLoggerId loggerId);

         /** Get pointer to ThreadData for given ID and increase use counter.
         * @returns NULL if ID does not exist. */
         ThreadData *getData(LocalLoggerId loggerId);

         /// Decrease use counter for given loggerId.
         void decreaseUseCount(LocalLoggerId loggerId);

      protected:
         /// Storage for Thread Local loggers and their use-counts.
         typedef HashMap<LocalLoggerId, std::pair<ThreadData*, int> > LoggerInstanceMap;
         LoggerInstanceMap mLoggerInstancesMap;
         /// Last used LocalLoggerId
         LocalLoggerId mLastLocalLoggerId;
         /// Mutex to synchronize access to Thread Local logger settings storage
         Mutex mLoggerInstancesMapMutex;
      };

      friend void ::freeLocalLogger(void* pThreadData);
      friend class LogStaticInitializer;
      static LocalLoggerMap mLocalLoggerMap;
      static ThreadIf::TlsKey* mLocalLoggerKey;


      /// DEPRECATED! Left for backward compatibility - use localLoggers instead
#ifdef LOG_ENABLE_THREAD_SETTING
      static HashMap<ThreadIf::Id, std::pair<ThreadSetting, bool> > mThreadToLevel;
      static HashMap<int, std::set<ThreadIf::Id> > mServiceToThreads;
      static ThreadIf::TlsKey* mLevelKey;
#endif
      static HashMap<int, Level> mServiceToLevel;
};

/** @brief Interface functor for external logging.
*/
class ExternalLogger
{
public:
   virtual ~ExternalLogger() {};
   /** return true to also do default logging, false to suppress default logging. */
   virtual bool operator()(Log::Level level,
      const Subsystem& subsystem, 
      const Data& appName,
      const char* file,
      int line,
      const Data& message,
      const Data& messageWithHeaders) = 0;
};

/// Class to initialize Log class static variables.
class LogStaticInitializer {
public:
   LogStaticInitializer();
   ~LogStaticInitializer();
protected:
   static unsigned int mInstanceCounter;
    
#ifdef WIN32
   // LogStaticInitializer calls ThreadIf::tlsKeyCreate which 
   // relies on the static TlsDestructorInitializer having set
   // up mTlsDestructorsMutex which is used to Lock TLS access
   // Since the order of static initialization is not reliable,
   // we must make sure that TlsDestructorInitializer is initialized
   // before LogStaticInitializer is inizialized:
   TlsDestructorInitializer tlsDestructorInitializer;
#endif
};
static LogStaticInitializer _staticLogInit;

}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005.
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
