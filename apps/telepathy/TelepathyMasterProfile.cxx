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

#include "TelepathyMasterProfile.hxx"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/stack/SdpContents.hxx>
#include <resip/stack/SipMessage.hxx>
#include <resip/stack/Tuple.hxx>
#include <resip/stack/Pidf.hxx>
#include <resip/recon/ReconSubsystem.hxx>

#include "MyMessageDecorator.hxx"

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

using namespace tr;
using namespace resip;

TelepathyMasterProfile::TelepathyMasterProfile(const QVariantMap &parameters)
   : TelepathyParameters(parameters)
{
#ifdef USE_SSL
   Data rootCertPath = getString("root-cert-path");
   if(rootCertPath.empty())
   {
      ErrLog(<<"root-cert-path was not set or not obtained correctly from the Telepathy settings API, using /etc/ssl/certs");
      rootCertPath = "/etc/ssl/certs";
   }
   certPath() = rootCertPath;
   InfoLog(<<"Using certPath(): " << rootCertPath);
   addTransport(TLS, 0, V4);
   addTransport(TLS, 0, V6);
#endif
   addTransport(TCP, 0, V4);
   addTransport(TCP, 0, V6);
   addTransport(UDP, 0, V4);
   addTransport(UDP, 0, V6);

   // Settings
   setDefaultRegistrationTime(60);

   setDefaultFrom(contact());

   setDigestCredential(realm(), authUser(), password());

   // Disable Statisitics Manager
   statisticsManagerEnabled() = false;

   bool keepAlivesEnabled = true;
   if(keepAlivesEnabled)
   {
      setKeepAliveTimeForDatagram(30);
      setKeepAliveTimeForStream(180);
   }

   // Support Methods, etc.
   validateContentEnabled() = false;
   validateContentLanguageEnabled() = false;
   validateAcceptEnabled() = false;

   clearSupportedLanguages();
   addSupportedLanguage(Token("en"));

   clearSupportedMimeTypes();
   addSupportedMimeType(MESSAGE, Mime("text", "plain"));
   addSupportedMimeType(INVITE, Mime("application", "sdp"));
   addSupportedMimeType(INVITE, Mime("multipart", "mixed"));
   addSupportedMimeType(INVITE, Mime("multipart", "signed"));
   addSupportedMimeType(INVITE, Mime("multipart", "alternative"));
   addSupportedMimeType(OPTIONS,Mime("application", "sdp"));
   addSupportedMimeType(OPTIONS,Mime("multipart", "mixed"));
   addSupportedMimeType(OPTIONS, Mime("multipart", "signed"));
   addSupportedMimeType(OPTIONS, Mime("multipart", "alternative"));
   addSupportedMimeType(PRACK,  Mime("application", "sdp"));
   addSupportedMimeType(PRACK,  Mime("multipart", "mixed"));
   addSupportedMimeType(PRACK,  Mime("multipart", "signed"));
   addSupportedMimeType(PRACK,  Mime("multipart", "alternative"));
   addSupportedMimeType(UPDATE, Mime("application", "sdp"));
   addSupportedMimeType(UPDATE, Mime("multipart", "mixed"));
   addSupportedMimeType(UPDATE, Mime("multipart", "signed"));
   addSupportedMimeType(UPDATE, Mime("multipart", "alternative"));
   addSupportedMimeType(NOTIFY, Mime("message", "sipfrag"));
   addSupportedMimeType(NOTIFY, Pidf::getStaticType());
   addSupportedMimeType(INFO, Mime("application", "dtmf-relay"));

   clearSupportedMethods();
   addSupportedMethod(INVITE);
   addSupportedMethod(ACK);
   addSupportedMethod(CANCEL);
   addSupportedMethod(OPTIONS);
   addSupportedMethod(BYE);
   addSupportedMethod(REFER);
   addSupportedMethod(NOTIFY);
   addSupportedMethod(SUBSCRIBE);
   addSupportedMethod(UPDATE);
   addSupportedMethod(PRACK);
   addSupportedMethod(INFO);
   addSupportedMethod(MESSAGE);

   clearSupportedOptionTags();
   addSupportedOptionTag(Token(Symbols::Replaces));
   addSupportedOptionTag(Token(Symbols::Timer));
   addSupportedOptionTag(Token(Symbols::NoReferSub));
   addSupportedOptionTag(Token(Symbols::AnswerMode));
   addSupportedOptionTag(Token(Symbols::TargetDialog));
   //addSupportedOptionTag(Token(Symbols::C100rel));  // Automatically added by calling setUacReliableProvisionalMode

   setUacReliableProvisionalMode(MasterProfile::Supported);

   clearSupportedSchemes();
   addSupportedScheme("sip");
#ifdef USE_SSL
   addSupportedScheme("sips");
#endif

   // Have stack add Allow/Supported/Accept headers to INVITE dialog establishment messages
   clearAdvertisedCapabilities(); // Remove Profile Defaults, then add our preferences
   addAdvertisedCapability(Headers::Allow);
   //addAdvertisedCapability(Headers::AcceptEncoding);  // This can be misleading - it might specify what is expected in response
   addAdvertisedCapability(Headers::AcceptLanguage);
   addAdvertisedCapability(Headers::Supported);
   setMethodsParamEnabled(true);

   //setOverrideHostAndPort(mContact);
   NameAddr outboundProxy("sip:" + contact().uri().host());  // FIXME - best way to do this?
   if(!outboundProxy.uri().host().empty())
   {
      setOutboundProxy(outboundProxy.uri());
   }

   unsigned short mediaPortStart = 25000;
   setUserAgent("telepathy-resiprocate");
   rtpPortRangeMin() = mediaPortStart;
   rtpPortRangeMax() = mediaPortStart + 101; // Allows 100 media streams

   // This should only be used when there is no NAT traversal mode
   SharedPtr<MessageDecorator> md(new MyMessageDecorator());
   setOutboundDecorator(md);
}

