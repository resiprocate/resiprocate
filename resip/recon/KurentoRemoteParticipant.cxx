#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <media/kurento/Object.hxx>

#include "KurentoConversationManager.hxx"

#include "KurentoRemoteParticipant.hxx"
#include "Conversation.hxx"
#include "UserAgent.hxx"
#include "DtmfEvent.hxx"
#include "ReconSubsystem.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/Random.hxx>
#include <resip/stack/DtmfPayloadContents.hxx>
#include <resip/stack/SipFrag.hxx>
#include <resip/stack/ExtensionHeader.hxx>
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

/* Technically, there are a range of features that need to be implemented
   to be fully (S)AVPF compliant.
   However, it is speculated that (S)AVPF peers will communicate with legacy
   systems that just fudge the RTP/SAVPF protocol in their SDP.  Enabling
   this define allows such behavior to be tested. 

   http://www.ietf.org/mail-archive/web/rtcweb/current/msg01145.html
   "1) RTCWEB end-point will always signal AVPF or SAVPF. I signalling
   gateway to legacy will change that by removing the F to AVP or SAVP."

   http://www.ietf.org/mail-archive/web/rtcweb/current/msg04380.html
*/
//#define RTP_SAVPF_FUDGE

// UAC
KurentoRemoteParticipant::KurentoRemoteParticipant(ParticipantHandle partHandle,
                                     KurentoConversationManager& kurentoConversationManager,
                                     DialogUsageManager& dum,
                                     RemoteParticipantDialogSet& remoteParticipantDialogSet)
: Participant(partHandle, kurentoConversationManager),
  RemoteParticipant(partHandle, kurentoConversationManager, dum, remoteParticipantDialogSet),
  KurentoParticipant(partHandle, kurentoConversationManager),
  mRemoveExtraMediaDescriptors(false),
  mSipRtpEndpoint(true),
  mReuseSdpAnswer(false)
{
   InfoLog(<< "KurentoRemoteParticipant created (UAC), handle=" << mHandle);
}

// UAS - or forked leg
KurentoRemoteParticipant::KurentoRemoteParticipant(KurentoConversationManager& kurentoConversationManager,
                                     DialogUsageManager& dum, 
                                     RemoteParticipantDialogSet& remoteParticipantDialogSet)
: Participant(kurentoConversationManager),
  RemoteParticipant(kurentoConversationManager, dum, remoteParticipantDialogSet),
  KurentoParticipant(kurentoConversationManager),
  mRemoveExtraMediaDescriptors(false),
  mSipRtpEndpoint(true),
  mReuseSdpAnswer(false)
{
   InfoLog(<< "KurentoRemoteParticipant created (UAS or forked leg), handle=" << mHandle);
}

KurentoRemoteParticipant::~KurentoRemoteParticipant()
{
   // Note:  Ideally this call would exist in the Participant Base class - but this call requires 
   //        dynamic_casts and virtual methods to function correctly during destruction.
   //        If the call is placed in the base Participant class then these things will not
   //        function as desired because a classes type changes as the descructors unwind.
   //        See https://stackoverflow.com/questions/10979250/usage-of-this-in-destructor.
   unregisterFromAllConversations();

   InfoLog(<< "KurentoRemoteParticipant destroyed, handle=" << mHandle);
}

int 
KurentoRemoteParticipant::getConnectionPortOnBridge()
{
   if(getDialogSet().getActiveRemoteParticipantHandle() == mHandle)
   {
      return -1;  // FIXME Kurento
   }
   else
   {
      // If this is not active fork leg, then we don't want to effect the bridge mixer.  
      // Note:  All forked endpoints/participants have the same connection port on the bridge
      return -1;
   }
}

int 
KurentoRemoteParticipant::getMediaConnectionId()
{ 
   return getKurentoDialogSet().getMediaConnectionId();
}

