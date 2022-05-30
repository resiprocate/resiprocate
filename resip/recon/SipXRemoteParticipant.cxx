#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SipXConversationManager.hxx"

#include "sdp/SdpHelperResip.hxx"
#include "sdp/Sdp.hxx"

#include <sdp/SdpCodec.h>  // sipX SdpCodec

#include "SipXRemoteParticipant.hxx"
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
using namespace sdpcontainer;
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
SipXRemoteParticipant::SipXRemoteParticipant(ParticipantHandle partHandle,
                                     ConversationManager& conversationManager,
                                     SipXConversationManager& sipXConversationManager,
                                     DialogUsageManager& dum,
                                     RemoteParticipantDialogSet& remoteParticipantDialogSet)
: Participant(partHandle, ConversationManager::ParticipantType_Remote, conversationManager),
  RemoteParticipant(partHandle, conversationManager, dum, remoteParticipantDialogSet),
  SipXParticipant(partHandle, ConversationManager::ParticipantType_Remote, conversationManager, sipXConversationManager)
{
   InfoLog(<< "SipXRemoteParticipant created (UAC), handle=" << mHandle);
}

// UAS - or forked leg
SipXRemoteParticipant::SipXRemoteParticipant(ConversationManager& conversationManager,
                                     SipXConversationManager& sipXConversationManager,
                                     DialogUsageManager& dum, 
                                     RemoteParticipantDialogSet& remoteParticipantDialogSet)
: Participant(ConversationManager::ParticipantType_Remote, conversationManager),
  RemoteParticipant(conversationManager, dum, remoteParticipantDialogSet),
  SipXParticipant(ConversationManager::ParticipantType_Remote, conversationManager, sipXConversationManager)
{
   InfoLog(<< "SipXRemoteParticipant created (UAS or forked leg), handle=" << mHandle);
}

SipXRemoteParticipant::~SipXRemoteParticipant()
{
   // Note:  Ideally this call would exist in the Participant Base class - but this call requires 
   //        dynamic_casts and virtual methods to function correctly during destruction.
   //        If the call is placed in the base Participant class then these things will not
   //        function as desired because a classes type changes as the descructors unwind.
   //        See https://stackoverflow.com/questions/10979250/usage-of-this-in-destructor.
   unregisterFromAllConversations();

   InfoLog(<< "SipXRemoteParticipant destroyed, handle=" << mHandle);
}

unsigned int 
SipXRemoteParticipant::getLocalRTPPort()
{
   SipXRemoteParticipantDialogSet* sipXDialogSet = getSipXDialogSet();
   if (sipXDialogSet)
   {
      return sipXDialogSet->getLocalRTPPort();
   }
   return 0;
}

//static const resip::ExtensionHeader h_AlertInfo("Alert-Info");


int 
SipXRemoteParticipant::getConnectionPortOnBridge()
{
   if(getDialogSet().getActiveRemoteParticipantHandle() == mHandle)
   {
      SipXRemoteParticipantDialogSet* sipXDialogSet = getSipXDialogSet();
      if (sipXDialogSet)
      {
         return sipXDialogSet->getConnectionPortOnBridge();
      }
      return -1;
   }
   else
   {
      // If this is not active fork leg, then we don't want to effect the bridge mixer.  
      // Note:  All forked endpoints/participants have the same connection port on the bridge
      return -1;
   }
}

int 
SipXRemoteParticipant::getMediaConnectionId()
{ 
   SipXRemoteParticipantDialogSet* sipXDialogSet = getSipXDialogSet();
   if (sipXDialogSet)
   {
      return sipXDialogSet->getMediaConnectionId();
   }
   return 0;
}

