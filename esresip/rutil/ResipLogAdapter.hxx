/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#ifndef RESIP_LOG_ADAPTER_HXX
#define RESIP_LOG_ADAPTER_HXX 1

#include "rutil/compat.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Data.hxx"
#include "rutil/Subsystem.hxx"
#include "log4cplus/logger.h"
#include "rutil/EsLogger.hxx"
#include <string>

namespace resip
{
    class Data;
}


namespace estacado
{
   /**
      @brief Implementation of ExternalLogger that adapts logging calls to use
         the macros defined in EsLogger.hxx.
      @ingroup logging
   */
    class ResipLogAdapter : public resip::ExternalLogger
    {
    
        public:
        ResipLogAdapter();
        ~ResipLogAdapter();
        bool operator()(resip::Log::Level level,
                                const resip::Subsystem& subsystem,
                                const resip::Data& appName,
                                const char* file,
                                int line,
                                const resip::Data& message,
                                const resip::Data& messageWithHeaders);
    
    };

}
#endif

/* Copyright 2007 Estacado Systems */
