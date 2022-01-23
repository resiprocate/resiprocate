#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ConversationManager.hxx"

#include "RemoteIMSessionParticipant.hxx"
#include "Conversation.hxx"
#include "UserAgent.hxx"
#include "ReconSubsystem.hxx"

#include <rutil/ResipAssert.h>
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/Random.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/ClientInviteSession.hxx>
#include <resip/dum/ServerInviteSession.hxx>
#include <resip/dum/ClientSubscription.hxx>
#include <resip/dum/ServerOutOfDialogReq.hxx>
#include <resip/dum/ServerSubscription.hxx>

#include <rutil/WinLeakCheck.hxx>

#include <utility>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

// UAC
RemoteIMSessionParticipant::RemoteIMSessionParticipant(ParticipantHandle partHandle,
                                     ConversationManager& conversationManager,
                                     DialogUsageManager& dum,
                                     RemoteParticipantDialogSet& remoteParticipantDialogSet)
: Participant(partHandle, conversationManager),
  RemoteParticipant(partHandle, conversationManager, dum, remoteParticipantDialogSet)
{
   InfoLog(<< "RemoteIMSessionParticipant created (UAC), handle=" << mHandle);
}

// UAS - or forked leg
RemoteIMSessionParticipant::RemoteIMSessionParticipant(ConversationManager& conversationManager,
                                     DialogUsageManager& dum, 
                                     RemoteParticipantDialogSet& remoteParticipantDialogSet)
: Participant(conversationManager),
  RemoteParticipant(conversationManager, dum, remoteParticipantDialogSet)
{
   InfoLog(<< "RemoteIMSessionParticipant created (UAS or forked leg), handle=" << mHandle);
}

RemoteIMSessionParticipant::~RemoteIMSessionParticipant()
{
   // Note:  Ideally this call would exist in the Participant Base class - but this call requires 
   //        dynamic_casts and virtual methods to function correctly during destruction.
   //        If the call is placed in the base Participant class then these things will not
   //        function as desired because a classes type changes as the descructors unwind.
   //        See https://stackoverflow.com/questions/10979250/usage-of-this-in-destructor.
   unregisterFromAllConversations();

   InfoLog(<< "RemoteIMSessionParticipant destroyed, handle=" << mHandle);
}

void
RemoteIMSessionParticipant::notifyIncomingParticipant(const resip::SipMessage& msg, bool autoAnswer, ConversationProfile& conversationProfile)
{
   if (mHandle) mConversationManager.onIncomingIMSessionParticipant(mHandle, msg, autoAnswer, conversationProfile);
}

void
RemoteIMSessionParticipant::buildSdpOffer(bool holdSdp, SdpContents& offer)
{
   ConversationProfile* profile = getDialogSet().getConversationProfile().get();
   resip_assert(profile);

   offer = profile->sessionCaps();

   // Set sessionid and version for this sdp
   UInt64 currentTime = Timer::getTimeMicroSec();
   offer.session().origin().getSessionId() = currentTime;
   offer.session().origin().getVersion() = currentTime;

   offer.session().media().clear();  // Clear any media lines and add in one approproate for message sessions

   // Added Message Session medium (ala. Microsoft): https://docs.microsoft.com/en-us/openspecs/office_protocols/ms-confim/70925df9-ee05-4f8f-9eda-e22ef32fd414
   SdpContents::Session::Medium medium("message", 5060, 1, "sip");
   medium.addFormat("null");
   offer.session().addMedium(medium);

   setProposedSdp(offer);
}

bool
RemoteIMSessionParticipant::buildSdpAnswer(const SdpContents& offer, SdpContents& answer)
{
   bool valid = false;

   try
   {
      ConversationProfile* profile = getDialogSet().getConversationProfile().get();
      assert(profile);

      answer = profile->sessionCaps();

      UInt64 currentTime = Timer::getTimeMicroSec();
      answer.session().origin().getSessionId() = currentTime;
      answer.session().origin().getVersion() = currentTime;

      // Copy t= field from sdp (RFC3264)
      if (offer.session().getTimes().size() >= 1)
      {
         answer.session().getTimes().clear();
         answer.session().addTime(offer.session().getTimes().front());
      }

      // Clear any media lines and add in media line responses below
      answer.session().media().clear();

      // Loop through each offered m= line and provide a response
      // We leave our offer m=message line in and use this as the response if the offer contains a valid m=message line as well.
      for(auto it = offer.session().media().begin(); it != offer.session().media().end(); it++)
      {
         if (!valid && it->name() == "message" && it->port() == 5060 && it->protocol() == "sip")
         {
            // Added Message Session medium (ala. Microsoft): https://docs.microsoft.com/en-us/openspecs/office_protocols/ms-confim/70925df9-ee05-4f8f-9eda-e22ef32fd414
            SdpContents::Session::Medium medium("message", 5060, 1, "sip");
            medium.addFormat("null");
            answer.session().addMedium(medium);

            valid = true;
         }
         else
         {
            SdpContents::Session::Medium rejmedium(it->name(), 0, 1,  // Reject medium by specifying port 0 (RFC3264)	
               it->protocol());
            if (it->codecs().size() > 0)
            {
               rejmedium.addCodec(it->codecs().front());
            }
            answer.session().addMedium(rejmedium);
         }
      }  // end loop through m= offers
   }
   catch(BaseException &e)
   {
      WarningLog( << "buildSdpAnswer: exception parsing SDP offer: " << e.getMessage());
      valid = false;
   }
   catch(...)
   {
      WarningLog( << "buildSdpAnswer: unknown exception parsing SDP offer");
      valid = false;
   }

   //InfoLog( << "SDPOffer: " << offer);
   //InfoLog( << "SDPAnswer: " << answer);
   if(valid)
   {
      setLocalSdp(answer);
      setRemoteSdp(offer);
   }
   return valid;
}


/* ====================================================================

 Copyright (c) 2022, SIP Spectrum, Inc.  http://www.sipspectrum.com
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