void
SipXRemoteParticipant::buildSdpOffer(bool holdSdp, SdpContents& offer)
{
   SdpContents::Session::Medium *audioMedium = 0;
   ConversationProfile *profile = getDialogSet().getConversationProfile().get();
   resip_assert(profile);

   // We need a copy of the session caps, since we modify them
   SdpContents sessionCaps = profile->sessionCaps();

   // If we already have a local sdp for this sesion, then use this to form the next offer - doing so will ensure
   // that we do not switch codecs or payload id's mid session.  
   if(getInviteSessionHandle().isValid() && getInviteSessionHandle()->getLocalSdp().session().media().size() != 0)
   {
      offer = getInviteSessionHandle()->getLocalSdp();

      // Set sessionid and version for this sdp
      UInt64 currentTime = Timer::getTimeMicroSec();
      offer.session().origin().getSessionId() = currentTime;
      offer.session().origin().getVersion() = currentTime;  

      // Find the audio medium
      for (std::list<SdpContents::Session::Medium>::iterator mediaIt = offer.session().media().begin();
           mediaIt != offer.session().media().end(); mediaIt++)
      {
         if(mediaIt->name() == "audio" && 
            (mediaIt->protocol() == Symbols::RTP_AVP ||
             mediaIt->protocol() == Symbols::RTP_SAVP ||
#ifdef RTP_SAVPF_FUDGE
             mediaIt->protocol() == Symbols::RTP_SAVPF ||
#endif
             mediaIt->protocol() == Symbols::UDP_TLS_RTP_SAVP))
         {
            audioMedium = &(*mediaIt);
            break;
         }
      }
      resip_assert(audioMedium);

      // Add any codecs from our capabilities that may not be in current local sdp - since endpoint may have changed and may now be capable 
      // of handling codecs that it previously could not (common when endpoint is a B2BUA).

      int highPayloadId = 96;  // Note:  static payload id's are in range of 0-96
      // Iterate through codecs in session caps and check if already in offer
      for (std::list<SdpContents::Session::Codec>::iterator codecsIt = sessionCaps.session().media().front().codecs().begin();
           codecsIt != sessionCaps.session().media().front().codecs().end(); codecsIt++)
      {		
         bool found=false;
         bool payloadIdCollision=false;
         for (std::list<SdpContents::Session::Codec>::iterator codecsIt2 = audioMedium->codecs().begin();
              codecsIt2 != audioMedium->codecs().end(); codecsIt2++)
         {
            if(isEqualNoCase(codecsIt->getName(), codecsIt2->getName()) &&
               codecsIt->getRate() == codecsIt2->getRate())
            {
               found = true;
            }
            else if(codecsIt->payloadType() == codecsIt2->payloadType())
            {
               payloadIdCollision = true;
            }
            // Keep track of highest payload id in offer - used if we need to resolve a payload id conflict
            if(codecsIt2->payloadType() > highPayloadId)
            {
               highPayloadId = codecsIt2->payloadType();
            }
         }
         if(!found)
         {
            if(payloadIdCollision)
            {
               highPayloadId++;
               codecsIt->payloadType() = highPayloadId;
            }
            else if(codecsIt->payloadType() > highPayloadId)
            {
               highPayloadId = codecsIt->payloadType();
            }
            audioMedium->addCodec(*codecsIt);
         }
      }
   }
   else
   {
      // Build base offer
      mConversationManager.buildSdpOffer(profile, offer);

      // Assumes there is only 1 media stream in session caps and it the audio one
      audioMedium = &offer.session().media().front();
      resip_assert(audioMedium);

      // Set the local RTP Port
      SipXRemoteParticipantDialogSet* sipXDialogSet = getSipXDialogSet();
      if (sipXDialogSet)
      {
         audioMedium->port() = sipXDialogSet->getLocalRTPPort();
      }
      else
      {
         audioMedium->port() = 0;
      }
   }

   // Add Crypto attributes (if required) - assumes there is only 1 media stream
   audioMedium->clearAttribute("crypto");
   audioMedium->clearAttribute("encryption");
   audioMedium->clearAttribute("tcap");
   audioMedium->clearAttribute("pcfg");
   offer.session().clearAttribute("fingerprint");
   offer.session().clearAttribute("setup");
   if(getDialogSet().getSecureMediaMode() == ConversationProfile::Srtp)
   {
      // Note:  We could add the crypto attribute to the "SDP Capabilties Negotiation" 
      //        potential configuration if secure media is not required - but other implementations 
      //        should ignore them any way if just plain RTP is used.  It is thought the 
      //        current implementation will increase interopability. (ie. SNOM Phones)

      Data crypto;

      SipXRemoteParticipantDialogSet* sipXDialogSet = getSipXDialogSet();
      if (sipXDialogSet)
      {
         switch (sipXDialogSet->getSrtpCryptoSuite())
         {
         case MediaConstants::SRTP_AES_CM_128_HMAC_SHA1_32:
            crypto = "1 AES_CM_128_HMAC_SHA1_32 inline:" + sipXDialogSet->getLocalSrtpSessionKey().base64encode();
            audioMedium->addAttribute("crypto", crypto);
            crypto = "2 AES_CM_128_HMAC_SHA1_80 inline:" + sipXDialogSet->getLocalSrtpSessionKey().base64encode();
            audioMedium->addAttribute("crypto", crypto);
            break;
         default:
            crypto = "1 AES_CM_128_HMAC_SHA1_80 inline:" + sipXDialogSet->getLocalSrtpSessionKey().base64encode();
            audioMedium->addAttribute("crypto", crypto);
            crypto = "2 AES_CM_128_HMAC_SHA1_32 inline:" + sipXDialogSet->getLocalSrtpSessionKey().base64encode();
            audioMedium->addAttribute("crypto", crypto);
            break;
         }
      }
      if(getDialogSet().getSecureMediaRequired())
      {
#ifdef RTP_SAVPF_FUDGE
         if(mDialogSet.peerExpectsSAVPF())
            audioMedium->protocol() = Symbols::RTP_SAVPF;
         else
#endif
            audioMedium->protocol() = Symbols::RTP_SAVP;
      }
      else
      {
         audioMedium->protocol() = Symbols::RTP_AVP;
         audioMedium->addAttribute("encryption", "optional");  // Used by SNOM phones?
         audioMedium->addAttribute("tcap", "1 RTP/SAVP");      // draft-ietf-mmusic-sdp-capability-negotiation-08
         audioMedium->addAttribute("pcfg", "1 t=1");
      }
   }
#ifdef USE_SSL
   else if(getDialogSet().getSecureMediaMode() == ConversationProfile::SrtpDtls)
   {
      if(mSipXConversationManager.getFlowManager().getDtlsFactory())
      {
         // Note:  We could add the fingerprint and setup attributes to the "SDP Capabilties Negotiation" 
         //        potential configuration if secure media is not required - but other implementations 
         //        should ignore them any way if just plain RTP is used.  It is thought the 
         //        current implementation will increase interopability.

         // Add fingerprint attribute
         char fingerprint[100];
         mSipXConversationManager.getFlowManager().getDtlsFactory()->getMyCertFingerprint(fingerprint);
         //offer.session().addAttribute("fingerprint", "SHA-1 " + Data(fingerprint));
         offer.session().addAttribute("fingerprint", "SHA-256 " + Data(fingerprint));  // Use SHA-256 for web-rtc compatibility
         //offer.session().addAttribute("acap", "1 fingerprint:SHA-1 " + Data(fingerprint));

         // Add setup attribute
         offer.session().addAttribute("setup", "actpass"); 

         if(getDialogSet().getSecureMediaRequired())
         {
            audioMedium->protocol() = Symbols::UDP_TLS_RTP_SAVP;
         }
         else
         {
            audioMedium->protocol() = Symbols::RTP_AVP;
            audioMedium->addAttribute("tcap", "1 UDP/TLS/RTP/SAVP");      // draft-ietf-mmusic-sdp-capability-negotiation-08
            audioMedium->addAttribute("pcfg", "1 t=1");
            //audioMedium->addAttribute("pcfg", "1 t=1 a=1");
         }
      }
   }
#endif

   audioMedium->clearAttribute("sendrecv");
   audioMedium->clearAttribute("sendonly");
   audioMedium->clearAttribute("recvonly");
   audioMedium->clearAttribute("inactive");

   if(holdSdp)
   {
      if(isRemoteHold())
      {
         audioMedium->addAttribute("inactive");
      }
      else
      {
         audioMedium->addAttribute("sendonly");
      }
   }
   else
   {
      if(isRemoteHold())
      {
         audioMedium->addAttribute("recvonly");
      }
      else
      {
         audioMedium->addAttribute("sendrecv");
      }
   }
   setProposedSdp(offer);
}

