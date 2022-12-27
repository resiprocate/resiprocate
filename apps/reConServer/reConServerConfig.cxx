#include <fstream>
#include <iostream>

#ifdef USE_SIPXTAPI
#include "os/OsIntTypes.h"
#endif

#include "reConServerConfig.hxx"

#include "AppSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM AppSubsystem::RECONSERVER

using namespace std;
using namespace resip;
using namespace recon;

namespace reconserver {

ReConServerConfig::ReConServerConfig(): mSecureMediaRequired(false)
{
}

ReConServerConfig::~ReConServerConfig()
{
}

bool
ReConServerConfig::getConfigValue(const resip::Data& name, recon::ConversationManager::AutoHoldMode& value) const
{
   std::map<recon::ConversationManager::AutoHoldMode, Data> dict;
   dict[recon::ConversationManager::AutoHoldDisabled] = "AutoHoldDisabled";
   dict[recon::ConversationManager::AutoHoldEnabled] = "AutoHoldEnabled";
   dict[recon::ConversationManager::AutoHoldBroadcastOnly] = "AutoHoldBroadcastOnly";
   return translateConfigValue<recon::ConversationManager::AutoHoldMode>(dict, name, value);
}

recon::ConversationManager::AutoHoldMode
ReConServerConfig::getConfigAutoHoldMode(const resip::Data& name,
   const recon::ConversationManager::AutoHoldMode& defaultValue) const
{
   recon::ConversationManager::AutoHoldMode ret = defaultValue;
   getConfigValue(name, ret);
   return ret;
}

bool
ReConServerConfig::getConfigValue(const resip::Data& name, recon::ConversationProfile::MediaEndpointMode& value)
{
   std::map<recon::ConversationProfile::MediaEndpointMode, Data> dict;
   dict[ConversationProfile::Base] = "Base";
   dict[ConversationProfile::WebRTC] = "WebRTC";
   return translateConfigValue<recon::ConversationProfile::MediaEndpointMode>(dict, name, value);
}

recon::ConversationProfile::MediaEndpointMode
ReConServerConfig::getConfigMediaEndpointMode(const resip::Data& name, const recon::ConversationProfile::MediaEndpointMode defaultValue)
{
   recon::ConversationProfile::MediaEndpointMode ret = defaultValue;
   getConfigValue(name, ret);
   return ret;
}

bool 
ReConServerConfig::getConfigValue(const resip::Data& name, ConversationProfile::SecureMediaMode &value)
{
   std::map<ConversationProfile::SecureMediaMode, Data> dict;
   dict[ConversationProfile::NoSecureMedia] = "None";
   dict[ConversationProfile::Srtp] = "Srtp";
   dict[ConversationProfile::SrtpReq] = "SrtpReq";
#ifdef USE_SSL
   dict[ConversationProfile::SrtpDtls] = "SrtpDtls";
   dict[ConversationProfile::SrtpDtlsReq] = "SrtpDtlsReq";
#endif
   bool success = translateConfigValue<ConversationProfile::SecureMediaMode>(dict, name, value);
   if(success)
   {
      if(value == ConversationProfile::SrtpReq)
      {
         value = ConversationProfile::Srtp;
         mSecureMediaRequired = true;
      }
#ifdef USE_SSL
      else if(value == ConversationProfile::SrtpDtlsReq)
      {
         value = ConversationProfile::SrtpDtls;
         mSecureMediaRequired = true;
      }
#endif
   }
   return success;
}

ConversationProfile::SecureMediaMode
ReConServerConfig::getConfigSecureMediaMode(const resip::Data& name, const ConversationProfile::SecureMediaMode defaultValue)
{
   ConversationProfile::SecureMediaMode ret = defaultValue;
   getConfigValue(name, ret);
   return ret;
}


bool ReConServerConfig::isSecureMediaModeRequired()
{
   return mSecureMediaRequired;
}


bool 
ReConServerConfig::getConfigValue(const resip::Data& name, ConversationProfile::NatTraversalMode &value)
{
   std::map<ConversationProfile::NatTraversalMode, Data> dict;
   dict[ConversationProfile::NoNatTraversal] = "None";
   dict[ConversationProfile::StunBindDiscovery] = "Bind";
   dict[ConversationProfile::TurnUdpAllocation] = "UdpAlloc";
   dict[ConversationProfile::TurnTcpAllocation] = "TcpAlloc";
#ifdef USE_SSL
   dict[ConversationProfile::TurnTlsAllocation] = "TlsAlloc";
#endif
   return translateConfigValue<ConversationProfile::NatTraversalMode>(dict, name, value);
}

ConversationProfile::NatTraversalMode
ReConServerConfig::getConfigNatTraversalMode(const resip::Data& name, const ConversationProfile::NatTraversalMode defaultValue)
{
   ConversationProfile::NatTraversalMode ret = defaultValue;
   getConfigValue(name, ret);
   return ret;
}

bool
ReConServerConfig::getConfigValue(const resip::Data& name, ReConServerConfig::Application& value)
{
   std::map<ReConServerConfig::Application, Data> dict;
   dict[None] = "None";
   dict[B2BUA] = "B2BUA";
   return translateConfigValue<ReConServerConfig::Application>(dict, name, value);
}

ReConServerConfig::Application
ReConServerConfig::getConfigApplication(const resip::Data& name, const ReConServerConfig::Application defaultValue)
{
   ReConServerConfig::Application ret = defaultValue;
   getConfigValue(name, ret);
   return ret;
}

bool
ReConServerConfig::getConfigValue(const resip::Data& name, ReConServerConfig::MediaStack& value) const
{
   std::map<ReConServerConfig::MediaStack, Data> dict;
   dict[sipXtapi] = "sipXtapi";
   dict[Kurento] = "Kurento";
   dict[Gstreamer] = "Gstreamer";
   dict[LibWebRTC] = "LibWebRTC";
   return translateConfigValue<ReConServerConfig::MediaStack>(dict, name, value);
}

ReConServerConfig::MediaStack
ReConServerConfig::getConfigMediaStack(const resip::Data& name, const ReConServerConfig::MediaStack defaultValue) const
{
   ReConServerConfig::MediaStack ret = defaultValue;
   getConfigValue(name, ret);
   return ret;
}

void
ReConServerConfig::printHelpText(int argc, char **argv)
{
   cout << "Command line format is:" << endl;
   cout << "  " << removePath(argv[0]) << " [<ConfigFilename>] [--<ConfigValueName>=<ConfigValue>] [--<ConfigValueName>=<ConfigValue>] ..." << endl;
   cout << "Sample Command lines:" << endl;
   cout << "  " << removePath(argv[0]) << " reConServer.config --IPAddress=192.168.1.100 --SIPUri=sip:1000@myproxy.com --Password=123 --EnableAutoAnswer=true" << endl;
}

} // namespace


/* ====================================================================
 *
 * Copyright 2013 Catalin Constantin Usurelu.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

