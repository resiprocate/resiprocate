#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_SSL 

#include <functional>

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>

#include "FlowDtlsTimerContext.hxx"
#include "FlowManagerSubsystem.hxx"

using namespace flowmanager;
using namespace resip;
using namespace dtls;
using namespace std;

#define RESIPROCATE_SUBSYSTEM FlowManagerSubsystem::FLOWMANAGER

using namespace std::chrono;

FlowDtlsTimerContext::FlowDtlsTimerContext(asio::io_context& ioService) :
  mIOService(ioService),
  mStopped(false)
{
}

void
FlowDtlsTimerContext::addTimer(std::shared_ptr<dtls::DtlsTimer> timer, unsigned int durationMs)
{
   std::lock_guard<std::mutex> lock(mMutex);
   if(mStopped)
   {
      return;  // timer is freed when its owner lets it go
   }

   // The handler holds timer until it runs: when the asio timer expires, or is cancelled by stop()
   auto deadlineTimer = std::make_shared<asio::steady_timer>(mIOService);
   deadlineTimer->expires_after(milliseconds(durationMs));
   deadlineTimer->async_wait([this, timer, deadlineTimer](const asio::error_code& errorCode)
   {
      {
         std::lock_guard<std::mutex> lock(mMutex);
         mPending.erase(deadlineTimer);
      }
      if(!errorCode)
      {
         timer->fire();
      }
      else if(errorCode != asio::error::operation_aborted)
      {
         ErrLog(<< "Timer error: " << errorCode.message());
      }
   });
   mPending.insert(deadlineTimer);
}

void
FlowDtlsTimerContext::stop()
{
   {
      std::lock_guard<std::mutex> lock(mMutex);
      mStopped = true;
   }
   // An asio timer can't be used from two threads at once: cancel them on the io thread
   asio::post(mIOService, [this]()
   {
      std::lock_guard<std::mutex> lock(mMutex);
      for(const auto& deadlineTimer : mPending)
      {
         deadlineTimer->cancel();
      }
   });
}

#endif

/* ====================================================================

 Copyright (c) 2008-2026, SIP Spectrum, Inc. http://sipspectrum.com
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
