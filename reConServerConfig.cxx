#include <fstream>
#include <iostream>

#include "os/OsIntTypes.h"

#include "reConServerConfig.hxx"

#include <AppSubsystem.hxx>

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
ReConServerConfig::getConfigValue(const resip::Data& name, ConversationProfile::SecureMediaMode &value)
{
   Data lowerName(name);  lowerName.lowercase();
   ConfigValuesMap::iterator it = mConfigValues.find(lowerName);
   if(it != mConfigValues.end())
   {
      if(isEqualNoCase(it->second, "None"))
      {
         value = ConversationProfile::NoSecureMedia;
      }
      else if(isEqualNoCase(it->second, "Srtp"))
      {
         value = ConversationProfile::Srtp;
      }
      else if(isEqualNoCase(it->second, "SrtpReq"))
      {
         mSecureMediaRequired = true;
         value = ConversationProfile::Srtp;
      }
#ifdef USE_SSL
      else if(isEqualNoCase(it->second, "SrtpDtls"))
      {
         value = ConversationProfile::SrtpDtls;
      }
      else if(isEqualNoCase(it->second, "SrtpDtlsReq"))
      {
         mSecureMediaRequired = true;
         value = ConversationProfile::SrtpDtls;
      }
#endif
      else
      {
         cerr << "Invalid Secure Media Mode: " << it->second << endl;
         return false;
      }
   }
   // Not found
   return false;
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
   Data lowerName(name);  lowerName.lowercase();
   ConfigValuesMap::iterator it = mConfigValues.find(lowerName);
   if(it != mConfigValues.end())
   {
      if(isEqualNoCase(it->second, "None"))
      {
         value = ConversationProfile::NoNatTraversal;
      }
      else if(isEqualNoCase(it->second, "Bind"))
      {
         value = ConversationProfile::StunBindDiscovery;
      }
      else if(isEqualNoCase(it->second, "UdpAlloc"))
      {
         value = ConversationProfile::TurnUdpAllocation;
      }
      else if(isEqualNoCase(it->second, "TcpAlloc"))
      {
         value = ConversationProfile::TurnTcpAllocation;
      }
#ifdef USE_SSL
      else if(isEqualNoCase(it->second, "TlsAlloc"))
      {
         value = ConversationProfile::TurnTlsAllocation;
      }
#endif
      else
      {
         cerr << "Invalid NAT Traversal Mode: " << it->second << endl;
         exit(-1);
      }
   }
   // Not found
   return false;
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
   Data lowerName(name);
   lowerName.lowercase();

   ConfigValuesMap::iterator it = mConfigValues.find(lowerName);
   if(it != mConfigValues.end())
   {
      if(isEqualNoCase(it->second, "None"))
      {
         value = None;
      }
      else if(isEqualNoCase(it->second, "B2BUA"))
      {
         value = B2BUA;
      }
      else
      {
         cerr << "Invalid Application: " << it->second << endl;
         exit(-1);
      }
   }
   // Not found
   return false;
}

ReConServerConfig::Application
ReConServerConfig::getConfigApplication(const resip::Data& name, const ReConServerConfig::Application defaultValue)
{
   ReConServerConfig::Application ret = defaultValue;
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