bool
SipXRemoteParticipant::answerMediaLine(SdpContents::Session::Medium& mediaSessionCaps, const sdpcontainer::SdpMediaLine& sdpMediaLine, SdpContents& answer, bool potential)
{
   sdpcontainer::SdpMediaLine::SdpTransportProtocolType protocolType = sdpMediaLine.getTransportProtocolType();
   bool valid = false;

   // If this is a valid audio medium then process it
   if(sdpMediaLine.getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO && 
      (protocolType == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_AVP ||
       protocolType == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVP ||
#ifdef RTP_SAVPF_FUDGE
       protocolType == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVPF ||
#endif
       protocolType == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_UDP_TLS_RTP_SAVP) && 
      sdpMediaLine.getConnections().size() != 0 &&
      sdpMediaLine.getConnections().front().getPort() != 0)
   {
      SdpContents::Session::Medium medium("audio", getLocalRTPPort(), 1, 
#ifndef RTP_SAVPF_FUDGE
                                          protocolType == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVP ? Symbols::RTP_SAVP :
#else
                                          ( protocolType == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVP
                                           || protocolType == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVPF)
                                          ? (protocolType == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVPF ? Symbols::RTP_SAVPF : Symbols::RTP_SAVP) :
#endif
                                          (protocolType == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_UDP_TLS_RTP_SAVP ? Symbols::UDP_TLS_RTP_SAVP :
                                           Symbols::RTP_AVP));

      // Check secure media properties and requirements
      bool secureMediaRequired = getDialogSet().getSecureMediaRequired() || protocolType != sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_AVP;
      SipXRemoteParticipantDialogSet* sipXDialogSet = getSipXDialogSet();
      if (sipXDialogSet)
      {
         sipXDialogSet->setPeerExpectsSAVPF(protocolType == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVPF);

         if (getDialogSet().getSecureMediaMode() == ConversationProfile::Srtp ||
#ifdef RTP_SAVPF_FUDGE
            protocolType == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVPF ||
#endif
            protocolType == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVP)  // allow accepting of SAVP profiles, even if SRTP is not enabled as a SecureMedia mode
         {
            bool supportedCryptoSuite = false;
            sdpcontainer::SdpMediaLine::CryptoList::const_iterator itCrypto = sdpMediaLine.getCryptos().begin();
            for (; !supportedCryptoSuite && itCrypto != sdpMediaLine.getCryptos().end(); itCrypto++)
            {
               Data cryptoKeyB64(itCrypto->getCryptoKeyParams().front().getKeyValue());
               Data cryptoKey = cryptoKeyB64.base64decode();

               if (cryptoKey.size() == SRTP_MASTER_KEY_LEN)
               {
                  switch (itCrypto->getSuite())
                  {
                  case sdpcontainer::SdpMediaLine::CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_80:
                     medium.addAttribute("crypto", Data(itCrypto->getTag()) + " AES_CM_128_HMAC_SHA1_80 inline:" + sipXDialogSet->getLocalSrtpSessionKey().base64encode());
                     supportedCryptoSuite = true;
                     break;
                  case sdpcontainer::SdpMediaLine::CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_32:
                     medium.addAttribute("crypto", Data(itCrypto->getTag()) + " AES_CM_128_HMAC_SHA1_32 inline:" + sipXDialogSet->getLocalSrtpSessionKey().base64encode());
                     supportedCryptoSuite = true;
                     break;
                  default:
                     break;
                  }
               }
               else
               {
                  InfoLog(<< "SDES crypto key found in SDP, but is not of correct length after base 64 decode: " << cryptoKey.size());
               }
            }
            if (!supportedCryptoSuite && secureMediaRequired)
            {
               InfoLog(<< "Secure media stream is required, but there is no supported crypto attributes in the offer - skipping this stream...");
               return false;
            }
         }
#ifdef USE_SSL
         else if (mSipXConversationManager.getFlowManager().getDtlsFactory() &&
            (getDialogSet().getSecureMediaMode() == ConversationProfile::SrtpDtls ||
               protocolType == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_UDP_TLS_RTP_SAVP))  // allow accepting of DTLS SAVP profiles, even if DTLS-SRTP is not enabled as a SecureMedia mode
         {
            bool supportedFingerprint = false;

            // We will only process Dtls-Srtp if fingerprint is in SHA-1 format
            if (sdpMediaLine.getFingerPrintHashFunction() == sdpcontainer::SdpMediaLine::FINGERPRINT_HASH_FUNC_SHA_1)
            {
               answer.session().clearAttribute("fingerprint");  // ensure we don't add these twice
               answer.session().clearAttribute("setup");  // ensure we don't add these twice

               // Add fingerprint attribute to answer
               char fingerprint[100];
               mSipXConversationManager.getFlowManager().getDtlsFactory()->getMyCertFingerprint(fingerprint);
               //answer.session().addAttribute("fingerprint", "SHA-1 " + Data(fingerprint));
               answer.session().addAttribute("fingerprint", "SHA-256 " + Data(fingerprint));  // Use SHA-256 for web-rtc compatibility

               // Add setup attribute
               if (sdpMediaLine.getTcpSetupAttribute() == sdpcontainer::SdpMediaLine::TCP_SETUP_ATTRIBUTE_ACTIVE)
               {
                  answer.session().addAttribute("setup", "passive");
               }
               else
               {
                  answer.session().addAttribute("setup", "active");
               }

               supportedFingerprint = true;
            }
            if (!supportedFingerprint && secureMediaRequired)
            {
               InfoLog(<< "Secure media stream is required, but there is no supported fingerprint attributes in the offer - skipping this stream...");
               return false;
            }
         }
#endif
      }

      if(potential && !sdpMediaLine.getPotentialMediaViewString().empty())
      {
         medium.addAttribute("acfg", sdpMediaLine.getPotentialMediaViewString());
      }
      
      // Iterate through codecs and look for supported codecs - tag found ones by storing their payload id
      sdpcontainer::SdpMediaLine::CodecList::const_iterator itCodec = sdpMediaLine.getCodecs().begin();
      for(; itCodec != sdpMediaLine.getCodecs().end(); itCodec++)
      {
         unsigned int itCodecRate = itCodec->getRate();
         if(itCodec->getMimeSubtype() == "G722" && itCodecRate != 8000)
         {
            WarningLog(<<"Peer is advertising G.722 with rate " << itCodecRate << ", overriding to 8000 (see RFC 3551 s4.5.2)");
            itCodecRate = 8000;
         }
         std::list<SdpContents::Session::Codec>::iterator bestCapsCodecMatchIt = mediaSessionCaps.codecs().end();
         bool modeInOffer = itCodec->getFormatParameters().prefix("mode=");

         // Loop through allowed codec list and see if codec is supported locally
         for (std::list<SdpContents::Session::Codec>::iterator capsCodecsIt = mediaSessionCaps.codecs().begin();
              capsCodecsIt != mediaSessionCaps.codecs().end(); capsCodecsIt++)
         {
            if(isEqualNoCase(capsCodecsIt->getName(), itCodec->getMimeSubtype()) &&
               (unsigned int)capsCodecsIt->getRate() == itCodecRate)
            {
               bool modeInCaps = capsCodecsIt->parameters().prefix("mode=");
               if(!modeInOffer && !modeInCaps)
               {
                  // If mode is not specified in either - then we have a match
                  bestCapsCodecMatchIt = capsCodecsIt;
                  break;
               }
               else if(modeInOffer && modeInCaps)
               {
                  if(isEqualNoCase(capsCodecsIt->parameters(), itCodec->getFormatParameters()))
                  {
                     bestCapsCodecMatchIt = capsCodecsIt;
                     break;
                  }
                  // If mode is specified in both, and doesn't match - then we have no match
               }
               else
               {
                  // Mode is specified on either offer or caps - this match is a potential candidate
                  // As a rule - use first match of this kind only
                  if(bestCapsCodecMatchIt == mediaSessionCaps.codecs().end())
                  {
                     bestCapsCodecMatchIt = capsCodecsIt;
                  }
               }
            }
         } 

         if(bestCapsCodecMatchIt != mediaSessionCaps.codecs().end())
         {
            SdpContents::Session::Codec codec(*bestCapsCodecMatchIt);
            codec.payloadType() = itCodec->getPayloadType();  // honour offered payload id - just to be nice  :)
            medium.addCodec(codec);
            if(!valid && !isEqualNoCase(bestCapsCodecMatchIt->getName(), "telephone-event"))
            {
               // Consider offer valid if we see any matching codec other than telephone-event
               valid = true;
            }
         }
      }
      
      if(valid)
      {
         // copy ptime attribute from session caps (if exists)
         if(mediaSessionCaps.exists("ptime"))
         {
            medium.addAttribute("ptime", mediaSessionCaps.getValues("ptime").front());
         }

         // Check requested direction
         unsigned int remoteRtpPort = sdpMediaLine.getConnections().front().getPort();
         if(sdpMediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_INACTIVE || 
           (isHolding() && (sdpMediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDONLY || remoteRtpPort == 0)))  // If remote inactive or both sides are holding
         {
            medium.addAttribute("inactive");
         }
         else if(sdpMediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDONLY || remoteRtpPort == 0 /* old RFC 2543 hold */)
         {
            medium.addAttribute("recvonly");
         }
         else if(sdpMediaLine.getDirection() == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_RECVONLY || isHolding())
         {
            medium.addAttribute("sendonly");
         }
         else
         {
            // Note:  sendrecv is the default in SDP
            medium.addAttribute("sendrecv");
         }
         answer.session().addMedium(medium);
      }
   }
   return valid;
}

