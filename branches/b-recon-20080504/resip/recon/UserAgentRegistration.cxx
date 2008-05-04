#include "UserAgent.hxx"
#include "UserAgentSubsystem.hxx"
#include "UserAgentRegistration.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/ClientRegistration.hxx>

using namespace useragent;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM UserAgentSubsystem::USERAGENT

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

UserAgent::ConversationProfileHandle 
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
