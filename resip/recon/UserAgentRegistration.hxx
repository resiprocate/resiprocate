#if !defined(UserAgentRegistration_hxx)
#define UserAgentRegistration_hxx

#include <resip/dum/AppDialogSet.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/DialogSetHandler.hxx>
#include <resip/dum/SubscriptionHandler.hxx>

#include "UserAgent.hxx"

namespace resip
{
class DialogUsageManager;
class SipMessage;
}

namespace useragent
{
class UserAgent;

/**
  This class is used to manage active SIP registrations.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class UserAgentRegistration : public resip::AppDialogSet
{
   public:  
      UserAgentRegistration(UserAgent& userAgent, resip::DialogUsageManager& dum, unsigned int handle);  
      virtual ~UserAgentRegistration();

      UserAgent::ConversationProfileHandle getConversationProfileHandle();
      virtual void end();

      const resip::NameAddrs& getContactAddresses();

      // Registration Handler ////////////////////////////////////////////////////////
      virtual void onSuccess(resip::ClientRegistrationHandle h, const resip::SipMessage& response);
      virtual void onFailure(resip::ClientRegistrationHandle h, const resip::SipMessage& response);
      virtual void onRemoved(resip::ClientRegistrationHandle h, const resip::SipMessage& response);
      virtual int onRequestRetry(resip::ClientRegistrationHandle h, int retryMinimum, const resip::SipMessage& msg);

   private:       
      UserAgent &mUserAgent;
      resip::DialogUsageManager &mDum;
      UserAgent::ConversationProfileHandle mConversationProfileHandle;
      bool mEnded;
      resip::ClientRegistrationHandle mRegistrationHandle;  
};

}

#endif


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
