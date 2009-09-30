#include "ConfigParser.hxx"
#include "Server.hxx"

#include "AppSubsystem.hxx"

#include <iostream>
#include <fstream>
#include <iterator>

#include <rutil/DnsUtil.hxx>
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace gateway;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::GATEWAY

namespace gateway 
{

ConfigParser::ConfigParser(int argc, char** argv) : 
   // Defaults
   mSipPort(5070),
   mTlsPort(5071),
   mTlsDomain(DnsUtil::getLocalHostName()),
   mRegistrationTime(3600),
   mRegistrationRetryTime(120),
   mKeepAlives(true),
   mLogLevel("INFO"),
   mLogFilename("ichat-gw.log"),
   mLogFileMaxLines(50000),     // 50000 is about 5M size
   mGatewayIPCPort(2078),
   mJabberConnectorIPCPort(2079),
   mHttpPort(5090),
   mHttpAuth(false),
   mHttpAuthPwd("admin"),
   mIChatProceedingTimeout(5000),
   mAlwaysRelayIChatMedia(true),
   mPreferIPv6(false),
   mSkipFirstIChatAddress(false),
   mMediaRelayPortRangeMin(8000),
   mMediaRelayPortRangeMax(9999),
   mJabberComponentPort(5275),
   mJabberServerPingDuration(60)  // 1 min
{
   // Parse config file first
   parseConfigFile("ichat-gw.config");

   // Parse command line options
   // Note:  command line overrides config file setting
   parseCommandLine(argc, argv);
}

ConfigParser::~ConfigParser()
{
}

void 
ConfigParser::parseCommandLine(int argc, char** argv)
{
   // Loop through command line arguments and process them
   for(int i = 1; i < argc; i++)
   {
      Data commandName(argv[i]);

      // Process all commandNames that don't take values
      if(isEqualNoCase(commandName, "-?") || 
         isEqualNoCase(commandName, "--?") ||
         isEqualNoCase(commandName, "--help") ||
         isEqualNoCase(commandName, "/?"))
      {
         cout << "Command line options are:" << endl;
         cout << " -a <IP Address> - bind SIP transports to this IP address" << endl;
         cout << " -d <DNS servers> - comma seperated list of DNS servers, overrides OS detected list" << endl;
         cout << " -gi <Gateway identity> - used in From header of click-to-call requests to initiator" << endl;
         cout << " -sp <port num> - local port number to use for SIP messaging (UDP/TCP)" << endl;
         cout << " -tp <port num> - local port number to use for TLS SIP messaging" << endl;
         cout << " -td <domain name> - domain name to use for TLS server connections" << endl;
         cout << " -nk - no keepalives, set this to disable sending of keepalives" << endl;
         cout << " -op <SIP URI> - URI of a proxy server to use a SIP outbound proxy" << endl;
         cout << " -l <NONE|CRIT|ERR|WARNING|INFO|DEBUG|STACK> - logging level" << endl;
         cout << endl;
         cout << "Sample Command line:" << endl;
         cout << "ichat-gw -a 192.168.1.100 -l DEBUG" << endl;
         exit(0);
      }
      else if(isEqualNoCase(commandName, "-nk"))
      {
         mKeepAlives = false;
      }
      else if(commandName.at(0) == '-')
      {
         commandName = commandName.substr(1); // Remove -

         // Process commands that have values
         Data commandValue(i+1 < argc ? argv[i+1] : Data::Empty);
         if(commandValue.empty() || commandValue.at(0) == '-')
         {
            cerr << "Invalid command line parameters!" << endl;
            exit(-1);
         }
         i++;  // increment argument

         //cout << "Command Line Name='" << commandName << "' value='" << commandValue << "'" << endl;
         if(!processOption(commandName.lowercase(), commandValue))
         {
            cerr << "Invalid command line parameters!" << endl;
            exit(-1);
         }
      }
      else
      {
         cerr << "Invalid command line parameters!" << endl;
         exit(-1);
      }
   }
}

void
ConfigParser::parseConfigFile(const Data& filename)
{
   ifstream configFile(filename.c_str());
    
   string sline;                     
   while(getline(configFile, sline)) 
   {
      Data line(sline);
      Data name;
      Data value;
      ParseBuffer pb(line);

      pb.skipWhitespace();
      const char * anchor = pb.position();
      if(pb.eof() || *anchor == '#') continue;  // if line is a comment or blank then skip it
      // Look for =
      pb.skipToOneOf("= \t");
      if(!pb.eof())
      {
         pb.data(name,anchor);
         if(*pb.position()!='=') 
         {
            pb.skipToChar('=');
         }
         pb.skipChar('=');
         pb.skipWhitespace();         
         anchor = pb.position();
         if(!pb.eof())
         {
            pb.skipToEnd();
            pb.data(value, anchor);
         }
         //cout << "Config file Name='" << name << "' value='" << value << "'" << endl;
         processOption(name.lowercase(), value);
      }
   }
}

bool 
ConfigParser::assignOnOffSetting(const Data& value, bool& setting)
{
   if(value == "1" || value == "true" || value == "on" || value == "enable")
   {
      setting = true;
      return true;
   }
   else if(value == "0" || value == "false" || value == "off" || value == "disable")
   {
      setting = false;
      return true;
   }
   return false;
}

bool 
ConfigParser::processOption(const Data& name, const Data& value)
{   
   bool result = true;
   if(name == "a" || name == "ipaddress")
   {
      mAddress = value;
   }
   else if(name == "d" || name == "dnsservers")
   {
      if(!value.empty()) mDnsServers.clear(); // allow command line to override config file, since list is added to

      // DNS Servers
      ParseBuffer pb(value);
      Data dnsServer;
      while(!value.empty() && !pb.eof())
      {
         pb.skipWhitespace();
         const char *start = pb.position();
         pb.skipToOneOf(ParseBuffer::Whitespace, ";,");  // allow white space 
         pb.data(dnsServer, start);
         if(DnsUtil::isIpV4Address(dnsServer))
         {
            mDnsServers.push_back(Tuple(dnsServer, 0, UNKNOWN_TRANSPORT).toGenericIPAddress());
         }
         else
         {
            cerr << "Tried to add dns server, but invalid format: " << value << endl;
            result = false;
         }
         if(!pb.eof())
         {
            pb.skipChar();
         }
      }   
   }
   else if(name == "gi" || name == "sipgatewayidentity")
   {
      result = assignNameAddr("sip gateway identity", value, mGatewayIdentity);
   }
   else if(name == "sp" || name == "udptcpport")
   {
      mSipPort = (unsigned short)value.convertUnsignedLong();
   }
   else if(name == "tp" || name == "tlsport")
   {
      mTlsPort = (unsigned short)value.convertUnsignedLong();
   }
   else if(name == "td" || name == "tlsdomain")
   {
      mTlsDomain = value;
   }
   else if(name == "keepalives")
   {
      assignOnOffSetting(value, mKeepAlives);
   }
   else if(name == "op" || name == "outboundproxy")
   {
      result = assignNameAddr("outbound proxy", value, mOutboundProxy);
   }
   else if(name == "registrationtime")
   {
      mRegistrationTime = value.convertUnsignedLong();
   }
   else if(name == "registrationretrytime")
   {
      mRegistrationRetryTime = value.convertUnsignedLong();
   }
   else if(name == "l" || name == "loglevel")
   {
      mLogLevel = value;
   }
   else if(name == "logfilename")
   {
      mLogFilename = value;
   }
   else if(name == "logfilemaxlines")
   {
      mLogFileMaxLines = value.convertUnsignedLong();
   }
   else if(name == "gatewayipcport")
   {
      mGatewayIPCPort = (unsigned short)value.convertUnsignedLong();
   }
   else if(name == "jabberconnectoripcport")
   {
      mJabberConnectorIPCPort = (unsigned short)value.convertUnsignedLong();
   }
   else if(name == "httpport")
   {
      mHttpPort = (unsigned short)value.convertUnsignedLong();
   }
   else if(name == "httpauth")
   {
      assignOnOffSetting(value, mHttpAuth);
   }
   else if(name == "httpauthpwd")
   {
      mHttpAuthPwd = value;
   }
   else if(name == "ichatproceedingtimeout")
   {
      mIChatProceedingTimeout = value.convertUnsignedLong();
   }
   else if(name == "alwaysrelayichatmedia")
   {
      assignOnOffSetting(value, mAlwaysRelayIChatMedia);
   }
   else if(name == "preferipv6")
   {
      assignOnOffSetting(value, mPreferIPv6);
   }
   else if(name == "skipfirstichataddress")
   {
      assignOnOffSetting(value, mSkipFirstIChatAddress);
   }   
   else if(name == "codecidfilterlist")
   {
      ParseBuffer pb(value);
      Data codecId;
      while(!value.empty() && !pb.eof())
      {
         pb.skipWhitespace();
         const char *start = pb.position();
         pb.skipToOneOf(ParseBuffer::Whitespace, ";,");  // allow white space 
         pb.data(codecId, start);
         mCodecIdFilterList.insert((unsigned short)codecId.convertUnsignedLong());
         if(!pb.eof())
         {
            pb.skipChar();
         }
      }   
   }
   else if(name == "mediarelayportrangemin")
   {
      mMediaRelayPortRangeMin = (unsigned short)value.convertUnsignedLong();
   }
   else if(name == "mediarelayportrangemax")
   {
      mMediaRelayPortRangeMax = (unsigned short)value.convertUnsignedLong();
   }
   else if(name == "translationpattern")
   {
      mAddressTranslations.push_back(std::make_pair(value, Data::Empty));
   }
   else if(name == "translationoutput")
   {
      if(!mAddressTranslations.empty())
      {
         mAddressTranslations.back().second = value;
      }
   }
   else if(name == "jabberserver")
   {
      mJabberServer = value;
   }
   else if(name == "jabbercomponentname")
   {
      mJabberComponentName = value;
   }
   else if(name == "jabbercomponentpassword")
   {
      mJabberComponentPassword = value;
   }
   else if(name == "jabbercomponentport")
   {
      mJabberComponentPort = (unsigned short)value.convertUnsignedLong();
   }
   else if(name == "jabberserverpingduration")
   {
      mJabberServerPingDuration = value.convertUnsignedLong();
   }
   else if(name == "jabbercontrolusername")
   {
      mJabberControlUsername = value;
   }
   else
   {
      result = false;
   }
   return result;
}

bool 
ConfigParser::assignNameAddr(const Data& settingName, const Data& settingValue, NameAddr& nameAddr)
{
   try
   {
      if(!settingValue.empty())
      {
         NameAddr tempNameAddr(settingValue);
         nameAddr = tempNameAddr;
      }
   }
   catch(resip::BaseException& e)
   {
      // Try adding sip: to address to see if it will be valid
      try
      {
         NameAddr tempNameAddr(Data("sip:" + settingValue));
         nameAddr = tempNameAddr;
      }
      catch(resip::BaseException&)
      {
         cerr << "Invalid " << settingName << " NameAddr format=" << settingValue << ": " << e << endl;
         return false;
      }
   }
   return true;
}

}

/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
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

