#include "ProxyConfig.hxx"

#include <iostream>
#include <fstream>
#include <iterator>

//#include <rutil/DnsUtil.hxx>
#include <resip/stack/NameAddr.hxx>
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace repro;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

namespace repro 
{

ProxyConfig::ProxyConfig(int argc, char** argv, const resip::Data& defaultConfigFilename) : mStore(0)
{
   parseCommandLine(argc, argv);  // will fill in mCmdLineConfigFilename if present
   if(mCmdLineConfigFilename.empty())
   {
      parseConfigFile(defaultConfigFilename);
   }
   else
   {
      parseConfigFile(mCmdLineConfigFilename);
   }
}

ProxyConfig::ProxyConfig() : mStore(0)
{
}

ProxyConfig::~ProxyConfig()
{
   delete mStore; 
   mStore=0;
}

bool 
ProxyConfig::getConfigValue(const resip::Data& name, resip::Data &value)
{
   Data lowerName(name);  lowerName.lowercase();
   ConfigValuesMap::iterator it = mConfigValues.find(lowerName);
   if(it != mConfigValues.end())
   {
      value = it->second;
      return true;
   }
   // Not found
   return false;
}

Data 
ProxyConfig::getConfigData(const resip::Data& name, const resip::Data& defaultValue, bool useDefaultIfEmpty)
{
   Data ret(defaultValue);
   if(getConfigValue(name, ret) && ret.empty() && useDefaultIfEmpty)
   {
      return defaultValue;
   }
   return ret;
}

bool 
ProxyConfig::getConfigValue(const resip::Data& name, bool &value)
{
   Data lowerName(name);  lowerName.lowercase();
   ConfigValuesMap::iterator it = mConfigValues.find(lowerName);
   if(it != mConfigValues.end())
   {
      if(it->second == "1" || 
         isEqualNoCase(it->second, "true") || 
         isEqualNoCase(it->second, "on") || 
         isEqualNoCase(it->second, "enable"))
      {
         value = true;
         return true;
      }
      else if(it->second == "0" ||
              isEqualNoCase(it->second, "false") || 
              isEqualNoCase(it->second, "off") || 
              isEqualNoCase(it->second, "disable"))
      {
         value = false;
         return true;
      }
      cerr << "Invalid boolean setting:  " << name << " = " << it->second << ": Valid values are: 1,true,on,enable,0,false,off or disable" << endl;
      return false;
   }
   // Not found
   return false;
}

bool 
ProxyConfig::getConfigBool(const resip::Data& name, bool defaultValue)
{
   bool ret = defaultValue;
   getConfigValue(name, ret);
   return ret;
}

bool 
ProxyConfig::getConfigValue(const resip::Data& name, unsigned long &value)
{
   Data lowerName(name);  lowerName.lowercase();
   ConfigValuesMap::iterator it = mConfigValues.find(lowerName);
   if(it != mConfigValues.end())
   {
      value = it->second.convertUnsignedLong();
      return true;
   }
   // Not found
   return false;
}

unsigned long 
ProxyConfig::getConfigUnsignedLong(const resip::Data& name, unsigned long defaultValue)
{
   unsigned long ret = defaultValue;
   getConfigValue(name, ret);
   return ret;
}

bool 
ProxyConfig::getConfigValue(const resip::Data& name, int &value)
{
   Data lowerName(name);  lowerName.lowercase();
   ConfigValuesMap::iterator it = mConfigValues.find(lowerName);
   if(it != mConfigValues.end())
   {
      value = it->second.convertInt();
      return true;
   }
   // Not found
   return false;
}


int 
ProxyConfig::getConfigInt(const resip::Data& name, int defaultValue)
{
   int ret = defaultValue;
   getConfigValue(name, ret);
   return ret;
}

bool 
ProxyConfig::getConfigValue(const resip::Data& name, resip::Uri &value)
{
   Data lowerName(name);  lowerName.lowercase();
   ConfigValuesMap::iterator it = mConfigValues.find(lowerName);
   if(it != mConfigValues.end())
   {
      try
      {
         if(!it->second.empty())
         {
            NameAddr tempNameAddr(it->second);
            value = tempNameAddr.uri();
            return true;
         }
         else
         {
            value = Uri();  // return an empty Uri
            return true;
         }
      }
      catch(resip::BaseException& e)
      {
         // Try adding sip: to value to see if it will be valid
         try
         {
            NameAddr tempNameAddr(Data("sip:" + it->second));
            value = tempNameAddr.uri();
            return true;
         }
         catch(resip::BaseException&)
         {
            cerr << "Invalid Uri setting:  " << name << " = " << it->second << ": " << e << endl;
            return false;
         }
      }  
   }
   // Not found
   return false;
}

resip::Uri 
ProxyConfig::getConfigUri(const resip::Data& name, const resip::Uri defaultValue, bool useDefaultIfEmpty)
{
   Uri ret(defaultValue);
   if(getConfigValue(name, ret) && ret.host().empty() && useDefaultIfEmpty)
   {
      return defaultValue;
   }
   return ret;
}

bool 
ProxyConfig::getConfigValue(const resip::Data& name, std::vector<resip::Data> &value)
{
   Data lowerName(name);  lowerName.lowercase();
   std::pair<ConfigValuesMap::iterator,ConfigValuesMap::iterator> valuesIts = mConfigValues.equal_range(lowerName);
   bool found = false;
   for (ConfigValuesMap::iterator it=valuesIts.first; it!=valuesIts.second; ++it)
   {
      found = true;
      ParseBuffer pb(it->second);
      Data item;
      while(!it->second.empty() && !pb.eof())
      {
         pb.skipWhitespace();
         const char *start = pb.position();
         pb.skipToOneOf(ParseBuffer::Whitespace, ",");  // allow white space 
         pb.data(item, start);
         value.push_back(item);
         if(!pb.eof())
         {
            pb.skipChar();
         }
      }
   }

   return found;
}

void 
ProxyConfig::parseCommandLine(int argc, char** argv)
{
   int startingArgForNameValuePairs = 1;
   // First argument is the configuration filename - it is optional and is never proceeded with a - or /
   if(argc >= 2 && argv[1][0] != '-' && argv[1][0] != '/')
   {
      mCmdLineConfigFilename = argv[1];
      startingArgForNameValuePairs = 2;
   }

   // Loop through command line arguments and process them
   for(int i = startingArgForNameValuePairs; i < argc; i++)
   {
      Data argData(argv[i]);

      // Process all commandNames that don't take values
      if(isEqualNoCase(argData, "-?") || 
         isEqualNoCase(argData, "--?") ||
         isEqualNoCase(argData, "--help") ||
         isEqualNoCase(argData, "/?"))
      {
         cout << "Command line format is:" << endl;
         cout << "  " << argv[0] << " [<ConfigFilename>] [--<ConfigValueName>=<ConfigValue>] [--<ConfigValueName>=<ConfigValue>] ..." << endl;
         cout << "Sample Command lines:" << endl;
         cout << "  " << argv[0] << "repro.config --RecordRouteUri=sip:proxy.sipdomain.com --ForceRecordRouting=true" << endl;
         cout << "  " << argv[0] << "repro.config /RecordRouteUri:sip:proxy.sipdomain.com /ForceRecordRouting:true" << endl;
         exit(0);
      }
      else if(argData.at(0) == '-' || argData.at(0) == '/')
      {
         Data name;
         Data value;
         ParseBuffer pb(argData);

         try
         {
            pb.skipChars(Data::toBitset("-/"));  // Skip any leading -'s or /'s
            const char * anchor = pb.position();
            pb.skipToOneOf("=:");
            if(!pb.eof())
            {
               pb.data(name, anchor);
               pb.skipChar();
               anchor = pb.position();
               pb.skipToEnd();
               pb.data(value, anchor);

               cout << "Command line Name='" << name << "' value='" << value << "'" << endl;
               mConfigValues.insert(ConfigValuesMap::value_type(name.lowercase(), value));
            }
            else
            {
               cerr << "Invalid command line parameters:"  << endl;
               cerr << " Name/Value pairs must contain an = or a : between the name and the value" << endl;
               exit(-1);
            }
         }
         catch(BaseException& ex)
         {
            cerr << "Invalid command line parameters:"  << endl;
            cerr << " Exception parsing Name/Value pairs: " << ex << endl;
            exit(-1);
         }
      }
      else
      {
         cerr << "Invalid command line parameters:"  << endl;
         cerr << " Name/Value pairs must be prefixed with either a -, --, or a /" << endl;
         exit(-1);
      }
   }
}

void
ProxyConfig::parseConfigFile(const Data& filename)
{
   cout << "Reading configuraiton from: " << filename << endl;

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
            pb.skipToOneOf("\r\n");
            pb.data(value, anchor);
         }
         //cout << "Config file Name='" << name << "' value='" << value << "'" << endl;
         mConfigValues.insert(ConfigValuesMap::value_type(name.lowercase(), value));
      }
   }
}

void 
ProxyConfig::createDataStore(AbstractDb* db)
{
   assert(db);
   mStore = new Store(*db);
}

}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
