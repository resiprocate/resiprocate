#include <resip/stack/Tuple.hxx>
#include <resip/stack/SipMessage.hxx>
#include <resip/stack/ExtensionParameter.hxx>

#include "ReconSubsystem.hxx"
#include "ConversationProfile.hxx"

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

static const resip::ExtensionParameter p_answerafter("answer-after");
static const resip::ExtensionParameter p_required("required");

ConversationProfile::ConversationProfile() :
   mHandle(0),
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
   mHandle(0),
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

ConversationProfileHandle 
ConversationProfile::getHandle()
{
    return mHandle;
}

void
ConversationProfile::setHandle(ConversationProfileHandle handle)
{
    resip_assert(mHandle==0);
    mHandle = handle;
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
   resip_assert(inviteRequest.method() == INVITE);
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
