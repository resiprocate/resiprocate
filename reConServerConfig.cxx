#include <fstream>
#include <iostream>

#include "os/OsIntTypes.h"

#include "reConServerConfig.hxx"

#include <recon/ReconSubsystem.hxx>

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

using namespace std;
using namespace resip;

namespace recon {

ReConServerConfig::ReConServerConfig(): mSecureMediaRequired(false)
{
}

ReConServerConfig::~ReConServerConfig()
{
}

bool 
ReConServerConfig::getConfigValue(const resip::Data& name, resip::NameAddr &value)
{
   Data lowerName(name);  lowerName.lowercase();
   ConfigValuesMap::iterator it = mConfigValues.find(lowerName);
   if(it != mConfigValues.end())
   {
      try
      {
         if(!it->second.empty())
         {
            NameAddr tempuri(it->second);
            value = tempuri;
            return true;
         }
         else
         {
            value = NameAddr();
            return true; 
         }
      }
      catch(resip::BaseException& e)
      {
         cerr << "Invalid uri format: " << e << endl;
         return false;
      }
   }
   // Not found
   return false;
}

resip::NameAddr
ReConServerConfig::getConfigNameAddr(const resip::Data& name, const resip::NameAddr defaultValue,  bool useDefaultIfEmpty)
{
   resip::NameAddr ret(defaultValue);
   if(getConfigValue(name, ret) && ret.uri().host().empty() && useDefaultIfEmpty)
   {
      return defaultValue;
   }
   return ret;
}


bool 
ReConServerConfig::getConfigValue(const resip::Data& name, ConversationProfile::SecureMediaMode &value)
{
   Data lowerName(name);  lowerName.lowercase();
   ConfigValuesMap::iterator it = mConfigValues.find(lowerName);
   if(it != mConfigValues.end())
   {
      if(isEqualNoCase(it->second, "Srtp"))
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
      if(isEqualNoCase(it->second, "Bind"))
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


void
ReConServerConfig::printHelpText(int argc, char **argv)
{
   cout << "Command line format is:" << endl;
   cout << "  " << removePath(argv[0]) << " [<ConfigFilename>] [--<ConfigValueName>=<ConfigValue>] [--<ConfigValueName>=<ConfigValue>] ..." << endl;
   cout << "Sample Command lines:" << endl;
   cout << "  " << removePath(argv[0]) << " reConServer.config --IPAddress=192.168.1.100 --SIPUri=sip:1000@myproxy.com --Password=123 --EnableAutoAnswer=true" << endl;
}

} // namespace

