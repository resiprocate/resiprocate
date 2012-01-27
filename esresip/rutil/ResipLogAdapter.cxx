/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#include "rutil/ResipLogAdapter.hxx"
#include "rutil/EsLogger.hxx"


namespace estacado
{

estacado::ResipLogAdapter::ResipLogAdapter()
{
}

estacado::ResipLogAdapter::~ResipLogAdapter()
{
}

bool estacado::ResipLogAdapter::operator()(resip::Log::Level level,
                                const resip::Subsystem& subsystem,
                                const resip::Data& appName,
                                const char* file,
                                int line,
                                const resip::Data& message,
                                const resip::Data& messageWithHeaders)
{
    switch (level)
    {
        case resip::Log::Crit:     
        ES_FATAL( estacado::SBF_OLDLOG, file << ":" << line << " <> " << message );
        break;
        case resip::Log::Err:      
        ES_ERROR( estacado::SBF_OLDLOG, file << ":" << line << " <> " << message );
        break;
        case resip::Log::Warning:  
        // we need to divert statistics from reSIP to a different category
        //This is something that was needed in stateAgent, but could come in handy later.
    /*            if ((resip::Data::npos != message.find("Transaction summary:"))
            && (resip::Data::npos != message.find("TU summary:"))
            && (resip::Data::npos != message.find("Details:")) )
        {
            ES_WARN ( estacado::SBF_STATISTICS, file << ":" << line << " <> " << message );
        }
        else*/
        {
            ES_WARN ( estacado::SBF_OLDLOG, file << ":" << line << " <> " << message );
        }
        break;
        case resip::Log::Info:     
        ES_INFO ( estacado::SBF_OLDLOG, file << ":" << line << " <> " << message );
        break;
        case resip::Log::Debug:    
        ES_DEBUG( estacado::SBF_OLDLOG, file << ":" << line << " <> " << message );
        break;
        default:                   
        ES_TRACE( estacado::SBF_OLDLOG, file << ":" << line << " <> " << message );
    } // of switch
    
    return false;


}
}

/* Copyright 2007 Estacado Systems */
