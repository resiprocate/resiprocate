#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_SSL 

#include <boost/bind.hpp>

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>

#include "FlowDtlsTimerContext.hxx"
#include "FlowManagerSubsystem.hxx"

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

#endif

/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
