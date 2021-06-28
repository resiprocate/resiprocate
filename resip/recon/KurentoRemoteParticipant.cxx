#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <kurento-client/KurentoClient.h>

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

void
ReConKurentoClientLogSink::log(KurentoClientLogSink::Level level, const char *s)
{
   switch(level)
   {
   case Crit:
      CritLog(<< s);
      break;
   case Err:
      ErrLog(<< s);
      break;
   case Warning:
      WarningLog(<< s);
      break;
   case Info:
      InfoLog(<< s);
      break;
   case Debug:
      DebugLog(<< s);
      break;
   default:
      ErrLog(<< "unrecognized log level from KurentoClient: " << level);
      ErrLog(<< s);
   }
}

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
  KurentoParticipant(partHandle, kurentoConversationManager)
{
   InfoLog(<< "KurentoRemoteParticipant created (UAC), handle=" << mHandle);

   // FIXME, this could be much better
   KurentoClientLogSink::impl = new ReConKurentoClientLogSink(); // FIXME
}

// UAS - or forked leg
KurentoRemoteParticipant::KurentoRemoteParticipant(KurentoConversationManager& kurentoConversationManager,
                                     DialogUsageManager& dum, 
                                     RemoteParticipantDialogSet& remoteParticipantDialogSet)
: Participant(kurentoConversationManager),
  RemoteParticipant(kurentoConversationManager, dum, remoteParticipantDialogSet),
  KurentoParticipant(kurentoConversationManager)
{
   InfoLog(<< "KurentoRemoteParticipant created (UAS or forked leg), handle=" << mHandle);

   // FIXME, this could be much better
   KurentoClientLogSink::impl = new ReConKurentoClientLogSink(); // FIXME
}

