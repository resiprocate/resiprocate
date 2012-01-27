/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#ifndef _ES_LOGGER_HXX
#define _ES_LOGGER_HXX 1

#include "rutil/compat.hxx"
#include "log4cplus/logger.h"
#include "rutil/ResipLogAdapter.hxx"

#include <string>

/**
   @file
   @brief Defines logging macros that use log4cplus.
   @code
   #include "EsLogger.hxx"
   // ...
   ES_DEBUG_BYNAME("my.custom.logger", <<"This sends a debug statement to a custom log4cplus logger.");
   ES_DEBUG_BYNAME("sipbasis.foundation.oldlog", <<"This sends a debug statement to the resip logging system.");
   int sparrows = 3;
   ES_DEBUG_BYNAME("my.custom.logger", <<"Current Number Of Sparrows:"<<sparrows);
   // ...
   @endcode
*/

/**
   @brief trace level logging macro
   @param name logging context name
   @param logEvent event to log
   @code
   #include "EsLogger.hxx"
   // ...
   void methodFoo(){
       //ES_TRACE_METHOD will print a logging statement here from methodFoo : entering
       ES_TRACE_METHOD("my.custom.trace","methodFoo");
       ES_TRACE_BYNAME("my.custom.trace","In the body of Foo.");
       //ES_TRACE_METHOD will print a logging statement here from methodFoo : exiting
   }
   void methodBar(){
       //ES_TRACE_METHOD will print a logging statement here from methodBar : entering
       ES_TRACE_METHOD("my.custom.trace","methodBar");
       ES_TRACE_BYNAME("my.custom.trace","In the body of Bar.");
       ES_TRACE_BYNAME("my.custom.trace","Calling Foo from Bar.");
       methodFoo(); // Calling method foo
       ES_TRACE_BYNAME("my.custom.trace","Returned from Foo into the body of Bar.");
       //ES_TRACE_METHOD will print a logging statement here from methodBar : exiting
   }
   // ...
   // Logging order for a call to methodBar with trace level logging activated would look
   // something like the following (exact appearance will vary with customization)
   // "Entering Bar"
   // "In the body of Bar."
   // "Calling Foo from Bar."
   // "Entering Foo"
   // "In the body of Foo."
   // "Exiting Foo"
   // "Returned from Foo into the body of Bar."
   // "Exiting Bar"
   @endcode

*/
#define ES_TRACE_METHOD_BYNAME( name, method ) LOG4CPLUS_TRACE_METHOD( log4cplus::Logger::getInstance(name) , method )
#define ES_TRACE_BYNAME(name,logEvent) LOG4CPLUS_TRACE( log4cplus::Logger::getInstance(name), logEvent )
#define ES_DEBUG_BYNAME(name,logEvent) LOG4CPLUS_DEBUG( log4cplus::Logger::getInstance(name), logEvent )
#define ES_INFO_BYNAME(name,logEvent)  LOG4CPLUS_INFO(log4cplus::Logger::getInstance(name), logEvent )
#define ES_WARN_BYNAME(name,logEvent)  LOG4CPLUS_WARN(  log4cplus::Logger::getInstance(name), logEvent )
#define ES_ERROR_BYNAME(name,logEvent) LOG4CPLUS_ERROR( log4cplus::Logger::getInstance(name), logEvent )
#define ES_FATAL_BYNAME(name,logEvent) LOG4CPLUS_FATAL( log4cplus::Logger::getInstance(name), logEvent )
#define ES_EVENT_BYNAME(name,logEvent) LOG4CPLUS_INFO( log4cplus::Logger::getInstance(name), logEvent )

#define ES_TRACE_METHOD(logger,logEvent) LOG4CPLUS_TRACE_METHOD(logger, logEvent)
#define ES_TRACE(logger,logEvent) LOG4CPLUS_TRACE(logger, logEvent)
#define ES_DEBUG(logger,logEvent) LOG4CPLUS_DEBUG(logger, logEvent)
#define ES_INFO(logger,logEvent)  LOG4CPLUS_INFO(logger, logEvent)
#define ES_WARN(logger,logEvent)  LOG4CPLUS_WARN(logger, logEvent)
#define ES_ERROR(logger,logEvent) LOG4CPLUS_ERROR(logger, logEvent)
#define ES_FATAL(logger,logEvent) LOG4CPLUS_FATAL(logger, logEvent)
#define ES_EVENT(logger,logEvent) LOG4CPLUS_INFO(logger, logEvent)

#define ES_SET_CONTEXT( context ) log4cplus::NDCContextCreator _context( context );

namespace estacado
{
class ResipLogAdapter;

static const std::string SBF_OLDLOG_NAME("sipbasis.foundation.oldlog");
static const std::string SBF_SIPMESSAGES_NAME("sipbasis.foundation.sipmessages");
static const std::string SBF_PERSISTENCE_NAME("sipbasis.foundation.persistence");
static const std::string SBF_LOGGING_NAME("sipbasis.foundation.logging");
static const std::string SBF_STATISTICS_NAME("sipbasis.foundation.statistics");
static const std::string SBF_QUEUEING_NAME("sipbasis.foundation.queueing");

static log4cplus::Logger SBF_OLDLOG(log4cplus::Logger::getInstance(SBF_OLDLOG_NAME));
static log4cplus::Logger SBF_SIPMESSAGES(log4cplus::Logger::getInstance(SBF_SIPMESSAGES_NAME));
static log4cplus::Logger SBF_PERSISTENCE(log4cplus::Logger::getInstance(SBF_PERSISTENCE_NAME));
static log4cplus::Logger SBF_LOGGING(log4cplus::Logger::getInstance(SBF_LOGGING_NAME));
static log4cplus::Logger SBF_STATISTICS(log4cplus::Logger::getInstance(SBF_STATISTICS_NAME));
static log4cplus::Logger SBF_QUEUEING(log4cplus::Logger::getInstance(SBF_QUEUEING_NAME));


/**
   @brief Initializer class for log4cplus.
   @ingroup logging
*/
class EsLogger
{
   public:
      static int init(std::string initFileName, int argc, char *argv[], log4cplus::LogLevel rootLevel=log4cplus::DEBUG_LOG_LEVEL);
   
   private:
      EsLogger(){};
      static ResipLogAdapter mResipLogAdapter;
};


} 
#endif // _ES_LOGGER_HXX not defined

/* Copyright 2007 Estacado Systems */
