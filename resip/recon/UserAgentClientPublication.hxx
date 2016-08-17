#if !defined(UserAgentClientPublication_hxx)
#define UserAgentClientPublication_hxx

#include <resip/dum/AppDialogSet.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/DialogSetHandler.hxx>
#include <resip/dum/PublicationHandler.hxx>

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
  This class is used to manage active client publications.

*/

class UserAgentClientPublication : public resip::AppDialogSet
{
   public:  
      UserAgentClientPublication(UserAgent& userAgent, resip::DialogUsageManager& dum, unsigned int handle);  
      virtual ~UserAgentClientPublication();

      PublicationHandle getPublicationHandle();
      virtual void end();

      // ClientPublicationHandler ///////////////////////////////////////////////////
      /// Called when the publication succeeds or each time it is sucessfully
      /// refreshed.
      virtual void onSuccess(resip::ClientPublicationHandle handle, const resip::SipMessage& status);

      //publication was successfully removed
      virtual void onRemove(resip::ClientPublicationHandle handle, const resip::SipMessage& status);

      //call on failure. The usage will be destroyed.  Note that this may not
      //necessarily be 4xx...a malformed 200, etc. could also reach here.
      virtual void onFailure(resip::ClientPublicationHandle handle, const resip::SipMessage& status);

      /// call on Retry-After failure.
      /// return values: -1 = fail, 0 = retry immediately, N = retry in N seconds
      virtual int onRequestRetry(resip::ClientPublicationHandle handle, int retrySeconds, const resip::SipMessage& status);

      // ?dcm? -- when should this be called
      virtual void onStaleUpdate(resip::ClientPublicationHandle handle, const resip::SipMessage& status);

   
   private:       
      UserAgent &mUserAgent;
      resip::DialogUsageManager &mDum;
      PublicationHandle mPublicationHandle;
      bool mEnded;
};

}

#endif


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
