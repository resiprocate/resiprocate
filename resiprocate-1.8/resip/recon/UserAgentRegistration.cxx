#include "UserAgent.hxx"
#include "ReconSubsystem.hxx"
#include "UserAgentRegistration.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/ClientRegistration.hxx>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

UserAgentRegistration::UserAgentRegistration(UserAgent& userAgent, DialogUsageManager& dum, unsigned int handle)
: AppDialogSet(dum),
  mUserAgent(userAgent),
  mDum(dum),
  mConversationProfileHandle(handle),
  mEnded(false)
{
   mUserAgent.registerRegistration(this);
}

UserAgentRegistration::~UserAgentRegistration()
{
   mUserAgent.unregisterRegistration(this);
}

ConversationProfileHandle 
UserAgentRegistration::getConversationProfileHandle()
{
   return mConversationProfileHandle;
}

void 
UserAgentRegistration::end()
{
   if(!mEnded)
   {
      mEnded = true;
      if(mRegistrationHandle.isValid())
      {
         try
         {
            // If ended - then just shutdown registration - likely due to shutdown
            mRegistrationHandle->end();
         }
         catch(BaseException&)
         {
            // If end() call is nested - it will throw - catch here so that processing continues normally
         }
      }
   }
}

const NameAddrs& 
UserAgentRegistration::getContactAddresses()
{
   static NameAddrs empty;
   if(mRegistrationHandle.isValid())
   {
      return mRegistrationHandle->allContacts();  // .slg. note:  myContacts is not sufficient, since they do not contain the stack populated transport address
   }
   else
   {
      return empty;
   }
}

////////////////////////////////////////////////////////////////////////////////
// Registration Handler ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
UserAgentRegistration::onSuccess(ClientRegistrationHandle h, const SipMessage& msg)
{
   InfoLog(<< "onSuccess(ClientRegistrationHandle): " << msg.brief());
   if(!mEnded)
   {
      mRegistrationHandle = h;
   }
   else
   {
      try
      {
         // If ended - then just shutdown registration - likely due to shutdown
         h->end();  
      }
      catch(BaseException&)
      {
         // If end() call is nested - it will throw - catch here so that processing continues normally
      }
   }
}

void
UserAgentRegistration::onFailure(ClientRegistrationHandle h, const SipMessage& msg)
{
   InfoLog(<< "onFailure(ClientRegistrationHandle): " << msg.brief());
   if(!mEnded)
   {
      mRegistrationHandle = h;
   }
   else
   {
      try
      {
         // If we don't have a handle - then just shutdown registration - likely due to shutdown
         h->end();  
      }
      catch(BaseException&)
      {
         // If end() call is nested - it will throw - catch here so that processing continues normally
      }
   }
}

void
UserAgentRegistration::onRemoved(ClientRegistrationHandle h, const SipMessage&msg)
{
   InfoLog(<< "onRemoved(ClientRegistrationHandle): " << msg.brief());
}

int 
UserAgentRegistration::onRequestRetry(ClientRegistrationHandle h, int retryMinimum, const SipMessage& msg)
{
   InfoLog(<< "onRequestRetry(ClientRegistrationHandle): " << msg.brief());
   return -1;  // Let Profile retry setting take effect
}


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
