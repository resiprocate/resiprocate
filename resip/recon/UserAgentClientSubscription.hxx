#if !defined(UserAgentClientSubscription_hxx)
#define UserAgentClientSubscription_hxx

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
  This class is used to manage active client subscriptions.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class UserAgentClientSubscription : public resip::AppDialogSet
{
   public:  
      UserAgentClientSubscription(UserAgent& userAgent, resip::DialogUsageManager& dum, unsigned int handle);  
      virtual ~UserAgentClientSubscription();

      SubscriptionHandle getSubscriptionHandle();
      virtual void end();

      // ClientSubscriptionHandler ///////////////////////////////////////////////////
      virtual void onUpdatePending(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify, bool outOfOrder);
      virtual void onUpdateActive(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify, bool outOfOrder);
      virtual void onUpdateExtension(resip::ClientSubscriptionHandle, const resip::SipMessage& notify, bool outOfOrder);
      virtual void onTerminated(resip::ClientSubscriptionHandle h, const resip::SipMessage* notify);
      virtual void onNewSubscription(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify);
      virtual int  onRequestRetry(resip::ClientSubscriptionHandle h, int retryMinimum, const resip::SipMessage& notify);

   private:       
      void notifyReceived(const resip::Data& notifyData);

      UserAgent &mUserAgent;
      resip::DialogUsageManager &mDum;
      SubscriptionHandle mSubscriptionHandle;
      size_t mLastNotifyHash;
      bool mEnded;
};

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
