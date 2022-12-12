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

namespace recon
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

      ConversationProfileHandle getConversationProfileHandle();
      virtual void end();
      virtual bool forceRefresh();

      const resip::NameAddrs& getContactAddresses();
      const resip::Tuple& getLastServerTuple();

      // Registration Handler ////////////////////////////////////////////////////////
      virtual void onSuccess(resip::ClientRegistrationHandle h, const resip::SipMessage& response);
      virtual void onFailure(resip::ClientRegistrationHandle h, const resip::SipMessage& response);
      virtual void onRemoved(resip::ClientRegistrationHandle h, const resip::SipMessage& response);
      virtual int onRequestRetry(resip::ClientRegistrationHandle h, int retryMinimum, const resip::SipMessage& msg);

   private:       
      UserAgent &mUserAgent;
      ConversationProfileHandle mConversationProfileHandle;
      bool mEnded;
      resip::ClientRegistrationHandle mRegistrationHandle;  
      resip::Tuple mLastServerTuple;
};

}

#endif


/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 Copyright (c) 2016, SIP Spectrum, Inc.

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