kurento::BaseRtpEndpoint*
KurentoRemoteParticipant::newEndpoint()
{
   return mSipRtpEndpoint ?
            dynamic_cast<kurento::BaseRtpEndpoint*>(new kurento::SipRtpEndpoint(mKurentoConversationManager.mPipeline)) :
            dynamic_cast<kurento::BaseRtpEndpoint*>(new kurento::RtpEndpoint(mKurentoConversationManager.mPipeline));
}

void
KurentoRemoteParticipant::buildSdpOffer(bool holdSdp, ContinuationSdpReady c)
{
   // FIXME Kurento - include video, SRTP, WebRTC?

   try
   {
      bool endpointExists = true;
      bool isWebRTC = false; // FIXME - define per RemoteParticipant
      if(!mEndpoint)
      {
         endpointExists = false;
         if(isWebRTC)
         {
            // delay while ICE gathers candidates from STUN and TURN
            mIceGatheringDone = false;
            mEndpoint.reset(new kurento::WebRtcEndpoint(mKurentoConversationManager.mPipeline));
         }
         else
         {
            mEndpoint.reset(newEndpoint());
         }
      }

      // FIXME - add listeners for Kurento events

      kurento::ContinuationString cOnOfferReady = [this, holdSdp, c](const std::string& offer){
         StackLog(<<"offer FROM Kurento: " << offer);
         HeaderFieldValue hfv(offer.data(), offer.size());
         Mime type("application", "sdp");
         std::unique_ptr<SdpContents> _offer(new SdpContents(hfv, type));
         if(holdSdp)
         {
            SdpContents::Session::MediumContainer::iterator it = _offer->session().media().begin();
            for(;it != _offer->session().media().end(); it++)
            {
               SdpContents::Session::Medium& m = *it;
               if(m.exists("sendrecv"))
               {
                  m.clearAttribute("sendrecv");
                  m.addAttribute("sendonly");
               }
               if(m.exists("recvonly"))
               {
                  m.clearAttribute("recvonly");
                  m.addAttribute("inactive");
               }
            }
         }
         else
         {
            SdpContents::Session::MediumContainer::iterator it = _offer->session().media().begin();
            for(;it != _offer->session().media().end(); it++)
            {
               SdpContents::Session::Medium& m = *it;
               if(m.exists("sendonly"))
               {
                  m.clearAttribute("sendonly");
                  m.addAttribute("sendrecv");
               }
               if(m.exists("inactive"))
               {
                  m.clearAttribute("inactive");
                  m.addAttribute("recvonly");
               }
            }
         }
         setProposedSdp(*_offer);
         c(true, std::move(_offer));
      };

      kurento::ContinuationVoid cConnected = [this, isWebRTC, c, cOnOfferReady]{
         mEndpoint->generateOffer([this, isWebRTC, c, cOnOfferReady](const std::string& offer){
            if(isWebRTC)
            {
               std::shared_ptr<kurento::WebRtcEndpoint> webRtc = std::static_pointer_cast<kurento::WebRtcEndpoint>(mEndpoint);

               std::shared_ptr<kurento::EventContinuation> elIceGatheringDone =
                        std::make_shared<kurento::EventContinuation>([this, cOnOfferReady]{
                  mIceGatheringDone = true;
                  mEndpoint->getLocalSessionDescriptor(cOnOfferReady);
               });
               webRtc->addOnIceGatheringDoneListener(elIceGatheringDone, [this](){});

               webRtc->gatherCandidates([]{
                        // FIXME - handle the case where it fails
                        // on success, we continue from the IceGatheringDone event handler
               }); // gatherCandidates
            }
            else
            {
               cOnOfferReady(offer);
            }
         }); // generateOffer
      };

      if(endpointExists)
      {
         std::ostringstream offerMangledBuf;
         offerMangledBuf << *getLocalSdp();
         std::shared_ptr<std::string> offerMangledStr = std::make_shared<std::string>(offerMangledBuf.str());
         cOnOfferReady(*offerMangledStr);
      }
      else{
         mEndpoint->create([this, cConnected]{
            mEndpoint->connect(cConnected, *mEndpoint); // connect
         }); // create
      }

   }
   catch(exception& e)
   {
      ErrLog(<<"something went wrong: " << e.what());
      c(false, nullptr);
   }
}

