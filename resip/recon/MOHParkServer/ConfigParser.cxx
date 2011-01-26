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
   mMOHUri("sip:moh@server.com"),
   mMOHRegistrationTime(3600),
   mMOHFilenameUrl("file:music.wav;repeat"),
   mParkUri("sip:park@server.com"),
   mParkRegistrationTime(3600),
   mParkMOHFilenameUrl("file:music.wav;repeat"),
   mParkOrbitRangeStart(6000),
   mParkNumOrbits(10),
   mParkOrbitRegistrationTime(3600),
   mMaxParkTime(600),  // 600 seconds = 10 mins
   mUdpPort(0),
   mTcpPort(0),
   mTlsPort(0),
   mTlsDomain(DnsUtil::getLocalHostName()),
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
   mMOHUri("sip:moh@server.com"),
   mMOHRegistrationTime(3600),
   mMOHFilenameUrl("file:music.wav;repeat"),
   mParkUri("sip:park@server.com"),
   mParkRegistrationTime(3600),
   mParkMOHFilenameUrl("file:music.wav;repeat"),
   mParkOrbitRangeStart(6000),
   mParkNumOrbits(10),
   mParkOrbitRegistrationTime(3600),
   mMaxParkTime(600),  // 600 seconds = 10 mins
   mUdpPort(0),
   mTcpPort(0),
   mTlsPort(0),
   mTlsDomain(DnsUtil::getLocalHostName()),
   mKeepAlives(true),
   mMediaPortRangeStart(50000),
   mMediaPortRangeSize(100),
   mHttpPort(5082),
   mLogLevel("INFO"),
   mLogFilename("mohparkserver.log"),
   mLogFileMaxBytes(5000000)     // about 5Mb size
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
   if(name == "mohuri")
   {
      result = assignNameAddr("MOH Uri", value, mMOHUri);
   }
   else if(name == "mohpassword")
   {
      mMOHPassword = value;
   }
   else if(name == "mohregistrationtime")
   {
      mMOHRegistrationTime = value.convertUnsignedLong();
   }
   else if(name == "mohfilename")
   {
      Uri url("file:music.wav;repeat");
      Data urlData;
      if(value.find("http://") != Data::npos || 
         value.find("file:") != Data::npos)
      {
         // URL was specified - add repeat parameter
         urlData = value + ";repeat";         
      }
      else
      {
         urlData = "file:" + value + ";repeat";
      }
      try
      {
         Uri temp(urlData);
         url = temp;
      }
      catch(BaseException& e)
      {
         cerr << "Invalid MOHFilename format=" << value << ": " << e << endl;
         cerr << "Using " << url << " instead." << endl;
      }
      mMOHFilenameUrl = url;
   }
   else if(name == "parkuri")
   {
      result = assignNameAddr("Park Uri", value, mParkUri);
   }
   else if(name == "parkpassword")
   {
      mParkPassword = value;
   }
   else if(name == "parkregistrationtime")
   {
      mParkRegistrationTime = value.convertUnsignedLong();
   }
   else if(name == "parkmohfilename")
   {
      Uri url("file:music.wav;repeat");
      Data urlData;
      if(value.find("http://") != Data::npos || 
         value.find("file:") != Data::npos)
      {
         // URL was specified - add repeat parameter
         urlData = value + ";repeat";         
      }
      else
      {
         urlData = "file:" + value + ";repeat";
      }
      try
      {
         Uri temp(urlData);
         url = temp;
      }
      catch(BaseException& e)
      {
         cerr << "Invalid Park MOHFilename format=" << value << ": " << e << endl;
         cerr << "Using " << url << " instead." << endl;
      }
      mParkMOHFilenameUrl = url;
   }
   else if(name == "parkorbitrangestart")
   {
      mParkOrbitRangeStart = value.convertUnsignedLong();
   }
   else if(name == "parknumorbits")
   {
      mParkNumOrbits = value.convertUnsignedLong();
   }
   else if(name == "parkorbitregistrationtime")
   {
      mParkOrbitRegistrationTime = value.convertUnsignedLong();
   }
   else if(name == "parkorbitpassword")
   {
      mParkOrbitPassword = value;
   }
   else if(name == "maxparktime")
   {
      mMaxParkTime = value.convertUnsignedLong();
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
      mUdpPort = (unsigned short)value.convertUnsignedLong();
   }
   else if(name == "tp" || name == "tcpport")
   {
      mTcpPort = (unsigned short)value.convertUnsignedLong();
   }
   else if(name == "sp" || name == "tlsport")
   {
      mTlsPort = (unsigned short)value.convertUnsignedLong();
   }
   else if(name == "td" || name == "tlsdomain")
   {
      mTlsDomain = value;
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
      result = assignNameAddr("outbound proxy", value, mOutboundProxy);
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

 Copyright (c) 2010, SIP Spectrum, Inc.
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

