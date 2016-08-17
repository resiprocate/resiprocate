#include "UserAgent.hxx"
#include "ReconSubsystem.hxx"
#include "UserAgentClientPublication.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/ClientPublication.hxx>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

UserAgentClientPublication::UserAgentClientPublication(UserAgent& userAgent, DialogUsageManager& dum, unsigned int handle)
: AppDialogSet(dum),
  mUserAgent(userAgent),
  mDum(dum),
  mPublicationHandle(handle),
  mEnded(false)
{
   mUserAgent.registerPublication(this);
}

UserAgentClientPublication::~UserAgentClientPublication()
{
   mUserAgent.unregisterPublication(this);
}

PublicationHandle 
UserAgentClientPublication::getPublicationHandle()
{
   return mPublicationHandle;
}

void 
UserAgentClientPublication::end()
{
   if(!mEnded)
   {
      mEnded = true;
      AppDialogSet::end();
   }
}

////////////////////////////////////////////////////////////////////////////////
// ClientPublicationHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
UserAgentClientPublication::onSuccess(ClientPublicationHandle handle, const SipMessage& status)
{
   InfoLog(<<"UserAgentClientPublication::onSuccess - not implemented\n");
}

void
UserAgentClientPublication::onRemove(ClientPublicationHandle handle, const SipMessage& status)
{
   InfoLog(<<"UserAgentClientPublication::onRemove - not implemented\n");
}

void
UserAgentClientPublication::onFailure(ClientPublicationHandle handle, const SipMessage& status)
{
   InfoLog(<<"UserAgentClientPublication::onFailure - not implemented\n");
}

int
UserAgentClientPublication::onRequestRetry(ClientPublicationHandle handle, int retrySeconds, const SipMessage& status)
{
   InfoLog(<<"UserAgentClientPublication::onRequestRetry - not implemented\n");
   return 30;
}

void
UserAgentClientPublication::onStaleUpdate(ClientPublicationHandle handle, const SipMessage& status)
{
   InfoLog(<<"UserAgentClientPublication::onStaleUpdate - not implemented\n");
}

/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 Copyright (C) 2016, Mateus Bellomo (mateusbellomo AT gmail DOT com) https://mateusbellomo.wordpress.com/
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of the author(s) nor the names of its contributors 
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