AsyncBool
KurentoRemoteParticipant::buildSdpAnswer(const SdpContents& offer, ContinuationSdpReady c)
{
   AsyncBool valid = False;

   std::shared_ptr<SdpContents> offerMangled = std::make_shared<SdpContents>(offer);
   while(mRemoveExtraMediaDescriptors && offerMangled->session().media().size() > 2)
   {
      // FIXME hack to remove BFCP
      DebugLog(<<"more than 2 media descriptors, removing the last");
      offerMangled->session().media().pop_back();
   }

   try
   {
      // do some checks on the offer
      // check for video, check for WebRTC
      typedef std::map<resip::Data, int> MediaSummary;
      MediaSummary mediaMap;
      std::set<resip::Data> mediumTransports;
      for(SdpContents::Session::MediumContainer::const_iterator it = offerMangled->session().media().cbegin();
               it != offerMangled->session().media().cend();
               it++)
      {
         const SdpContents::Session::Medium& m = *it;
         if(mediaMap.find(m.name()) != mediaMap.end())
         {
            mediaMap[m.name()]++;
         }
         else
         {
            mediaMap[m.name()] = 1;
         }
         mediumTransports.insert(m.protocol());
      }
      for(MediaSummary::const_iterator it = mediaMap.begin();
               it != mediaMap.end();
               it++)
      {
         StackLog(<<"found medium type " << it->first << " "  << it->second << " instance(s)");
      }
      bool isWebRTC = std::find(mediumTransports.cbegin(),
               mediumTransports.end(),
               "RTP/SAVPF") != mediumTransports.end();
      DebugLog(<<"peer is " << (isWebRTC ? "WebRTC":"not WebRTC"));

      for(SdpContents::Session::MediumContainer::iterator it = offerMangled->session().media().begin();
                     it != offerMangled->session().media().end();
                     it++)
      {
         SdpContents::Session::Medium& m = *it;
         if(!isWebRTC)
         {
            m.port() = 9;
            m.addAttribute(Data("direction"), Data("active"));
         }
      }

      std::ostringstream offerMangledBuf;
      offerMangledBuf << *offerMangled;
      std::shared_ptr<std::string> offerMangledStr = std::make_shared<std::string>(offerMangledBuf.str());

      StackLog(<<"offer TO Kurento: " << *offerMangledStr);

      bool endpointExists = true;
      mIceGatheringDone = true;
      if(!mEndpoint)
      {
         endpointExists = false;
         if(isWebRTC)
         {
            // delay while ICE gathers candidates from STUN and TURN
            mIceGatheringDone = false;
            mEndpoint.reset(new kurento::WebRtcEndpoint(mKurentoConversationManager.mPipeline));
         }
         else
         {
            mEndpoint.reset(newEndpoint());
         }
      }

      // FIXME - add listeners for Kurento events

      kurento::ContinuationString cOnAnswerReady = [this, offerMangled, isWebRTC, c](const std::string& answer){
         StackLog(<<"answer FROM Kurento: " << answer);
         HeaderFieldValue hfv(answer.data(), answer.size());
         Mime type("application", "sdp");
         std::unique_ptr<SdpContents> _answer(new SdpContents(hfv, type));
         if(isHolding())
         {
            SdpContents::Session::MediumContainer::iterator it = _answer->session().media().begin();
            for(;it != _answer->session().media().end(); it++)
            {
               SdpContents::Session::Medium& m = *it;
               if(m.exists("sendrecv"))
               {
                  m.clearAttribute("sendrecv");
                  m.addAttribute("sendonly");
               }
               if(m.exists("recvonly"))
               {
                  m.clearAttribute("recvonly");
                  m.addAttribute("inactive");
               }
            }
         }
         setLocalSdp(*_answer);
         setRemoteSdp(*offerMangled);
         c(true, std::move(_answer));
      };

      kurento::ContinuationVoid cConnected = [this, offerMangled, offerMangledStr, isWebRTC, endpointExists, c, cOnAnswerReady]{
         if(endpointExists && mReuseSdpAnswer)
         {
            // FIXME - Kurento should handle hold/resume
            // but it fails with SDP_END_POINT_ALREADY_NEGOTIATED
            // if we call processOffer more than once
            std::ostringstream answerBuf;
            answerBuf << *getLocalSdp();
            std::shared_ptr<std::string> answerStr = std::make_shared<std::string>(answerBuf.str());
            cOnAnswerReady(*answerStr);
            return;
         }
         mEndpoint->processOffer([this, offerMangled, isWebRTC, c, cOnAnswerReady](const std::string& answer){
            if(isWebRTC)
            {
               std::shared_ptr<kurento::WebRtcEndpoint> webRtc = std::static_pointer_cast<kurento::WebRtcEndpoint>(mEndpoint);

               std::shared_ptr<kurento::EventContinuation> elIceGatheringDone =
                     std::make_shared<kurento::EventContinuation>([this, cOnAnswerReady]{
                  mIceGatheringDone = true;
                  mEndpoint->getLocalSessionDescriptor(cOnAnswerReady);
               });
               webRtc->addOnIceGatheringDoneListener(elIceGatheringDone, [this](){});

               webRtc->gatherCandidates([]{
                  // FIXME - handle the case where it fails
                  // on success, we continue from the IceGatheringDone event handler
               }); // gatherCandidates
            }
            else
            {
               cOnAnswerReady(answer);
            }
         }, *offerMangledStr); // processOffer
      };

      if(endpointExists)
      {
         cConnected();
      }
      else
      {
         //mMultiqueue.reset(new kurento::GStreamerFilter(mKurentoConversationManager.mPipeline, "videoconvert"));
         //mMultiqueue.reset(new kurento::PassthroughElement(mKurentoConversationManager.mPipeline));
         mPlayer.reset(new kurento::PlayerEndpoint(mKurentoConversationManager.mPipeline, "file:///tmp/test.mp4"));
         mEndpoint->create([this, cConnected]{
            //mMultiqueue->create([this, cConnected]{
               // mMultiqueue->connect([this, cConnected]{
                  // mEndpoint->connect([this, cConnected]{
                     mPlayer->create([this, cConnected]{
                        mPlayer->play([this, cConnected]{
                           cConnected();
                           //mPlayer->connect(cConnected, *mEndpoint); // connect
                        });
                     });
                  // }, *mMultiqueue); // mEndpoint->connect
               // }, *mEndpoint); // mMultiqueue->connect
            //}); // mMultiqueue->create
         }); // create
      }

      valid = Async;
   }
   catch(exception& e)
   {
      ErrLog(<<"something went wrong: " << e.what()); // FIXME - add try/catch to Continuation
   }

   return valid;
}

void
KurentoRemoteParticipant::adjustRTPStreams(bool sendingOffer)
{
   // FIXME Kurento - implement, may need to break up this method into multiple parts

   // FIXME Kurento - sometimes true
   setRemoteHold(false);

}

void
KurentoRemoteParticipant::addToConversation(Conversation *conversation, unsigned int inputGain, unsigned int outputGain)
{
   RemoteParticipant::addToConversation(conversation, inputGain, outputGain);
}

void
KurentoRemoteParticipant::removeFromConversation(Conversation *conversation)
{
   RemoteParticipant::removeFromConversation(conversation);
}

bool
KurentoRemoteParticipant::mediaStackPortAvailable()
{
   return true; // FIXME Kurento - can we check with Kurento somehow?
}

void
KurentoRemoteParticipant::waitingMode()
{
   mPlayer->connect([]{DebugLog(<<"connected in loopback/video, waiting for peer");}, *mEndpoint);
   //mEndpoint->connect([]{DebugLog(<<"connected in loopback/video, waiting for peer");}, *mEndpoint);
}


/* ====================================================================

 Copyright (c) 2021, SIP Spectrum, Inc.
 Copyright (c) 2021, Daniel Pocock https://danielpocock.com
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
