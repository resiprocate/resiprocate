#include <iostream>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <map>

#include "rutil/ConfigParse.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

namespace resip
{

ConfigParse::ConfigParse()
{
}

ConfigParse::~ConfigParse()
{
}

void 
ConfigParse::parseConfig(int argc, char** argv, const resip::Data& defaultConfigFilename, int skipCount)
{
   parseCommandLine(argc, argv, skipCount);  // will fill in mCmdLineConfigFilename if present
   if(mCmdLineConfigFilename.empty())
   {
      parseConfigFile(defaultConfigFilename);
   }
   else
   {
      parseConfigFile(mCmdLineConfigFilename);
   }
}

void 
ConfigParse::parseCommandLine(int argc, char** argv, int skipCount)
{
   int startingArgForNameValuePairs = 1 + skipCount;
   char *firstArg = argv[startingArgForNameValuePairs];
   // First argument is the configuration filename - it is optional and is never proceeded with a - or /
#ifdef WIN32
   if(argc >= (startingArgForNameValuePairs + 1) && firstArg[0] != '-' && firstArg[0] != '/')
#else
   if(argc >= (startingArgForNameValuePairs + 1) && firstArg[0] != '-')
#endif
   {
      mCmdLineConfigFilename = firstArg;
      startingArgForNameValuePairs++;
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
         printHelpText(argc, argv);
         exit(1);
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

               //cout << "Command line Name='" << name << "' value='" << value << "'" << endl;
               insertConfigValue(name, value);
            }
            else
            {
               cerr << "Invalid command line parameters:"  << endl;
               cerr << " Name/Value pairs must contain an = or a : between the name and the value" << endl;
               exit(-1);  // todo - should convert this stuff to exceptions and let user decide to exit or not
            }
         }
         catch(BaseException& ex)
         {
            cerr << "Invalid command line parameters:"  << endl;
            cerr << " Exception parsing Name/Value pairs: " << ex << endl;
            exit(-1); // todo - should convert this stuff to exceptions and let user decide to exit or not
         }
      }
      else
      {
         cerr << "Invalid command line parameters:"  << endl;
         cerr << " Name/Value pairs must be prefixed with either a -, --, or a /" << endl;
         exit(-1); // todo - should convert this stuff to exceptions and let user decide to exit or not
      }
   }
}

void
ConfigParse::parseConfigFile(const Data& filename)
{
   ifstream configFile(filename.c_str());
   
   if(!configFile)
   {
      throw Exception("Error opening/reading configuration file", __FILE__, __LINE__);
   }

   string sline;
   const char * anchor;
   while(getline(configFile, sline)) 
   {
      Data line(sline);
      Data name;
      Data value;
      ParseBuffer pb(line);

      pb.skipWhitespace();
      anchor = pb.position();
      if(pb.eof() || *anchor == '#') continue;  // if line is a comment or blank then skip it

      // Look for end of name
      pb.skipToOneOf("= \t");
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
      insertConfigValue(name, value);
   }
}

bool 
ConfigParse::getConfigValue(const resip::Data& name, resip::Data &value)
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
ConfigParse::getConfigData(const resip::Data& name, const resip::Data& defaultValue, bool useDefaultIfEmpty)
{
   Data ret(defaultValue);
   if(getConfigValue(name, ret) && ret.empty() && useDefaultIfEmpty)
   {
      return defaultValue;
   }
   return ret;
}

bool 
ConfigParse::getConfigValue(const resip::Data& name, bool &value)
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
ConfigParse::getConfigBool(const resip::Data& name, bool defaultValue)
{
   bool ret = defaultValue;
   getConfigValue(name, ret);
   return ret;
}

bool 
ConfigParse::getConfigValue(const resip::Data& name, unsigned long &value)
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
ConfigParse::getConfigUnsignedLong(const resip::Data& name, unsigned long defaultValue)
{
   unsigned long ret = defaultValue;
   getConfigValue(name, ret);
   return ret;
}

bool 
ConfigParse::getConfigValue(const resip::Data& name, int &value)
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
ConfigParse::getConfigInt(const resip::Data& name, int defaultValue)
{
   int ret = defaultValue;
   getConfigValue(name, ret);
   return ret;
}

bool
ConfigParse::getConfigValue(const resip::Data& name, unsigned short &value)
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


unsigned short
ConfigParse::getConfigUnsignedShort(const resip::Data& name, int defaultValue)
{
   int ret = defaultValue;
   getConfigValue(name, ret);
   return ret;
}

bool 
ConfigParse::getConfigValue(const resip::Data& name, std::vector<resip::Data> &value)
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
ConfigParse::insertConfigValue(const resip::Data& name, const resip::Data& value)
{
   resip::Data lowerName(name);
   lowerName.lowercase();
   mConfigValues.insert(ConfigValuesMap::value_type(lowerName, value));
}

resip::Data
ConfigParse::removePath(const resip::Data& fileAndPath)
{
   Data filenameOnly;
   ParseBuffer pb(fileAndPath);
   const char* anchor = pb.position();
   while(pb.skipToOneOf("/\\") && !pb.eof())
   {
      pb.skipChar();
      anchor = pb.position();
   }
   pb.data(filenameOnly, anchor);
   return filenameOnly;
}

EncodeStream& 
operator<<(EncodeStream& strm, const ConfigParse& config)
{
   // Yes this is horribly inefficient - however it's only used when a user requests it
   // and we want to see the items in a sorted list and hash_maps are not sorted.
   std::multimap<Data, Data> sortedMap;
   ConfigParse::ConfigValuesMap::const_iterator it = config.mConfigValues.begin();
   for(; it != config.mConfigValues.end(); it++)
   {
      sortedMap.insert(std::multimap<Data, Data>::value_type(it->first, it->second));
   }
   std::multimap<Data, Data>::iterator it2 = sortedMap.begin();
   for(; it2 != sortedMap.end(); it2++)
   {
      strm << it2->first << " = " << it2->second << endl;
   }
   return strm;
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