bool
SipXRemoteParticipant::buildSdpAnswer(const SdpContents& offer, SdpContents& answer)
{
   // Note: this implementation has minimal support for draft-ietf-mmusic-sdp-capabilities-negotiation
   //       for responding "best-effort" / optional SRTP (Dtls-SRTP) offers

   bool valid = false;
   std::shared_ptr<sdpcontainer::Sdp> remoteSdp(SdpHelperResip::createSdpFromResipSdp(offer));

   try
   {
      // copy over session capabilities
      ConversationProfile *profile = getDialogSet().getConversationProfile().get();
      resip_assert(profile);

      answer = profile->sessionCaps();

      // Set sessionid and version for this answer
      UInt64 currentTime = Timer::getTimeMicroSec();
      answer.session().origin().getSessionId() = currentTime;
      answer.session().origin().getVersion() = currentTime;  

      // Set local port in answer
      // for now we only allow 1 audio media
      resip_assert(answer.session().media().size() == 1);
      SdpContents::Session::Medium& mediaSessionCaps = profile->sessionCaps().session().media().front();
      resip_assert(mediaSessionCaps.name() == "audio");
      resip_assert(mediaSessionCaps.codecs().size() > 0);

      // Copy t= field from sdp (RFC3264)
      resip_assert(answer.session().getTimes().size() > 0);
      if(offer.session().getTimes().size() >= 1)
      {
         answer.session().getTimes().clear();
         answer.session().addTime(offer.session().getTimes().front());
      }

      // Clear out m= lines in answer then populate below
      answer.session().media().clear();

      // Loop through each offered m= line and provide a response
      sdpcontainer::Sdp::MediaLineList::const_iterator itMediaLine = remoteSdp->getMediaLines().begin();
      for(; itMediaLine != remoteSdp->getMediaLines().end(); itMediaLine++)
      {
         bool mediaLineValid = false;

         // We only process one media stream - so if we already have a valid - just reject the rest
         if(valid)
         {
            SdpContents::Session::Medium rejmedium((*itMediaLine)->getMediaTypeString(), 0, 1,  // Reject medium by specifying port 0 (RFC3264)	
                                                   (*itMediaLine)->getTransportProtocolTypeString());
            if((*itMediaLine)->getCodecs().size() > 0)
            {
                rejmedium.addCodec(SdpContents::Session::Codec((*itMediaLine)->getCodecs().front().getMimeSubtype(), 
                                                               (*itMediaLine)->getCodecs().front().getRate(), 
                                                               (*itMediaLine)->getCodecs().front().getFormatParameters()));
                rejmedium.codecs().front().payloadType() = (*itMediaLine)->getCodecs().front().getPayloadType();
            }
            answer.session().addMedium(rejmedium);
            continue;
         }

         // Give preference to potential configuration first - if there are any
         sdpcontainer::SdpMediaLine::SdpMediaLineList::const_iterator itPotentialMediaLine = (*itMediaLine)->getPotentialMediaViews().begin();
         for(; itPotentialMediaLine != (*itMediaLine)->getPotentialMediaViews().end(); itPotentialMediaLine++)
         {
            mediaLineValid = answerMediaLine(mediaSessionCaps, *itPotentialMediaLine, answer, true);
            if(mediaLineValid)
            {
               // We have a valid potential media - line - copy over normal media line to make 
               // further processing easier
               *(*itMediaLine) = *itPotentialMediaLine;  
               valid = true;
               break;
            }
         }         
         if(!mediaLineValid) 
         {
            // Process SDP normally
            mediaLineValid = answerMediaLine(mediaSessionCaps, *(*itMediaLine), answer, false);
            if(!mediaLineValid)
            {
               SdpContents::Session::Medium rejmedium((*itMediaLine)->getMediaTypeString(), 0, 1,  // Reject medium by specifying port 0 (RFC3264)	
                                                      (*itMediaLine)->getTransportProtocolTypeString());
               if((*itMediaLine)->getCodecs().size() > 0)
               {
                  rejmedium.addCodec(SdpContents::Session::Codec((*itMediaLine)->getCodecs().front().getMimeSubtype(), 
                                                                 (*itMediaLine)->getCodecs().front().getRate(), 
                                                                 (*itMediaLine)->getCodecs().front().getFormatParameters()));
                  rejmedium.codecs().front().payloadType() = (*itMediaLine)->getCodecs().front().getPayloadType();
               }
               answer.session().addMedium(rejmedium);
            }
            else
            {
               valid = true;
            }
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

#ifdef OLD_CODE
// Note:  This old code used to serve 2 purposes - 
// 1 - that we do not change payload id's mid session
// 2 - that we do not add codecs or media streams that have previously rejected
// Purpose 2 is not correct.  RFC3264 states we need purpose 1, but you are allowed to add new codecs mid-session
//
// Decision to comment out this code and implement purpose 1 elsewhere - leaving this code here for reference (for now)
// as it may be useful for something in the future.
bool
SipXRemoteParticipant::formMidDialogSdpOfferOrAnswer(const SdpContents& localSdp, const SdpContents& remoteSdp, SdpContents& newSdp, bool offer)
{
   bool valid = false;

   try
   {
      // start with current localSdp
      newSdp = localSdp;

      // Clear all m= lines are rebuild
      newSdp.session().media().clear();

      // Set sessionid and version for this sdp
      UInt64 currentTime = Timer::getTimeMicroSec();
      newSdp.session().origin().getSessionId() = currentTime;
      newSdp.session().origin().getVersion() = currentTime;  

      // Loop through each m= line in local Sdp and remove or disable if not in remote
      for (std::list<SdpContents::Session::Medium>::const_iterator localMediaIt = localSdp.session().media().begin();
           localMediaIt != localSdp.session().media().end(); localMediaIt++)
      {
         for (std::list<SdpContents::Session::Medium>::const_iterator remoteMediaIt = remoteSdp.session().media().begin();
              remoteMediaIt != remoteSdp.session().media().end(); remoteMediaIt++)
         {
            if(localMediaIt->name() == remoteMediaIt->name() && localMediaIt->protocol() == remoteMediaIt->protocol())
            {
               // Found an m= line match, proceed to process codecs
               SdpContents::Session::Medium medium(localMediaIt->name(), localMediaIt->port(), localMediaIt->multicast(), localMediaIt->protocol());

               // Iterate through local codecs and look for remote supported codecs
               for (std::list<SdpContents::Session::Codec>::const_iterator localCodecsIt = localMediaIt->codecs().begin();
                    localCodecsIt != localMediaIt->codecs().end(); localCodecsIt++)
               {						
                  // Loop through remote supported codec list and see if codec is supported
                  for (std::list<SdpContents::Session::Codec>::const_iterator remoteCodecsIt = remoteMediaIt->codecs().begin();
                       remoteCodecsIt != remoteMediaIt->codecs().end(); remoteCodecsIt++)
                  {
                     if(isEqualNoCase(localCodecsIt->getName(), remoteCodecsIt->getName()) &&
                        localCodecsIt->getRate() == remoteCodecsIt->getRate())
                     {
                        // matching supported codec found - add to newSdp
                        medium.addCodec(*localCodecsIt);
                        if(!valid && !isEqualNoCase(localCodecsIt->getName(), "telephone-event"))
                        {
                           // Consider valid if we see any matching codec other than telephone-event
                           valid = true;
                        }
                        break;
                     }
                  }
               }

               // copy ptime attribute from session caps (if exists)
               if(localMediaIt->exists("ptime"))
               {
                  medium.addAttribute("ptime", localMediaIt->getValues("ptime").front());
               }

               if(offer)
               {
                  if(isHolding())
                  {
                     if(remoteMediaIt->exists("inactive") || 
                        remoteMediaIt->exists("sendonly") || 
                        remoteMediaIt->port() == 0)  // If remote inactive or both sides are holding
                     {
                        medium.addAttribute("inactive");
                     }
                     else
                     {
                        medium.addAttribute("sendonly");
                     }
                  }
                  else
                  {
                     if(remoteMediaIt->exists("inactive") || remoteMediaIt->exists("sendonly") || remoteMediaIt->port() == 0 /* old RFC 2543 hold */)
                     {
                        medium.addAttribute("recvonly");
                     }
                     else
                     {
                        medium.addAttribute("sendrecv");
                     }
                  }
               }
               else  // This is an sdp answer
               {
                  // Check requested direction
                  if(remoteMediaIt->exists("inactive") || 
                     (isHolding() && (remoteMediaIt->exists("sendonly") || remoteMediaIt->port() == 0)))  // If remote inactive or both sides are holding
                  {
                     medium.addAttribute("inactive");
                  }
                  else if(remoteMediaIt->exists("sendonly") || remoteMediaIt->port() == 0 /* old RFC 2543 hold */)
                  {
                     medium.addAttribute("recvonly");
                  }
                  else if(remoteMediaIt->exists("recvonly") || isHolding())
                  {
                     medium.addAttribute("sendonly");
                  }
                  else
                  {
                     // Note:  sendrecv is the default in SDP
                     medium.addAttribute("sendrecv");
                  }
               }

               newSdp.session().addMedium(medium);
               break;
            }
         }
      }
   }
   catch(BaseException &e)
   {
      WarningLog( << "formMidDialogSdpOfferOrAnswer: exception: " << e.getMessage());
      valid = false;
   }
   catch(...)
   {
      WarningLog( << "formMidDialogSdpOfferOrAnswer: unknown exception");
      valid = false;
   }

   return valid;
}
#endif

void
SipXRemoteParticipant::adjustRTPStreams(bool sendingOffer)
{
   //if(mHandle) mConversationManager.onParticipantMediaUpdate(mHandle, localSdp, remoteSdp);   
   int mediaDirection = sdpcontainer::SdpMediaLine::DIRECTION_TYPE_INACTIVE;
   Data remoteIPAddress;
   unsigned int remoteRtpPort=0;
   unsigned int remoteRtcpPort=0;
   std::shared_ptr<sdpcontainer::Sdp> localSdp(SdpHelperResip::createSdpFromResipSdp(sendingOffer ? *getDialogSet().getProposedSdp() : *getLocalSdp()));
   std::shared_ptr<sdpcontainer::Sdp> remoteSdp(sendingOffer ? 0 : SdpHelperResip::createSdpFromResipSdp(*getRemoteSdp()));
   const sdpcontainer::SdpMediaLine::CodecList* localCodecs = 0;
   const sdpcontainer::SdpMediaLine::CodecList* remoteCodecs = 0;
   bool supportedCryptoSuite = false;
   bool supportedFingerprint = false;

   resip_assert(localSdp);

   SipXRemoteParticipantDialogSet* sipXDialogSet = getSipXDialogSet();
   if (!sipXDialogSet)
   {
      // This would only ever happen on object destruction
      return;
   }

   /*
   InfoLog(<< "adjustRTPStreams: handle=" << mHandle << ", localSdp=" << localSdp);
   if(remoteSdp)
   {
      InfoLog(<< "adjustRTPStreams: handle=" << mHandle << ", remoteSdp=" << *remoteSdp);
   }*/

   int localMediaDirection = sdpcontainer::SdpMediaLine::DIRECTION_TYPE_INACTIVE;

   sdpcontainer::Sdp::MediaLineList::const_iterator itMediaLine = localSdp->getMediaLines().begin();
   for(; itMediaLine != localSdp->getMediaLines().end(); itMediaLine++)
   {
      DebugLog(<< "adjustRTPStreams: handle=" << mHandle << 
                  ", found media line in local sdp, mediaType=" << sdpcontainer::SdpMediaLine::SdpMediaTypeString[(*itMediaLine)->getMediaType()] << 
                  ", transportType=" << sdpcontainer::SdpMediaLine::SdpTransportProtocolTypeString[(*itMediaLine)->getTransportProtocolType()] << 
                  ", numConnections=" << (*itMediaLine)->getConnections().size() <<
                  ", port=" << ((*itMediaLine)->getConnections().size() > 0 ? (*itMediaLine)->getConnections().front().getPort() : 0));
      if((*itMediaLine)->getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO && 
         ((*itMediaLine)->getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_AVP ||
          (*itMediaLine)->getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVP ||
#ifdef RTP_SAVPF_FUDGE
          (*itMediaLine)->getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVPF ||
#endif
          (*itMediaLine)->getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_UDP_TLS_RTP_SAVP) && 
         (*itMediaLine)->getConnections().size() != 0 &&
         (*itMediaLine)->getConnections().front().getPort() != 0)
      {
         //InfoLog(<< "adjustRTPStreams: handle=" << mHandle << ", found audio media line in local sdp");
         localMediaDirection = (*itMediaLine)->getDirection();
         localCodecs = &(*itMediaLine)->getCodecs();
         break;
      }
   }
 
   if(remoteSdp)
   {
      int remoteMediaDirection = sdpcontainer::SdpMediaLine::DIRECTION_TYPE_INACTIVE;

      sdpcontainer::Sdp::MediaLineList::const_iterator itRemMediaLine = remoteSdp->getMediaLines().begin();
      for(; itRemMediaLine != remoteSdp->getMediaLines().end(); itRemMediaLine++)
      {
         //InfoLog(<< "adjustRTPStreams: handle=" << mHandle << ", found media line in remote sdp");
         if((*itRemMediaLine)->getMediaType() == sdpcontainer::SdpMediaLine::MEDIA_TYPE_AUDIO && 
            ((*itRemMediaLine)->getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_AVP ||
             (*itRemMediaLine)->getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVP ||
#ifdef RTP_SAVPF_FUDGE
             (*itRemMediaLine)->getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVPF ||
#endif
             (*itRemMediaLine)->getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_UDP_TLS_RTP_SAVP) && 
            (*itRemMediaLine)->getConnections().size() != 0 &&
            (*itRemMediaLine)->getConnections().front().getPort() != 0)
         {
            //InfoLog(<< "adjustRTPStreams: handle=" << mHandle << ", found audio media line in remote sdp");
            remoteMediaDirection = (*itRemMediaLine)->getDirection();
            remoteRtpPort = (*itRemMediaLine)->getConnections().front().getPort();
            remoteRtcpPort = (*itRemMediaLine)->getRtcpConnections().front().getPort();
            remoteIPAddress = (*itRemMediaLine)->getConnections().front().getAddress();
            remoteCodecs = &(*itRemMediaLine)->getCodecs();

            // Process Crypto settings (if required) - createSRTPSession using remote key
            // Note:  Top crypto in remote sdp will always be the correct suite/key
            if(getDialogSet().getSecureMediaMode() == ConversationProfile::Srtp ||
#ifdef RTP_SAVPF_FUDGE
               (*itRemMediaLine)->getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVPF ||
#endif
               (*itRemMediaLine)->getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_RTP_SAVP)
            {
               DebugLog(<<"adjustRTPStreams: handle=" << mHandle << ", process crypto settings");
               sdpcontainer::SdpMediaLine::CryptoList::const_iterator itCrypto = (*itRemMediaLine)->getCryptos().begin();
               for(; itCrypto != (*itRemMediaLine)->getCryptos().end(); itCrypto++)
               {
                  Data cryptoKeyB64(itCrypto->getCryptoKeyParams().front().getKeyValue());
                  Data cryptoKey = cryptoKeyB64.base64decode();
                  
                  if(cryptoKey.size() == SRTP_MASTER_KEY_LEN)
                  {
                     switch(itCrypto->getSuite())
                     {
                     case sdpcontainer::SdpMediaLine::CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_80:   
                        if(!sipXDialogSet->createSRTPSession(MediaConstants::SRTP_AES_CM_128_HMAC_SHA1_80, cryptoKey.data(), cryptoKey.size()))
                           InfoLog(<<"Failed creating SRTP session");
                        supportedCryptoSuite = true;
                        break;
                     case sdpcontainer::SdpMediaLine::CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_32:
                        if(!sipXDialogSet->createSRTPSession(MediaConstants::SRTP_AES_CM_128_HMAC_SHA1_32, cryptoKey.data(), cryptoKey.size()))
                           InfoLog(<<"Failed creating SRTP session");
                        supportedCryptoSuite = true;
                        break;
                     default:
                        break;
                     }
                  }
                  else
                  {
                     InfoLog(<< "SDES crypto key found in SDP, but is not of correct length after base 64 decode: " << cryptoKey.size());
                  }
                  if(supportedCryptoSuite)
                  {
                     StackLog(<< "Supported crypto suite found");
                     break;
                  }
               }
            }
            // Process Fingerprint and setup settings (if required) 
            else if((*itRemMediaLine)->getTransportProtocolType() == sdpcontainer::SdpMediaLine::PROTOCOL_TYPE_UDP_TLS_RTP_SAVP)
            {
               // We will only process Dtls-Srtp if fingerprint is in SHA-1 format
               if((*itRemMediaLine)->getFingerPrintHashFunction() == sdpcontainer::SdpMediaLine::FINGERPRINT_HASH_FUNC_SHA_1)
               {
                  if(!(*itRemMediaLine)->getFingerPrint().empty())
                  {
                     InfoLog(<< "Fingerprint retrieved from remote SDP: " << (*itRemMediaLine)->getFingerPrint());
                     // ensure we only accept media streams with this fingerprint
                     sipXDialogSet->setRemoteSDPFingerprint((*itRemMediaLine)->getFingerPrint());

                     // If remote setup value is not active then we must be the Dtls client  - ensure client DtlsSocket is create
                     if((*itRemMediaLine)->getTcpSetupAttribute() != sdpcontainer::SdpMediaLine::TCP_SETUP_ATTRIBUTE_ACTIVE)
                     {
                        // If we are the active end, then kick start the DTLS handshake
                        sipXDialogSet->startDtlsClient(remoteIPAddress.c_str(), remoteRtpPort, remoteRtcpPort);
                     }

                     supportedFingerprint = true;
                  }
               }
               else if((*itRemMediaLine)->getFingerPrintHashFunction() != sdpcontainer::SdpMediaLine::FINGERPRINT_HASH_FUNC_NONE)
               {
                  InfoLog(<< "Fingerprint found, but is not using SHA-1 hash.");
               }
            }

            break;
         }
      }

      if (remoteMediaDirection == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_INACTIVE ||
          remoteMediaDirection == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDONLY)
      {
          setRemoteHold(true);
      }
      else
      {
          setRemoteHold(false);
      }

      // Check if any conversations are broadcast only - if so, then we need to send media to parties on hold
      bool broadcastOnly = false;
      ConversationMap::iterator it;
      for(it = mConversations.begin(); it != mConversations.end(); it++)
      {
         if(it->second->broadcastOnly())
         {
            broadcastOnly = true;
            break;
         }
      }

      // Aggregate local and remote direction attributes to determine overall media direction
      if((isHolding() && !broadcastOnly) ||
         localMediaDirection == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_INACTIVE || 
         remoteMediaDirection == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_INACTIVE)
      {
         mediaDirection = sdpcontainer::SdpMediaLine::DIRECTION_TYPE_INACTIVE;
      }
      else if(localMediaDirection == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDONLY)
      {
         mediaDirection = sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDONLY;
      }
      else if(localMediaDirection == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_RECVONLY)
      {
         mediaDirection = sdpcontainer::SdpMediaLine::DIRECTION_TYPE_RECVONLY;
      }
      else if(remoteMediaDirection == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDONLY)
      {
         mediaDirection = sdpcontainer::SdpMediaLine::DIRECTION_TYPE_RECVONLY;
      }
      else if(remoteMediaDirection == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDONLY)
      {
         mediaDirection = sdpcontainer::SdpMediaLine::DIRECTION_TYPE_RECVONLY;
      }
      else
      {
         mediaDirection = sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV;
      }
   }
   else
   {
      // No remote SDP info - so put direction into receive only mode (unless inactive)
      if(isHolding() ||
         localMediaDirection == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_INACTIVE || 
         localMediaDirection == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDONLY)
      {
         mediaDirection = sdpcontainer::SdpMediaLine::DIRECTION_TYPE_INACTIVE;
      }
      else
      {
         mediaDirection = sdpcontainer::SdpMediaLine::DIRECTION_TYPE_RECVONLY;
      }
   }

   if(remoteSdp && getDialogSet().getSecureMediaRequired() && !supportedCryptoSuite && !supportedFingerprint)
   {
      InfoLog(<< "Secure media is required and no valid support found in remote sdp - ending call.");
      destroyParticipant();
      return;
   }

   InfoLog(<< "adjustRTPStreams: handle=" << mHandle << ", mediaDirection=" << sdpcontainer::SdpMediaLine::SdpDirectionTypeString[mediaDirection] << ", remoteIp=" << remoteIPAddress << ", remotePort=" << remoteRtpPort);

   if(!remoteIPAddress.empty() && remoteRtpPort != 0)
   {
      sipXDialogSet->setActiveDestination(remoteIPAddress.c_str(), remoteRtpPort, remoteRtcpPort);
   }

   if((mediaDirection == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV ||
       mediaDirection == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDONLY) &&
       !remoteIPAddress.empty() && remoteRtpPort != 0 && 
       remoteCodecs && localCodecs)
   {
      // Calculate intersection of local and remote codecs, and pass remote codecs that exist locally to RTP send fn
      int numCodecs=0;
      ::SdpCodec** codecs = new ::SdpCodec*[remoteCodecs->size()];
      sdpcontainer::SdpMediaLine::CodecList::const_iterator itRemoteCodec = remoteCodecs->begin();
      for(; itRemoteCodec != remoteCodecs->end(); itRemoteCodec++)
      {
         bool modeInRemote = itRemoteCodec->getFormatParameters().prefix("mode=");
         sdpcontainer::SdpMediaLine::CodecList::const_iterator bestCapsCodecMatchIt = localCodecs->end();
         sdpcontainer::SdpMediaLine::CodecList::const_iterator itLocalCodec = localCodecs->begin();
         for(; itLocalCodec != localCodecs->end(); itLocalCodec++)
         {
            if(isEqualNoCase(itRemoteCodec->getMimeSubtype(), itLocalCodec->getMimeSubtype()) &&
               itRemoteCodec->getRate() == itLocalCodec->getRate())
            {
               bool modeInLocal = itLocalCodec->getFormatParameters().prefix("mode=");
               if(!modeInLocal && !modeInRemote)
               {
                  // If mode is not specified in either - then we have a match
                  bestCapsCodecMatchIt = itLocalCodec;
                  break;
               }
               else if(modeInLocal && modeInRemote)
               {
                  if(isEqualNoCase(itRemoteCodec->getFormatParameters(), itLocalCodec->getFormatParameters()))
                  {
                     bestCapsCodecMatchIt = itLocalCodec;
                     break;
                  }
                  // If mode is specified in both, and doesn't match - then we have no match
               }
               else
               {
                  // Mode is specified on either offer or caps - this match is a potential candidate
                  // As a rule - use first match of this kind only
                  if(bestCapsCodecMatchIt == localCodecs->end())
                  {
                     bestCapsCodecMatchIt = itLocalCodec;
                  }
               }
            }
         }
         if(bestCapsCodecMatchIt != localCodecs->end())
         {
            codecs[numCodecs++] = new ::SdpCodec(itRemoteCodec->getPayloadType(), 
                                                 itRemoteCodec->getMimeType().c_str(), 
                                                 itRemoteCodec->getMimeSubtype().c_str(), 
                                                 itRemoteCodec->getRate(), 
                                                 itRemoteCodec->getPacketTime(), 
                                                 itRemoteCodec->getNumChannels(), 
                                                 itRemoteCodec->getFormatParameters().c_str());

            UtlString codecString;
            codecs[numCodecs-1]->toString(codecString);

            DebugLog(<< "adjustRTPStreams: handle=" << mHandle << ", sending codec: " << codecString.data());
         }
      }

      if(numCodecs > 0)
      {
         InfoLog(<< "adjustRTPStreams: handle=" << mHandle << ", starting to send for " << numCodecs << " codecs to destination address " << remoteIPAddress << ":" <<
            remoteRtpPort << " (RTCP on " << remoteRtcpPort << ")");
         int ret = getMediaInterface()->getInterface()->startRtpSend(sipXDialogSet->getMediaConnectionId(), numCodecs, codecs);
         if(ret != OS_SUCCESS)
         {
            InfoLog(<<"adjustRTPStreams: handle=" << mHandle << ", failed to start RTP send, ret = " << ret);
         }
      }
      else
      {
         WarningLog(<< "adjustRTPStreams: handle=" << mHandle << ", something went wrong during SDP negotiations, no common codec found.");
      }
      for(int i = 0; i < numCodecs; i++)
      {
         delete codecs[i];
      }
      delete [] codecs;
   }
   else
   {
      if(getMediaInterface()->getInterface()->isSendingRtpAudio(sipXDialogSet->getMediaConnectionId()))
      {
         getMediaInterface()->getInterface()->stopRtpSend(sipXDialogSet->getMediaConnectionId());
      }
      InfoLog(<< "adjustRTPStreams: handle=" << mHandle << ", stop sending.");
   }

   if((mediaDirection == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_SENDRECV ||
      mediaDirection == sdpcontainer::SdpMediaLine::DIRECTION_TYPE_RECVONLY)
            && localCodecs)
   {
      if(!getMediaInterface()->getInterface()->isReceivingRtpAudio(sipXDialogSet->getMediaConnectionId()))
      {
         // !SLG! - we could make this better, no need to recalculate this every time
         // We are always willing to receive any of our locally supported codecs
         int numCodecs=0;
         ::SdpCodec** codecs = new ::SdpCodec*[localCodecs->size()];
         sdpcontainer::SdpMediaLine::CodecList::const_iterator itLocalCodec = localCodecs->begin();
         for(; itLocalCodec != localCodecs->end(); itLocalCodec++)
         {
            codecs[numCodecs++] = new ::SdpCodec(itLocalCodec->getPayloadType(), 
                                                 itLocalCodec->getMimeType().c_str(), 
                                                 itLocalCodec->getMimeSubtype().c_str(), 
                                                 itLocalCodec->getRate(), 
                                                 itLocalCodec->getPacketTime(), 
                                                 itLocalCodec->getNumChannels(), 
                                                 itLocalCodec->getFormatParameters().c_str());
            UtlString codecString;
            codecs[numCodecs-1]->toString(codecString);
            DebugLog(<< "adjustRTPStreams: handle=" << mHandle << ", receiving: " << codecString.data());
         }
         InfoLog(<< "adjustRTPStreams: handle=" << mHandle << ", starting to receive for " << numCodecs << " codecs");

         getMediaInterface()->getInterface()->startRtpReceive(sipXDialogSet->getMediaConnectionId(), numCodecs, codecs);
         for(int i = 0; i < numCodecs; i++)
         {
            delete codecs[i];
         }
         delete [] codecs;
      }
      DebugLog(<< "adjustRTPStreams: handle=" << mHandle << ", receiving...");
   }
   else
   {
      // Never stop receiving - keep reading buffers and let mixing matrix handle supression of audio output
      //if(getMediaInterface()->getInterface()->isReceivingRtpAudio(getSipXDialogSet().getMediaConnectionId()))
      //{
      //   getMediaInterface()->getInterface()->stopRtpReceive(getSipXDialogSet().getMediaConnectionId());
      //}
      //InfoLog(<< "adjustRTPStreams: handle=" << mHandle << ", stop receiving (mLocalHold=" << isHolding() << ").");
   }
}

bool
SipXRemoteParticipant::mediaStackPortAvailable()
{
   return getLocalRTPPort() != 0;
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
