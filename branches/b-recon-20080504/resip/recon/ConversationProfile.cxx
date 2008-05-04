#include <resip/stack/Tuple.hxx>
#include <resip/stack/SipMessage.hxx>
#include <resip/stack/ExtensionParameter.hxx>

#include "UserAgentSubsystem.hxx"
#include "ConversationProfile.hxx"

using namespace useragent;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM UserAgentSubsystem::USERAGENT

static const resip::ExtensionParameter p_answerafter("answer-after");
static const resip::ExtensionParameter p_required("required");

ConversationProfile::ConversationProfile() :
   mAllowAutoAnswer(false),
   mAllowPriorityAutoAnswer(false),
   mChallengeAutoAnswerRequests(false),
   mChallengeOODReferRequests(true),
   mSecureMediaMode(Srtp),
   mSecureMediaRequired(false),
   mDefaultSecureMediaCryptoSuite(SRTP_AES_CM_128_HMAC_SHA1_80),
   mNatTraversalMode(NoNatTraversal),
   mNatTraversalServerPort(0)
{
}

ConversationProfile::ConversationProfile(SharedPtr<Profile> baseProfile) :
   UserProfile(baseProfile),
   mAllowAutoAnswer(false),
   mAllowPriorityAutoAnswer(false),
   mChallengeAutoAnswerRequests(false),
   mChallengeOODReferRequests(true),
   mSecureMediaMode(Srtp),
   mSecureMediaRequired(false),
   mDefaultSecureMediaCryptoSuite(SRTP_AES_CM_128_HMAC_SHA1_80),
   mNatTraversalMode(NoNatTraversal),
   mNatTraversalServerPort(0)
{
}

SdpContents& 
ConversationProfile::sessionCaps()
{
   return mSessionCaps;
}

const SdpContents 
ConversationProfile::sessionCaps() const
{
   return mSessionCaps;
}

bool 
ConversationProfile::shouldAutoAnswer(const SipMessage& inviteRequest, bool *required)
{
   assert(inviteRequest.method() == INVITE);
   bool shouldAutoAnswer = false;
   bool autoAnswerRequired = false;
   if(inviteRequest.exists(h_PrivAnswerMode) && inviteRequest.header(h_PrivAnswerMode).value() == "Auto")
   {
      if(allowPriorityAutoAnswer())
      {
         shouldAutoAnswer = true;
      }
      if(inviteRequest.header(h_PrivAnswerMode).exists(p_required))
      {
         autoAnswerRequired = true;
      }
   }
   else if(inviteRequest.exists(h_AnswerMode) && inviteRequest.header(h_AnswerMode).value() == "Auto")
   {
      if(allowAutoAnswer())
      {         
         shouldAutoAnswer = true;
      }
      if(inviteRequest.header(h_AnswerMode).exists(p_required))
      {
         autoAnswerRequired = true;
      }
   }
   else if(allowAutoAnswer() && inviteRequest.exists(h_CallInfos)) 
   {
      // Iterate through Call-Info headers and look for answer-after=0 parameter
      for(GenericUris::const_iterator i = inviteRequest.header(h_CallInfos).begin(); i != inviteRequest.header(h_CallInfos).end(); i++)
      {
         if(i->exists(p_answerafter) && i->param(p_answerafter) == "0")
         {
            shouldAutoAnswer = true;
         }
      }
   }

   if(required) 
   {
      *required = autoAnswerRequired;
   }
   return shouldAutoAnswer;
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
