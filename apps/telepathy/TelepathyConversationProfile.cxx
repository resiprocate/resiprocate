/*
 * Copyright (C) 2015 Daniel Pocock http://danielpocock.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/stack/SdpContents.hxx>
#include <resip/stack/SipMessage.hxx>
#include <resip/stack/Tuple.hxx>
#include <resip/recon/ReconSubsystem.hxx>

#include "TelepathyConversationProfile.hxx"


#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

using namespace tr;
using namespace resip;

TelepathyConversationProfile::TelepathyConversationProfile(SharedPtr<Profile> baseProfile, const QVariantMap &parameters)
   : ConversationProfile(baseProfile),
     TelepathyParameters(parameters)
{
   bool registrationDisabled = false;
   if(contact().uri().user() != "noreg" && !registrationDisabled)
   {  
      setDefaultRegistrationTime(60);
   }
   else
   {
      setDefaultRegistrationTime(0);
   }
   setDefaultRegistrationRetryTime(120);  // 2 mins
   setDefaultFrom(contact());
   setDigestCredential(realm(), authUser(), password());

   // Create Session Capabilities and assign to coversation Profile
   // Note:  port, sessionId and version will be replaced in actual offer/answer   int port = 16384;
   // Build s=, o=, t=, and c= lines
   SdpContents::Session::Origin origin("-", 0 /* sessionId */, 0 /* version */, SdpContents::IP4, getDefaultAddress());   // o=
   SdpContents::Session session(0, origin, "-" /* s= */);
   session.connection() = SdpContents::Session::Connection(SdpContents::IP4, getDefaultAddress());  // c=
   session.addTime(SdpContents::Session::Time(0, 0));

   int defaultMediaPort = 25000;   // FIXME - should this be detected or linked to port allocation?
   // Build Codecs and media offering
   SdpContents::Session::Medium medium("audio", defaultMediaPort, 1, "RTP/AVP");
   SdpContents::Session::Codec g722codec("G722", 8000);
   g722codec.payloadType() = 9;  /* RFC3551 */ ;
   medium.addCodec(g722codec);

   // Note: the other constructors (e.g. g722 above) are
   // invoked incorrectly, payload format should be the second
   // argument or the rate is used as the payload format and
   // the desired rate is not used at all

   SdpContents::Session::Codec opuscodec("OPUS", 96, 48000);
   opuscodec.encodingParameters() = Data("2");
   medium.addCodec(opuscodec);

   SdpContents::Session::Codec g711ucodec("PCMU", 8000);
   g711ucodec.payloadType() = 0;  /* RFC3551 */ ;
   medium.addCodec(g711ucodec);

   SdpContents::Session::Codec g711acodec("PCMA", 8000);
   g711acodec.payloadType() = 8;  /* RFC3551 */ ;
   medium.addCodec(g711acodec);

   SdpContents::Session::Codec speexCodec("SPEEX", 8000);
   speexCodec.payloadType() = 110;
   speexCodec.parameters() = Data("mode=3");
   medium.addCodec(speexCodec);

   SdpContents::Session::Codec gsmCodec("GSM", 8000);
   gsmCodec.payloadType() = 3;  /* RFC3551 */ ;
   medium.addCodec(gsmCodec);

   medium.addAttribute("ptime", Data(20));  // 20 ms of speech per frame (note G711 has 10ms samples, so this is 2 samples per frame)
   medium.addAttribute("sendrecv");

   SdpContents::Session::Codec toneCodec("telephone-event", 8000);
   toneCodec.payloadType() = 102;
   toneCodec.parameters() = Data("0-15");
   medium.addCodec(toneCodec);
   session.addMedium(medium);

   sessionCaps().session() = session;

   // Setup NatTraversal Settings
   
   Data turnServer = getString("turn-server");
   DebugLog(<<"turn-server value = " << turnServer);
   if(turnServer.empty())
   {
      natTraversalMode() = ConversationProfile::NoNatTraversal; // FIXME - hardcode for TURN? detect TURN for domain by SRV lookup?
   }
   else
   {
      DebugLog(<<"will use TurnUdpAllocation mode");
      natTraversalMode() = ConversationProfile::TurnUdpAllocation;
      int defaultTurnPort = 3478;
      natTraversalServerHostname() = turnServer;
      natTraversalServerPort() = defaultTurnPort;
      stunUsername() = authUser();
      stunPassword() = password();
   }

   // Secure Media Settings
   secureMediaMode() = ConversationProfile::NoSecureMedia;
   secureMediaRequired() = false;
   secureMediaDefaultCryptoSuite() = ConversationProfile::SRTP_AES_CM_128_HMAC_SHA1_80;
}

Data
TelepathyConversationProfile::getDefaultAddress()
{
   return "0.0.0.0"; // FIXME - IPV6
}


