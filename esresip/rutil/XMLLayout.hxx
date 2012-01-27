/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#ifndef __XMLLAYOUT__
#define __XMLLAYOUT__
#include "rutil/SimpleXMLElement.hxx"
#include <log4cplus/streams.h>
#include <log4cplus/layout.h>
#include <log4cplus/helpers/property.h>
#include <log4cplus/spi/loggingevent.h>

/**********************************
 **********************************/
namespace resip{ 

/**
   @brief This Layout produces XML elements from logging events.
   @ingroup logging
   @details This is a log4cplus layout that creates an XML document
   from a logging event.
*/
class XMLLayout: public log4cplus::Layout
{        
    public:
        /**
            @brief constructor
        */
        XMLLayout(){}
        /**
            @brief construct an instance with logging properties
            @param props the log4cplus properties to set
        */
        XMLLayout(const log4cplus::helpers::Properties &props): 
            Layout(props){}
        /**
            @brief formats a logging event into an XML document and appends
            the document to a logging stream.
            @param output the destination logging stream
            @param event the event to format
        */
        virtual void formatAndAppend (log4cplus::tostream& output,  
            const log4cplus::spi::InternalLoggingEvent& event);
    private:
        explicit XMLLayout(const XMLLayout & ):Layout(){}
        XMLLayout & operator =(const XMLLayout & ){return * this;}

};
}

#endif


/* Copyright 2007 Estacado Systems */
