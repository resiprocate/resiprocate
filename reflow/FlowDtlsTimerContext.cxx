#include "FlowDtlsTimerContext.hxx"
#include "FlowManagerSubsystem.hxx"

#include <boost/bind.hpp>

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace flowmanager;
using namespace resip;
using namespace dtls;
using namespace std;

#define RESIPROCATE_SUBSYSTEM FlowManagerSubsystem::FLOWMANAGER

FlowDtlsTimerContext::FlowDtlsTimerContext(asio::io_service& ioService) :
  mIOService(ioService) 
{
}

void 
FlowDtlsTimerContext::addTimer(dtls::DtlsTimer *timer, unsigned int durationMs) 
{
   resip::SharedPtr<asio::deadline_timer> deadlineTimer(new asio::deadline_timer(mIOService));
   deadlineTimer->expires_from_now(boost::posix_time::milliseconds(durationMs));
   deadlineTimer->async_wait(boost::bind(&FlowDtlsTimerContext::handleTimeout, this, timer, asio::placeholders::error));
   mDeadlineTimers[timer] = deadlineTimer;
   //InfoLog(<< "FlowDtlsTimerContext: starting timer for " << durationMs << "ms.");
}    

void 
FlowDtlsTimerContext::handleTimeout(dtls::DtlsTimer *timer, const asio::error_code& errorCode) 
{
   if(!errorCode)
   {
      //InfoLog(<< "FlowDtlsTimerContext: timer expired!");
      timer->fire();
   }
   else
   {
     ErrLog(<< "Timer error: " << errorCode.message());
   }
   mDeadlineTimers.erase(timer);
}


/* ====================================================================

 Original contribution Copyright (C) 2008 Plantronics, Inc.
 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */
