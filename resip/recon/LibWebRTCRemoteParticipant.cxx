#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "LibWebRTCMediaStackAdapter.hxx"

#include "LibWebRTCRemoteParticipant.hxx"
#include "Conversation.hxx"
#include "UserAgent.hxx"
#include "DtmfEvent.hxx"
#include "ReconSubsystem.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/Random.hxx>
#include <resip/stack/DtmfPayloadContents.hxx>
#include <resip/stack/SdpContents.hxx>
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
LibWebRTCRemoteParticipant::LibWebRTCRemoteParticipant(ParticipantHandle partHandle,
                                     ConversationManager& conversationManager,
                                     LibWebRTCMediaStackAdapter& libwebrtcMediaStackAdapter,
                                     DialogUsageManager& dum,
                                     RemoteParticipantDialogSet& remoteParticipantDialogSet)
: Participant(partHandle, ConversationManager::ParticipantType_Remote, conversationManager),
  RemoteParticipant(partHandle, conversationManager, dum, remoteParticipantDialogSet),
  LibWebRTCParticipant(partHandle, ConversationManager::ParticipantType_Remote, conversationManager, libwebrtcMediaStackAdapter),
  mRemoveExtraMediaDescriptors(false),
  mSipRtpEndpoint(true),
  mReuseSdpAnswer(false),
  mWSAcceptsKeyframeRequests(true),
  mLastRemoteSdp(0),
  mWaitingAnswer(false),
  mWebRTCOutgoing(getDialogSet().getConversationProfile()->mediaEndpointMode() == ConversationProfile::WebRTC)
{
   InfoLog(<< "LibWebRTCRemoteParticipant created (UAC), handle=" << mHandle);
}

// UAS - or forked leg
LibWebRTCRemoteParticipant::LibWebRTCRemoteParticipant(ConversationManager& conversationManager,
                                     LibWebRTCMediaStackAdapter& libwebrtcMediaStackAdapter,
                                     DialogUsageManager& dum, 
                                     RemoteParticipantDialogSet& remoteParticipantDialogSet)
: Participant(ConversationManager::ParticipantType_Remote, conversationManager),
  RemoteParticipant(conversationManager, dum, remoteParticipantDialogSet),
  LibWebRTCParticipant(ConversationManager::ParticipantType_Remote, conversationManager, libwebrtcMediaStackAdapter),
  mRemoveExtraMediaDescriptors(false),
  mSipRtpEndpoint(true),
  mReuseSdpAnswer(false),
  mWSAcceptsKeyframeRequests(true),
  mLastRemoteSdp(0),
  mWaitingAnswer(false),
  mWebRTCOutgoing(getDialogSet().getConversationProfile()->mediaEndpointMode() == ConversationProfile::WebRTC)
{
   InfoLog(<< "LibWebRTCRemoteParticipant created (UAS or forked leg), handle=" << mHandle);
}

LibWebRTCRemoteParticipant::~LibWebRTCRemoteParticipant()
{
   // Note:  Ideally this call would exist in the Participant Base class - but this call requires 
   //        dynamic_casts and virtual methods to function correctly during destruction.
   //        If the call is placed in the base Participant class then these things will not
   //        function as desired because a classes type changes as the descructors unwind.
   //        See https://stackoverflow.com/questions/10979250/usage-of-this-in-destructor.
   unregisterFromAllConversations();

   InfoLog(<< "LibWebRTCRemoteParticipant destroyed, handle=" << mHandle);
}

int 
LibWebRTCRemoteParticipant::getConnectionPortOnBridge()
{
   if(getDialogSet().getActiveRemoteParticipantHandle() == mHandle)
   {
      return -1;  // FIXME LibWebRTC
   }
   else
   {
      // If this is not active fork leg, then we don't want to effect the bridge mixer.  
      // Note:  All forked endpoints/participants have the same connection port on the bridge
      return -1;
   }
}

int 
LibWebRTCRemoteParticipant::getMediaConnectionId()
{ 
   return getLibWebRTCDialogSet().getMediaConnectionId();
}

