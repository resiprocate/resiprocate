#include <cassert>

#include "UserAgent.hxx"
#include "UserAgentServerAuthManager.hxx"

#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/ServerAuthManager.hxx>
#include <resip/dum/UserAuthInfo.hxx>
#include <resip/dum/UserProfile.hxx>
#include <rutil/MD5Stream.hxx>
#include <rutil/WinLeakCheck.hxx>

#define RESIPROCATE_SUBSYSTEM Subsystem::USERAGENT

using namespace useragent;
using namespace resip;

UserAgentServerAuthManager::UserAgentServerAuthManager(UserAgent& userAgent) :
   ServerAuthManager(userAgent.getDialogUsageManager(), userAgent.getDialogUsageManager().dumIncomingTarget()),
   mUserAgent(userAgent)
{
}

UserAgentServerAuthManager::~UserAgentServerAuthManager()
{
}

bool 
UserAgentServerAuthManager::useAuthInt() const
{
   return true;
}

bool 
UserAgentServerAuthManager::proxyAuthenticationMode() const
{
   return false;  // Challenge with 401
}

const Data& 
UserAgentServerAuthManager::getChallengeRealm(const SipMessage& msg)
{
   return mUserAgent.getIncomingConversationProfile(msg)->getDefaultFrom().uri().host();
}

bool 
UserAgentServerAuthManager::isMyRealm(const Data& realm)
{
   return true;  // .slg. this means we will try to find credentials for any authorization headers 
                 // could improve this by looking through all active conversation profiles to see if realm exists
}

bool 
UserAgentServerAuthManager::authorizedForThisIdentity(const resip::Data &user, 
                                                      const resip::Data &realm, 
                                                      resip::Uri &fromUri)
{
   return true;  // We don't care who the request came from
}

ServerAuthManager::AsyncBool
UserAgentServerAuthManager::requiresChallenge(const SipMessage& msg)
{
   assert(msg.isRequest());
   ConversationProfile* profile = mUserAgent.getIncomingConversationProfile(msg).get();

   // We want to challenge OOD Refer requests and Invite Requests with Auto-Answer indications
   switch(msg.method())
   {
   case REFER:
      if(profile->challengeOODReferRequests() && !msg.header(h_To).exists(p_tag))
      {
         // Don't challenge OOD Refer requests have a valid TargetDialog header
         if(!msg.exists(h_TargetDialog) || mUserAgent.getDialogUsageManager().findInviteSession(msg.header(h_TargetDialog)).first == InviteSessionHandle::NotValid())
         {
            return True;
         }
      }
      break;

   case INVITE:
      if(profile->challengeAutoAnswerRequests() && profile->shouldAutoAnswer(msg))
      {
         return True;
      }
      break;
   default:
      break;
   }

   // Default to not challenge
   return False;
}

void 
UserAgentServerAuthManager::requestCredential(const Data& user, 
                                              const Data& realm, 
                                              const SipMessage& msg,
                                              const Auth& auth,
                                              const Data& transactionId )
{
   const UserProfile::DigestCredential& digestCredential = 
         mUserAgent.getIncomingConversationProfile(msg)->getDigestCredential(realm);

   MD5Stream a1;
   a1 << digestCredential.user
      << Symbols::COLON     
      << digestCredential.realm
      << Symbols::COLON
      << digestCredential.password;
   a1.flush();
   UserAuthInfo* userAuthInfo = new UserAuthInfo(user,realm,a1.getHex(),transactionId);
   mUserAgent.getDialogUsageManager().post( userAuthInfo );      
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