KurentoRemoteParticipant::~KurentoRemoteParticipant()
{
   // Note:  Ideally this call would exist in the Participant Base class - but this call requires 
   //        dynamic_casts and virtual methods to function correctly during destruction.
   //        If the call is placed in the base Participant class then these things will not
   //        function as desired because a classes type changes as the descructors unwind.
   //        See https://stackoverflow.com/questions/10979250/usage-of-this-in-destructor.
   unregisterFromAllConversations();

   if(!mEndpointId.empty())
   {
      mKurentoConversationManager.unregisterEndpoint(mEndpointId);
   }
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

void
KurentoRemoteParticipant::buildSdpOffer(bool holdSdp, SdpContents& offer)
{
   // FIXME Kurento - this needs to be async

   // FIXME Kurento - use holdSdp

   // FIXME Kurento - include video, SRTP, WebRTC?

   try
   {
      kurento_client::KurentoClient& client = mKurentoConversationManager.getKurentoClient();

      client.createRtpEndpoint("RtpEndpoint");
      setEndpointId(client.getRtpEndpointId().c_str());
      client.invokeConnect();
      client.gatherCandidates();
      client.invokeProcessOffer();

      std::string _offer(client.getMReturnedSdp());

      StackLog(<<"offer FROM Kurento: " << _offer.c_str());

      HeaderFieldValue hfv(_offer.data(), _offer.size());
      Mime type("application", "sdp");
      offer = SdpContents(hfv, type);

      setProposedSdp(offer);
   }
   catch(exception& e)
   {
      ErrLog(<<"something went wrong: " << e.what());
   }
}

bool
KurentoRemoteParticipant::buildSdpAnswer(const SdpContents& offer, SdpContents& answer)
{
   // FIXME Kurento - this needs to be async

   bool valid = false;

   try
   {
      // do some checks on the offer
      // check for video, check for WebRTC
      typedef std::map<resip::Data, int> MediaSummary;
      MediaSummary mediaMap;
      std::set<resip::Data> mediumTransports;
      for(SdpContents::Session::MediumContainer::const_iterator it = offer.session().media().cbegin();
               it != offer.session().media().cend();
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

      std::ostringstream _offer;
      _offer << offer;

      const std::string& kOffer = _offer.str();
      StackLog(<<"offer TO Kurento: " << kOffer);

      kurento_client::KurentoClient& client = mKurentoConversationManager.getKurentoClient();

      client.setMSdp(kOffer);
      std::string pipelineId(client.getMediaPipelineId());

      std::string _answer;

      mIceGatheringDone = true;
      if(isWebRTC)
      {
         // delay while ICE gathers candidates from STUN and TURN
         mIceGatheringDone = false;
      }
      std::string sessionId(client.getSessionId());
      client.createRtpEndpoint(isWebRTC ? "WebRtcEndpoint" : "RtpEndpoint");
      std::string ourEndpointId(client.getRtpEndpointId());
      setEndpointId(ourEndpointId.c_str());
      DebugLog(<<"our endpoint: " << ourEndpointId);
      DebugLog(<<"joining participant to existing pipeline: " << pipelineId
               << " session: " << sessionId
               << " for loopback");
      client.invokeConnect(ourEndpointId.c_str(), ourEndpointId.c_str(), sessionId.c_str());
      if(isWebRTC)
      {
         client.addListener("IceCandidateFound", ourEndpointId.c_str(), sessionId.c_str());
         client.addListener("IceGatheringDone", ourEndpointId.c_str(), sessionId.c_str());
      }
      _answer = client.invokeProcessOffer(ourEndpointId.c_str(), sessionId.c_str());
      if(isWebRTC)
      {
         client.gatherCandidates();
         while(!mIceGatheringDone)
         {
            sleepMs(10);
         }
         _answer = client.getLocalSessionDescriptor();
      }

      StackLog(<<"answer FROM Kurento: " << _answer);

      HeaderFieldValue hfv(_answer.data(), _answer.size());
      Mime type("application", "sdp");
      answer = SdpContents(hfv, type);

      valid = true;
   }
   catch(exception& e)
   {
      ErrLog(<<"something went wrong: " << e.what());
   }

   if(valid)
   {
      setLocalSdp(answer);
      setRemoteSdp(offer);
   }

   updateConversation();
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
KurentoRemoteParticipant::updateConversation()
{
   const ConversationMap& cm = getConversations();
   Conversation* conversation = cm.begin()->second;
   Conversation::ParticipantMap& m = conversation->getParticipants();
   if(m.size() < 2)
   {
      DebugLog(<<"we are first in the conversation");
      return;
   }
   if(m.size() > 2)
   {
      WarningLog(<<"participants already here, can't join, size = " << m.size());
      return;
   }
   DebugLog(<<"joining a Conversation with an existing Participant");

   kurento_client::KurentoClient& client = mKurentoConversationManager.getKurentoClient();
   std::string sessionId(client.getSessionId());

   if(mEndpointId.empty())
   {
      DebugLog(<<"our endpointId is missing");
      return;
   }
   std::string ourEndpointId(mEndpointId.c_str());
   // remove our loopback
   client.invokeDisconnect(ourEndpointId.c_str(), ourEndpointId.c_str(), sessionId.c_str());

   KurentoRemoteParticipant* krp = 0;
   Conversation::ParticipantMap::iterator it = m.begin();
   while(it != m.end() && krp == 0)
   {
      krp = dynamic_cast<KurentoRemoteParticipant*>(it->second.getParticipant());
      if(krp == this)
      {
         krp = 0;
      }
   }
   std::string otherEndpointId(krp->mEndpointId.c_str());
   // remove the peer's loopback
   client.invokeDisconnect(otherEndpointId.c_str(), otherEndpointId.c_str(), sessionId.c_str());

   // now join them together
   client.invokeConnect(ourEndpointId.c_str(), otherEndpointId.c_str(), sessionId.c_str());
   client.invokeConnect(otherEndpointId.c_str(), ourEndpointId.c_str(), sessionId.c_str());
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
KurentoRemoteParticipant::setEndpointId(const Data& endpointId)
{
   mEndpointId = endpointId;
   mKurentoConversationManager.registerEndpoint(endpointId, getKurentoDialogSet());
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