void
LibWebRTCRemoteParticipant::applyBridgeMixWeights()
{
   // FIXME LibWebRTC - do we need to implement this?
}

// Special version of this call used only when a participant
// is removed from a conversation.  Required when sipXConversationMediaInterfaceMode
// is used, in order to get a pointer to the bridge mixer
// for a participant (ie. LocalParticipant) that has no currently
// assigned conversations.
void
LibWebRTCRemoteParticipant::applyBridgeMixWeights(Conversation* removedConversation)
{
   // FIXME LibWebRTC - do we need to implement this?
}

libwebrtc::BaseRtpEndpoint*
LibWebRTCRemoteParticipant::newEndpoint()
{
   return mSipRtpEndpoint ?
            dynamic_cast<libwebrtc::BaseRtpEndpoint*>(new libwebrtc::SipRtpEndpoint(mLibWebRTCMediaStackAdapter.mPipeline)) :
            dynamic_cast<libwebrtc::BaseRtpEndpoint*>(new libwebrtc::RtpEndpoint(mLibWebRTCMediaStackAdapter.mPipeline));
}

bool
LibWebRTCRemoteParticipant::initEndpointIfRequired(bool isWebRTC)
{
   if(mEndpoint)
   {
      return false;
   }
   if(isWebRTC)
   {
      // delay while ICE gathers candidates from STUN and TURN
      mIceGatheringDone = false;
      mEndpoint.reset(new libwebrtc::WebRtcEndpoint(mLibWebRTCMediaStackAdapter.mPipeline));
   }
   else
   {
      mIceGatheringDone = true;
      mEndpoint.reset(newEndpoint());
   }

   //mMultiqueue.reset(new libwebrtc::GStreamerFilter(mLibWebRTCMediaStackAdapter.mPipeline, "videoconvert"));
   //mMultiqueue.reset(new libwebrtc::PassThroughElement(mLibWebRTCMediaStackAdapter.mPipeline));
   std::shared_ptr<resip::ConfigParse> cfg = mConversationManager.getConfig();
   if(cfg)
   {
      const Data& holdVideo = mConversationManager.getConfig()->getConfigData("HoldVideo", "file:///tmp/test.mp4", true);
      mPlayer.reset(new libwebrtc::PlayerEndpoint(mLibWebRTCMediaStackAdapter.mPipeline, holdVideo.c_str()));
   }
   mPassThrough.reset(new libwebrtc::PassThroughElement(mLibWebRTCMediaStackAdapter.mPipeline));

   return true;
}

void
LibWebRTCRemoteParticipant::doIceGathering(libwebrtc::ContinuationString sdpReady)
{
   std::shared_ptr<libwebrtc::WebRtcEndpoint> webRtc = std::static_pointer_cast<libwebrtc::WebRtcEndpoint>(mEndpoint);

   std::shared_ptr<libwebrtc::EventContinuation> elEventIceCandidateFound =
         std::make_shared<libwebrtc::EventContinuation>([this](std::shared_ptr<libwebrtc::Event> event){
      DebugLog(<<"received event: " << *event);
      std::shared_ptr<libwebrtc::OnIceCandidateFoundEvent> _event =
         std::dynamic_pointer_cast<libwebrtc::OnIceCandidateFoundEvent>(event);
      resip_assert(_event.get());

      if(!mTrickleIcePermitted)
      {
         return;
      }
      // FIXME - if we are waiting for a previous INFO to be confirmed,
      //         aggregate the candidates into a vector and send them in bulk
      auto ice = getLocalSdp()->session().makeIceFragment(Data(_event->getCandidate()),
         _event->getLineIndex(), Data(_event->getId()));
      if(ice.get())
      {
         StackLog(<<"about to send " << *ice);
         info(*ice);
      }
      else
      {
         WarningLog(<<"failed to create ICE fragment for mid: " << _event->getId());
      }
   });

   std::shared_ptr<libwebrtc::EventContinuation> elIceGatheringDone =
            std::make_shared<libwebrtc::EventContinuation>([this, sdpReady](std::shared_ptr<libwebrtc::Event> event){
      mIceGatheringDone = true;
      mEndpoint->getLocalSessionDescriptor(sdpReady);
   });

   webRtc->addOnIceCandidateFoundListener(elEventIceCandidateFound, [=](){
      webRtc->addOnIceGatheringDoneListener(elIceGatheringDone, [=](){
         webRtc->gatherCandidates([]{
                  // FIXME - handle the case where it fails
                  // on success, we continue from the IceGatheringDone event handler
         }); // gatherCandidates
      });
   });
}

