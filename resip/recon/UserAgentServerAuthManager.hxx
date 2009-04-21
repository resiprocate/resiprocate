#if !defined(UserAgentServerAuthManager_hxx)
#define UserAgentServerAuthManager_hxx

#include <map>

#include <resip/stack/Auth.hxx>
#include <resip/stack/Message.hxx>
#include <resip/dum/UserProfile.hxx>
#include <resip/dum/ServerAuthManager.hxx>

namespace recon
{
class UserAgent;

/**
  This class is used to provide server digest authentication
  capabilities.  It uses the profile settings in order to determine
  if a SIP request should be challenged or not, and challenges 
  appropriately.

  It is used to challenge auto-answer requests and OOD refer
  requests.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class UserAgentServerAuthManager: public resip::ServerAuthManager
{
   public:
      UserAgentServerAuthManager(UserAgent& userAgent);
      ~UserAgentServerAuthManager();
      
   protected:
      // this call back should async cause a post of UserAuthInfo
      virtual void requestCredential(const resip::Data& user, 
                                     const resip::Data& realm, 
                                     const resip::SipMessage& msg,
                                     const resip::Auth& auth,
                                     const resip::Data& transactionId );
      
      virtual bool useAuthInt() const;
      virtual bool proxyAuthenticationMode() const;
      virtual const resip::Data& getChallengeRealm(const resip::SipMessage& msg);   
      virtual bool isMyRealm(const resip::Data& realm);
      virtual bool authorizedForThisIdentity(const resip::Data &user, 
                                             const resip::Data &realm, 
                                             resip::Uri &fromUri);
      virtual AsyncBool requiresChallenge(const resip::SipMessage& msg);

   private:
      UserAgent& mUserAgent;
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
