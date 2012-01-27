/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#include "rutil/XMLLayout.hxx"
#include "rutil/SimpleXMLElement.hxx"
#include "rutil/Data.hxx"
#include <string>
#include <log4cplus/streams.h>
#include <log4cplus/layout.h>
#include <log4cplus/loglevel.h>
#include <log4cplus/helpers/property.h>
#include <log4cplus/spi/loggingevent.h>

namespace resip{
//String for formatting timestamps
const char * RFC_822_TIME_FORMAT ="%d %m %y %H:%M:%S %Z";

void XMLLayout::formatAndAppend (log4cplus::tostream& output,  
            const log4cplus::spi::InternalLoggingEvent& event)
        {
            //Create Character Representations of numeric data
             resip::Data line = resip::Data(event.getLine());
            // Create the new element
             resip::SimpleXMLElement element("EVENT");
            // Create set the attributes
             element.addAttribute("NDC",event.getNDC().c_str());
             element.addAttribute("level", 
              log4cplus::getLogLevelManager().toString(event.getLogLevel()).c_str()
             );
             element.addAttribute("thread",event.getThread().c_str());
             element.addAttribute("logger",event.getLoggerName().c_str());
             element.addAttribute("file",event.getFile().c_str());
             element.addAttribute("line",line.c_str());
             element.addAttribute("time-stamp", 
             event.getTimestamp().getFormattedTime
                (RFC_822_TIME_FORMAT).c_str());
            //Insert the log message as the body
            element.setBody(event.getMessage().c_str());
            //Transform the log message to a string and stream it out
            std::string * results = element.toString();
            output<<(*results);
            delete results;
        }
}


/* Copyright 2007 Estacado Systems */