void
LibWebRTCRemoteParticipant::createAndConnectElements(libwebrtc::ContinuationVoid cConnected)
{
   // FIXME - implement listeners for some of the events currently using elEventDebug

   std::shared_ptr<libwebrtc::EventContinuation> elError =
         std::make_shared<libwebrtc::EventContinuation>([this](std::shared_ptr<libwebrtc::Event> event){
      ErrLog(<<"Error from LibWebRTC MediaObject: " << *event);
   });

   std::shared_ptr<libwebrtc::EventContinuation> elEventDebug =
         std::make_shared<libwebrtc::EventContinuation>([this](std::shared_ptr<libwebrtc::Event> event){
      DebugLog(<<"received event: " << *event);
   });

   std::shared_ptr<libwebrtc::EventContinuation> elEventKeyframeRequired =
         std::make_shared<libwebrtc::EventContinuation>([this](std::shared_ptr<libwebrtc::Event> event){
      DebugLog(<<"received event: " << *event);
      requestKeyframeFromPeer();
   });

   mEndpoint->create([=]{
      mEndpoint->addErrorListener(elError, [=](){
         mEndpoint->addConnectionStateChangedListener(elEventDebug, [=](){
            mEndpoint->addMediaStateChangedListener(elEventDebug, [=](){
               mEndpoint->addMediaTranscodingStateChangeListener(elEventDebug, [=](){
                  mEndpoint->addMediaFlowInStateChangeListener(elEventDebug, [=](){
                     mEndpoint->addMediaFlowOutStateChangeListener(elEventDebug, [=](){
                        mEndpoint->addKeyframeRequiredListener(elEventKeyframeRequired, [=](){
                           //mMultiqueue->create([this, cConnected]{
                           // mMultiqueue->connect([this, cConnected]{
                           mPlayer->create([this, cConnected]{
                              mPassThrough->create([this, cConnected]{
                                 mEndpoint->connect([this, cConnected]{
                                    mPassThrough->connect([this, cConnected]{
                                       //mPlayer->play([this, cConnected]{
                                       cConnected();
                                       //mPlayer->connect(cConnected, *mEndpoint); // connect
                                       //});
                                    }, *mEndpoint);
                                 }, *mPassThrough);
                              });
                           });
                           //}, *mEndpoint); // mEndpoint->connect
                           // }, *mEndpoint); // mMultiqueue->connect
                           //}); // mMultiqueue->create
                        }); // addKeyframeRequiredListener

                     });
                  });
               });
            });
         });
      });
   }); // create
}

