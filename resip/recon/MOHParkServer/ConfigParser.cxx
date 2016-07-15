#include "ConfigParser.hxx"

#include "AppSubsystem.hxx"

#include <iostream>
#include <fstream>
#include <iterator>

#include <rutil/DnsUtil.hxx>
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/WinLeakCheck.hxx>
#include <resip/stack/Tuple.hxx>

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::MOHPARKSERVER

namespace mohparkserver 
{

ConfigParser::ConfigParser(int argc, char** argv) : 
   // Defaults
   mUdpPort(0),
   mTcpPort(0),
   mTlsPort(0),
   mTlsDomain(DnsUtil::getLocalHostName()),
   mCertificatePath("./certs"),
   mKeepAlives(true),
   mMediaPortRangeStart(50000),
   mMediaPortRangeSize(100),
   mHttpPort(5082),
   mLogLevel("INFO"),
   mLogFilename("mohparkserver.log"),
   mLogFileMaxBytes(5000000),     // about 5Mb size
   mSocketFunc(0)
{
   //mAddress = DnsUtil::getLocalIpAddress();

   // Parse config file first
   parseConfigFile("mohparkserver.config");

   // Parse command line options
   // Note:  command line overrides config file setting
   parseCommandLine(argc, argv);
}

ConfigParser::ConfigParser() : 
   // Defaults
   mUdpPort(0),
   mTcpPort(0),
   mTlsPort(0),
   mTlsDomain(DnsUtil::getLocalHostName()),
   mCertificatePath("./certs"),
   mKeepAlives(true),
   mMediaPortRangeStart(50000),
   mMediaPortRangeSize(100),
   mHttpPort(5082),
   mLogLevel("INFO"),
   mLogFilename("mohparkserver.log"),
   mLogFileMaxBytes(5000000),     // about 5Mb size
   mSocketFunc(0)
{
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
         cout << " -up <port num> - local port number to use for UDP SIP messaging" << endl;
         cout << " -tp <port num> - local port number to use for TCP SIP messaging" << endl;
         cout << " -sp <port num> - local port number to use for TLS SIP messaging" << endl;
         cout << " -td <domain name> - domain name to use for TLS server connections" << endl;
         cout << " -cp <certificate path> - path to load TLS certificates from" << endl;
         cout << " -nk - no keepalives, set this to disable sending of keepalives" << endl;
         cout << " -op <SIP URI> - URI of a proxy server to use a SIP outbound proxy" << endl;
         cout << " -l <NONE|ERR|WARNING|INFO|DEBUG|STACK> - logging level" << endl;
         cout << endl;
         cout << "Sample Command line:" << endl;
         cout << "MOHParkServer -a 192.168.1.100 -l DEBUG" << endl;
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
ConfigParser::processOption(const Data& name, const Data& value)
{   
   bool result = true;
   if(name.prefix("moh") && name.size() > 4)
   {
      unsigned int settingIndex = 0;
      ParseBuffer pb(name);
      pb.skipN(3);  // skip past "moh"
      const char* anchor = pb.position();
      while (!pb.eof() && *pb.position() >= '0' && *pb.position() <= '9')
      {
         pb.skipN(1);
      }
      if (anchor != pb.position()) 
      {
         settingIndex = pb.data(anchor).convertUnsignedLong();
      } // else - There were no numbers following "moh" - theses are legacy settings - use a 0 index

      anchor = pb.position();
      pb.skipToEnd();
      Data subToken = pb.data(anchor);
      if (subToken == "uri")
      {
         result = assignNameAddr(name, value, mMOHSettingsMap[settingIndex].mUri);
      }
      else if (subToken == "password")
      {
         mMOHSettingsMap[settingIndex].mPassword = value;
      }
      else if (subToken == "registrationtime")
      {
         mMOHSettingsMap[settingIndex].mRegistrationTime = value.convertUnsignedLong();
      }
      else if (subToken == "outboundproxy")
      {
         result = assignNameAddr(name, value, mMOHSettingsMap[settingIndex].mOutboundProxy);
      }
      else if (subToken == "filename")
      {
         result = assignMusicUrl(name, value, mMOHSettingsMap[settingIndex].mMOHFilenameUrl);
      }
   }
   else if (name.prefix("park") && name.size() > 5)
   {
      unsigned int settingIndex = 0;
      ParseBuffer pb(name);
      pb.skipN(4);  // skip past "park"
      const char* anchor = pb.position();
      while (!pb.eof() && *pb.position() >= '0' && *pb.position() <= '9')
      {
         pb.skipN(1);
      }
      if (anchor != pb.position())
      {
         settingIndex = pb.data(anchor).convertUnsignedLong();
      } // else - There were no numbers following "moh" - theses are legacy settings - use a 0 index

      anchor = pb.position();
      pb.skipToEnd();
      Data subToken = pb.data(anchor);
      if (subToken == "uri")
      {
         result = assignNameAddr(name, value, mParkSettingsMap[settingIndex].mUri);
      }
      else if (subToken == "password")
      {
         mParkSettingsMap[settingIndex].mPassword = value;
      }
      else if (subToken == "registrationtime")
      {
         mParkSettingsMap[settingIndex].mRegistrationTime = value.convertUnsignedLong();
      }
      else if (subToken == "outboundproxy")
      {
         result = assignNameAddr(name, value, mParkSettingsMap[settingIndex].mOutboundProxy);
      }
      else if (subToken == "mohfilename")
      {
         result = assignMusicUrl(name, value, mParkSettingsMap[settingIndex].mMOHFilenameUrl);
      }
      else if (subToken == "orbitrangestart")
      {
         mParkSettingsMap[settingIndex].mOrbitRangeStart = value.convertUnsignedLong();
      }
      else if (subToken == "numorbits")
      {
         mParkSettingsMap[settingIndex].mNumOrbits = value.convertUnsignedLong();
      }
      else if (subToken == "orbitregistrationtime")
      {
         mParkSettingsMap[settingIndex].mOrbitRegistrationTime = value.convertUnsignedLong();
      }
      else if (subToken == "orbitpassword")
      {
         mParkSettingsMap[settingIndex].mOrbitPassword = value;
      }
      else if (subToken == "maxparktime")
      {
         mParkSettingsMap[settingIndex].mMaxParkTime = value.convertUnsignedLong();
      }
   }
   else if(name == "maxparktime")
   {
      // This is a legacy setting and was renamed to be ParkXMaxTime in newer version.  We still read old setting 
      // for back compat purposes.
      mParkSettingsMap[0].mMaxParkTime = value.convertUnsignedLong();
   }
   else if(name == "a" || name == "ipaddress")
   {
      if(!value.empty())
      {
         mAddress = value;
      }
   }
   else if(name == "d" || name == "dnsservers")
   {
      mDnsServers = value;
   }
   else if(name == "up" || name == "udpport")
   {
      mUdpPort = (unsigned short)value.convertInt();
   }
   else if(name == "tp" || name == "tcpport")
   {
      mTcpPort = (unsigned short)value.convertInt();
   }
   else if(name == "sp" || name == "tlsport")
   {
      mTlsPort = (unsigned short)value.convertInt();
   }
   else if(name == "td" || name == "tlsdomain")
   {
      mTlsDomain = value;
   }
   else if (name == "cp" || name == "certificatepath")
   {
       mCertificatePath = value;
   }
   else if(name == "keepalives")
   {
      if(value == "1" || value == "true" || value == "on" || value == "enable")
      {
         mKeepAlives = true;
      }
      else if(value == "0" || value == "false" || value == "off" || value == "disable")
      {
         mKeepAlives = false;
      }
   }
   else if(name == "op" || name == "outboundproxy")
   {
      result = assignNameAddr("OutboundProxy", value, mOutboundProxy);
   }
   else if(name == "mediaportrangestart")
   {
      mMediaPortRangeStart = (unsigned short)value.convertUnsignedLong();
   }
   else if(name == "mediaportrangesize")
   {
      mMediaPortRangeSize = (unsigned short)value.convertUnsignedLong();
   }
   else if(name == "httpport")
   {
      mHttpPort = (unsigned short)value.convertUnsignedLong();
   }
   else if(name == "sipxlogfilename")
   {
      mSipXLogFilename = value;
   }
   else if(name == "l" || name == "loglevel")
   {
      mLogLevel = value;
   }
   else if(name == "logfilename")
   {
      mLogFilename = value;
   }
   else if(name == "logfilemaxbytes")
   {
      mLogFileMaxBytes = value.convertUnsignedLong();
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
   if (!settingValue.empty() && settingValue.find("sip:") == Data::npos && settingValue.find("sips:") == Data::npos)
   {
      try
      {
         NameAddr tempNameAddr(Data("sip:" + settingValue));
         nameAddr = tempNameAddr;
      }
      catch (resip::BaseException& e)
      {
         cerr << "Invalid " << settingName << " NameAddr format=" << settingValue << ": " << e << endl;
         return false;
      }
   }
   else if(!settingValue.empty())
   {
      try
      {
         NameAddr tempNameAddr(Data("sip:" + settingValue));
         nameAddr = tempNameAddr;
      }
      catch (resip::BaseException& e)
      {
         cerr << "Invalid " << settingName << " NameAddr format=" << settingValue << ": " << e << endl;
         return false;
      }
   }
   return true;
}

bool ConfigParser::assignMusicUrl(const resip::Data& settingName, const resip::Data& settingValue, resip::Uri& url)
{
   Data urlData;
   if (settingValue.find("http://") != Data::npos ||
       settingValue.find("file:") != Data::npos)
   {
      // URL was specified - add repeat parameter
      urlData = settingValue + ";repeat";
   }
   else
   {
      urlData = "file:" + settingValue + ";repeat";
   }
   try
   {
      Uri temp(urlData);
      url = temp;
   }
   catch (BaseException& e)
   {
      cerr << "Invalid " << settingName << " Uri format=" << settingValue << ": " << e << endl;
      cerr << "Using " << url << " instead." << endl;
      url = Uri("file:music.wav;repeat");
   }

   return true;
}

}

/* ====================================================================

 Copyright (c) 2010-2016, SIP Spectrum, Inc.
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

