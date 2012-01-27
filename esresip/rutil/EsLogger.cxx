/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#include "rutil/EsLogger.hxx"
#include "rutil/Log.hxx"

#include <stdlib.h>
#include <string>

#include "log4cplus/configurator.h"
#include "log4cplus/layout.h"
#include "log4cplus/appender.h"
#include "log4cplus/fileappender.h"



namespace estacado
{
estacado::ResipLogAdapter estacado::EsLogger::mResipLogAdapter;

int estacado::EsLogger::init (std::string initFileName, int argc, char *argv[], log4cplus::LogLevel rootLevel) 
{

    const char *methodName = "estacado::EsLogger::init()";
    if (initFileName.length() <= 0) 
    {
        initFileName = "log4cplus.properties";
    }
    
    char* srcdir = ::getenv("srcdir");
    if (srcdir != 0) 
    {
        // ronh: The / in the path below will need to be localized to the OS eventually.
        initFileName = std::string(srcdir) +"/"+ initFileName;
    } 

   log4cplus::PropertyConfigurator::doConfigure(initFileName);
   log4cplus::Logger::getRoot().setLogLevel(rootLevel);


   //!bwc! defaults if config file doesn't provide appenders (or loggers)

   if(estacado::SBF_SIPMESSAGES.getAllAppenders().size()==0)
   {
      log4cplus::RollingFileAppender* sipMsgAppender= new log4cplus::RollingFileAppender("sipmessages.log");
      log4cplus::Layout* sipMsgLayout = new log4cplus::TTCCLayout();
      sipMsgAppender->setLayout(std::auto_ptr<log4cplus::Layout>(sipMsgLayout));
      estacado::SBF_SIPMESSAGES.addAppender(sipMsgAppender);
   }

   if(estacado::SBF_LOGGING.getAllAppenders().size()==0)
   {
      log4cplus::RollingFileAppender* loggingAppender= new log4cplus::RollingFileAppender("logging.log");
      log4cplus::Layout* loggingLayout = new log4cplus::TTCCLayout();
      loggingAppender->setLayout(std::auto_ptr<log4cplus::Layout>(loggingLayout));
      estacado::SBF_LOGGING.addAppender(loggingAppender);
   }

   if(estacado::SBF_OLDLOG.getAllAppenders().size()==0)
   {
      log4cplus::RollingFileAppender* oldAppender= new log4cplus::RollingFileAppender("oldresip.log");
      log4cplus::Layout* oldLayout = new log4cplus::TTCCLayout();
      oldAppender->setLayout(std::auto_ptr<log4cplus::Layout>(oldLayout));
      estacado::SBF_OLDLOG.addAppender(oldAppender);
   }

   if(estacado::SBF_STATISTICS.getAllAppenders().size()==0)
   {
      log4cplus::RollingFileAppender* oldAppender= new log4cplus::RollingFileAppender("statistics.log");
      log4cplus::Layout* oldLayout = new log4cplus::TTCCLayout();
      oldAppender->setLayout(std::auto_ptr<log4cplus::Layout>(oldLayout));
      estacado::SBF_STATISTICS.addAppender(oldAppender);
   }

    ES_TRACE(estacado::SBF_LOGGING,  methodName << "log4cplus config file is " << initFileName << std::endl );

    ES_INFO(estacado::SBF_LOGGING, "configuring reSIProcate logging system to \"" << estacado::SBF_OLDLOG_NAME << "\"" );
    log4cplus::LogLevel logLevel = rootLevel;
    resip::Log::Level resipLogLevel;
    switch (logLevel)
    {
        case log4cplus::FATAL_LOG_LEVEL :
            resipLogLevel = resip::Log::Crit;
            break;
        case log4cplus::ERROR_LOG_LEVEL :
            resipLogLevel = resip::Log::Err;
            break;
        case log4cplus::WARN_LOG_LEVEL :
            resipLogLevel = resip::Log::Warning;
            break;
        case log4cplus::INFO_LOG_LEVEL :
            resipLogLevel = resip::Log::Info;
            break;
        case log4cplus::DEBUG_LOG_LEVEL :
            resipLogLevel = resip::Log::Debug;
            break;
        case log4cplus::TRACE_LOG_LEVEL :
            resipLogLevel = resip::Log::Stack;
            break;
        default :
            resipLogLevel = resip::Log::Stack;
    }
    ES_TRACE(estacado::SBF_LOGGING, "matching log4cplus loglevel (" << logLevel <<
                    ") to reSIP loglevel (" << resipLogLevel << ")");

    resip::Log::initialize(resip::Log::Cout, resipLogLevel, argc ? argv[0] : 0, estacado::EsLogger::mResipLogAdapter);

    ES_INFO(estacado::SBF_LOGGING, "done configuring reSIProcate logging system" );

    return 1;
} // of init

}

/* Copyright 2007 Estacado Systems */