void
LibWebRTCRemoteParticipant::buildSdpOffer(bool holdSdp, CallbackSdpReady sdpReady, bool preferExistingSdp)
{
   bool useExistingSdp = false;
   if(getLocalSdp())
   {
      useExistingSdp = preferExistingSdp || mReuseSdpAnswer;
   }

   try
   {
      bool isWebRTC = mWebRTCOutgoing;
      bool firstUseEndpoint = initEndpointIfRequired(isWebRTC);

      libwebrtc::ContinuationString cOnOfferReady = [this, holdSdp, sdpReady](const std::string& offer){
         StackLog(<<"offer FROM LibWebRTC: " << offer);
         HeaderFieldValue hfv(offer.data(), offer.size());
         Mime type("application", "sdp");
         std::unique_ptr<SdpContents> _offer(new SdpContents(hfv, type));
         _offer->session().transformLocalHold(holdSdp);
         setProposedSdp(*_offer);
         sdpReady(true, std::move(_offer));
      };

      libwebrtc::ContinuationVoid cConnected = [this, holdSdp, isWebRTC, sdpReady, cOnOfferReady]{
         // FIXME - can we tell LibWebRTC to use holdSdp?
         // We currently mangle the SDP after-the-fact in cOnOfferReady
         mEndpoint->generateOffer([this, isWebRTC, sdpReady, cOnOfferReady](const std::string& offer){
            mWaitingAnswer = true;
            if(isWebRTC)
            {
               doIceGathering(cOnOfferReady);
            }
            else
            {
               cOnOfferReady(offer);
            }
         }); // generateOffer
      };

      if(firstUseEndpoint)
      {
         createAndConnectElements(cConnected);
      }
      else
      {
         if(!useExistingSdp)
         {
            cConnected();
         }
         else
         {
            std::ostringstream offerMangledBuf;
            getLocalSdp()->session().transformLocalHold(isHolding());
            offerMangledBuf << *getLocalSdp();
            std::shared_ptr<std::string> offerMangledStr = std::make_shared<std::string>(offerMangledBuf.str());
            cOnOfferReady(*offerMangledStr);
         }
      }
   }
   catch(exception& e)
   {
      ErrLog(<<"something went wrong: " << e.what());
      sdpReady(false, nullptr);
   }
}

void
LibWebRTCRemoteParticipant::buildSdpAnswer(const SdpContents& offer, CallbackSdpReady sdpReady)
{
   bool requestSent = false;

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
      bool isWebRTC = offerMangled->session().isWebRTC();
      DebugLog(<<"peer is " << (isWebRTC ? "WebRTC":"not WebRTC"));

      if(!isWebRTC)
      {
         // RFC 4145 uses the attribute name "setup"
         // We override the attribute name and use the legacy name "direction"
         // from the drafts up to draft-ietf-mmusic-sdp-comedia-05.txt
         // Tested with LibWebRTC and Cisco EX90
         // https://datatracker.ietf.org/doc/html/draft-ietf-mmusic-sdp-comedia-05
         // https://datatracker.ietf.org/doc/html/rfc4145
         offerMangled->session().transformCOMedia("active", "direction");
      }

      std::ostringstream offerMangledBuf;
      offerMangledBuf << *offerMangled;
      std::shared_ptr<std::string> offerMangledStr = std::make_shared<std::string>(offerMangledBuf.str());

      StackLog(<<"offer TO LibWebRTC: " << *offerMangledStr);

      bool firstUseEndpoint = initEndpointIfRequired(isWebRTC);

      libwebrtc::ContinuationString cOnAnswerReady = [this, offerMangled, isWebRTC, sdpReady](const std::string& answer){
         StackLog(<<"answer FROM LibWebRTC: " << answer);
         HeaderFieldValue hfv(answer.data(), answer.size());
         Mime type("application", "sdp");
         std::unique_ptr<SdpContents> _answer(new SdpContents(hfv, type));
         _answer->session().transformLocalHold(isHolding());
         setLocalSdp(*_answer);
         setRemoteSdp(*offerMangled);
         sdpReady(true, std::move(_answer));
      };

      libwebrtc::ContinuationVoid cConnected = [this, offerMangled, offerMangledStr, isWebRTC, firstUseEndpoint, sdpReady, cOnAnswerReady]{
         if(!firstUseEndpoint && mReuseSdpAnswer)
         {
            // FIXME - LibWebRTC should handle hold/resume
            // but it fails with SDP_END_POINT_ALREADY_NEGOTIATED
            // if we call processOffer more than once
            std::ostringstream answerBuf;
            answerBuf << *getLocalSdp();
            std::shared_ptr<std::string> answerStr = std::make_shared<std::string>(answerBuf.str());
            cOnAnswerReady(*answerStr);
            return;
         }
         mEndpoint->processOffer([this, offerMangled, isWebRTC, sdpReady, cOnAnswerReady](const std::string& answer){
            if(isWebRTC)
            {
               if(mTrickleIcePermitted && offerMangled->session().isTrickleIceSupported())
               {
                  HeaderFieldValue hfv(answer.data(), answer.size());
                  Mime type("application", "sdp");
                  std::unique_ptr<SdpContents> _local(new SdpContents(hfv, type));
                  DebugLog(<<"storing incomplete webrtc answer");
                  setLocalSdp(*_local);
                  setRemoteSdp(*offerMangled);
                  ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(getInviteSessionHandle().get());
                  sis->provideAnswer(*_local);
                  sis->provisional(183, true);
                  //getDialogSet().provideAnswer(std::move(_local), getInviteSessionHandle(), false, true);
                  enableTrickleIce(); // now we are in early media phase, it is safe to send INFO

                  // FIXME - if we sent an SDP answer here,
                  //         make sure we don't call provideAnswer again on 200 OK
               }

               doIceGathering(cOnAnswerReady);
            }
            else
            {
               cOnAnswerReady(answer);
            }
         }, *offerMangledStr); // processOffer
      };

      if(firstUseEndpoint)
      {
         createAndConnectElements(cConnected);
      }
      else
      {
         cConnected();
      }

      requestSent = true;
   }
   catch(exception& e)
   {
      ErrLog(<<"something went wrong: " << e.what()); // FIXME - add try/catch to Continuation
      requestSent = false;
   }

   if(!requestSent)
   {
      sdpReady(false, nullptr);
   }
}

void
LibWebRTCRemoteParticipant::adjustRTPStreams(bool sendingOffer)
{
   // FIXME LibWebRTC - implement, may need to break up this method into multiple parts
   StackLog(<<"adjustRTPStreams");

   std::shared_ptr<SdpContents> localSdp = sendingOffer ? getDialogSet().getProposedSdp() : getLocalSdp();
   resip_assert(localSdp);

   std::shared_ptr<SdpContents> remoteSdp = getRemoteSdp();
   bool remoteSdpChanged = remoteSdp.get() != mLastRemoteSdp; // FIXME - better way to do this?
   mLastRemoteSdp = remoteSdp.get();
   if(remoteSdp)
   {
      const SdpContents::Session::Direction& remoteDirection = remoteSdp->session().getDirection();
      if(!remoteDirection.recv())
      {
         setRemoteHold(true);
      }
      else
      {
         setRemoteHold(false);
      }
      if(remoteSdpChanged && mWaitingAnswer)
      {
         // FIXME - maybe this should not be in adjustRTPStreams
         DebugLog(<<"remoteSdp has changed, sending to LibWebRTC");
         mWaitingAnswer = false;
         std::ostringstream answerBuf;
         answerBuf << *remoteSdp;
         mEndpoint->processAnswer([this](const std::string updatedOffer){
            // FIXME - use updatedOffer
            WarningLog(<<"LibWebRTC has processed the peer's SDP answer");
            StackLog(<<"updatedOffer FROM LibWebRTC: " << updatedOffer);
            HeaderFieldValue hfv(updatedOffer.data(), updatedOffer.size());
            Mime type("application", "sdp");
            std::unique_ptr<SdpContents> _updatedOffer(new SdpContents(hfv, type));
            _updatedOffer->session().transformLocalHold(isHolding());
            setLocalSdp(*_updatedOffer);
            //c(true, std::move(_updatedOffer));
         }, answerBuf.str());
      }
      for(int i = 1000; i <= 5000; i+=1000)
      {
         std::chrono::milliseconds _i = std::chrono::milliseconds(i);
         std::chrono::milliseconds __i = std::chrono::milliseconds(i + 500);
         mConversationManager.requestKeyframe(mHandle, _i);
         mConversationManager.requestKeyframeFromPeer(mHandle, __i);
      }
   }
}

void
LibWebRTCRemoteParticipant::addToConversation(Conversation *conversation, unsigned int inputGain, unsigned int outputGain)
{
   RemoteParticipant::addToConversation(conversation, inputGain, outputGain);
}

void
LibWebRTCRemoteParticipant::removeFromConversation(Conversation *conversation)
{
   RemoteParticipant::removeFromConversation(conversation);
}

bool
LibWebRTCRemoteParticipant::mediaStackPortAvailable()
{
   return true; // FIXME LibWebRTC - can we check with LibWebRTC somehow?
}

void
LibWebRTCRemoteParticipant::waitingMode()
{
   std::shared_ptr<libwebrtc::MediaElement> e = getWaitingModeElement();
   if(!e)
   {
      return;
   }
   e->connect([this]{
      DebugLog(<<"connected in waiting mode, waiting for peer");
      if(mWaitingModeVideo)
      {
         mPlayer->play([this]{}); // FIXME LibWebRTC async
      }
      else
      {
         mEndpoint->connect([this]{
            requestKeyframeFromPeer();
         }, *mPassThrough); // FIXME LibWebRTC async
      }
      requestKeyframeFromPeer();
   }, *mEndpoint);
}

std::shared_ptr<libwebrtc::MediaElement>
LibWebRTCRemoteParticipant::getWaitingModeElement()
{
   if(mWaitingModeVideo)
   {
      return dynamic_pointer_cast<libwebrtc::Endpoint>(mPlayer);
   }
   else
   {
      return mPassThrough;
   }
}

bool
LibWebRTCRemoteParticipant::onMediaControlEvent(MediaControlContents::MediaControl& mediaControl)
{
   if(mWSAcceptsKeyframeRequests)
   {
      auto now = std::chrono::steady_clock::now();
      if(now < (mLastLocalKeyframeRequest + getKeyframeRequestInterval()))
      {
         DebugLog(<<"keyframe request ignored, too soon");
         return false;
      }
      mLastLocalKeyframeRequest = now;
      InfoLog(<<"onMediaControlEvent: sending to LibWebRTC");
      // FIXME - check the content of the event
      mEndpoint->sendPictureFastUpdate([this](){}); // FIXME LibWebRTC async, do we need to wait for LibWebRTC here?
      return true;
   }
   else
   {
      WarningLog(<<"rejecting MediaControlEvent due to config option mWSAcceptsKeyframeRequests");
      return false;
   }
}

bool
LibWebRTCRemoteParticipant::onTrickleIce(resip::TrickleIceContents& trickleIce)
{
   DebugLog(<<"onTrickleIce: sending to LibWebRTC");
   // FIXME - did we already receive a suitable SDP for trickle ICE and send it to LibWebRTC?
   //         if not, LibWebRTC is not ready for the candidates
   // FIXME - do we need to validate the ice-pwd password attribute here?
   std::shared_ptr<libwebrtc::WebRtcEndpoint> webRtc = std::static_pointer_cast<libwebrtc::WebRtcEndpoint>(mEndpoint);
   for(auto m = trickleIce.media().cbegin(); m != trickleIce.media().cend(); m++)
   {
      if(m->exists("mid"))
      {
         const Data& mid = m->getValues("mid").front();
         const std::string _mid = mid.c_str();
         unsigned int mLineIndex = mid.convertInt(); // FIXME - calculate from the full SDP
         if(m->exists("candidate"))
         {
            auto candidates = m->getValues("candidate");
            for(auto a = candidates.cbegin(); a != candidates.cend(); a++)
            {
               webRtc->addIceCandidate([this](){},
                  a->c_str(),
                  _mid,
                  mLineIndex);
            }
         }
      }
      else
      {
         WarningLog(<<"mid is missing for Medium in SDP fragment: " << trickleIce);
         return false;
      }
   }
   return true;
}

void
LibWebRTCRemoteParticipant::requestKeyframe()
{
   WarningLog(<<"requestKeyframe not implemented for this media stack"); // FIXME
}

/* ====================================================================

 Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2022, Daniel Pocock https://danielpocock.com
 Copyright (c) 2021, SIP Spectrum, Inc.
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
