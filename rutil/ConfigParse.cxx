#include <iostream>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <sstream>
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
ConfigParse::parseConfig(int argc, char** argv, const resip::Data& defaultConfigFilename)
{
   ConfigParse::parseConfig(argc, argv, defaultConfigFilename, 0);
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
   mConfigValues = mFileConfigValues;
   // Overlay the command line config options on top of the config options from the file
   // The command line config options take precedence / override anything in the file
   for(
      ConfigValuesMap::iterator it = mCmdLineConfigValues.begin();
      it != mCmdLineConfigValues.end();
      it++)
   {
      if(mConfigValues.find(it->first) != mConfigValues.end())
      {
         mConfigValues.erase(it->first);
      }
      mConfigValues.insert(ConfigValuesMap::value_type(it->first, it->second));
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
         throw Exception("Help text requested - process stopping", __FILE__, __LINE__);
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
               insertConfigValue("command line", mCmdLineConfigValues, name, value);
            }
            else
            {
               cerr << "Invalid command line parameters:"  << endl;
               cerr << " Name/Value pairs must contain an = or a : between the name and the value" << endl;
               cerr << " Bad argument: " << argData << endl;
               Data exceptionString("Name/Value pairs must contain an = or a : between the name and the value (Bad argument: " + argData + ")");
               throw Exception(exceptionString, __FILE__, __LINE__);
            }
         }
         catch(BaseException& ex)
         {
            cerr << "Invalid command line parameters:"  << endl;
            cerr << " Exception parsing Name/Value pairs: " << ex << endl;
            cerr << " Bad argument: " << argData << endl;
            throw;
         }
      }
      else
      {
         cerr << "Invalid command line parameters:"  << endl;
         cerr << " Name/Value pairs must be prefixed with either a -, --, or a /" << endl;
         cerr << " Bad argument: " << argData << endl;
         Data exceptionString("Name/Value pairs must be prefixed with either a -, --, or a / (Bad argument: " + argData + ")");
         throw Exception(exceptionString,  __FILE__, __LINE__);
      }
   }
}

void
ConfigParse::parseConfigFile(const Data& filename)
{
   // Store off base config path
   ParseBuffer pb(filename);
   const char* anchor = pb.start();
   pb.skipToEnd();
   pb.skipBackToOneOf("/\\");
   if(!pb.bof())
   {
      mConfigBasePath = pb.data(pb.start());
   }

   ifstream configFile(filename.c_str());
   
   if(!configFile)
   {
      Data exceptionString("Error opening/reading configuration file: " + filename);
      throw Exception(exceptionString,  __FILE__, __LINE__);
   }

   string sline;
   while(getline(configFile, sline)) 
   {
      Data name;
      Data value;
      ParseBuffer pb(sline.c_str(), sline.size());

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
      Data lowerName(name);
      lowerName.lowercase();
      if(lowerName == "include")
      {
         parseConfigFile(value);
      }
      else
      {
         insertConfigValue("config file", mFileConfigValues, name, value);
      }
   }
}

void
ConfigParse::getConfigIndexKeys(const resip::Data& indexName, std::set<Data>& keys)
{
   Data::size_type numPos = indexName.size();
   Data indexNameLower(indexName);
   indexNameLower.lowercase();
   ConfigValuesMap::iterator it = mConfigValues.begin();
   for(; it != mConfigValues.end(); it++)
   {
      const Data& keyName = it->first;
      if(keyName.prefix(indexNameLower) && keyName.size() > numPos
         && isdigit(keyName[numPos]))
      {
         Data::size_type i = numPos + 1;
         while(i < keyName.size() && isdigit(keyName[i]))
         {
            i++;
         }
         Data indexFullName = keyName.substr(0, i);
         if(keys.find(indexFullName) == keys.end())
         {
            keys.insert(indexFullName);
         }
      }
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

bool
ConfigParse::getConfigValue(const resip::Data& name, std::set<resip::Data> &value)
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
         value.insert(item);
         if(!pb.eof())
         {
            pb.skipChar();
         }
      }
   }

   return found;
}

ConfigParse::NestedConfigMap
ConfigParse::getConfigNested(const resip::Data& mapsPrefix)
{
   NestedConfigMap m;
   Data::size_type numPos = mapsPrefix.size();
   Data mapsPrefixLower(mapsPrefix);
   mapsPrefixLower.lowercase();
   ConfigValuesMap::iterator it = mConfigValues.begin();
   for(; it != mConfigValues.end(); it++)
   {
      const Data& keyName = it->first;
      if(keyName.prefix(mapsPrefixLower) && keyName.size() > numPos
         && isdigit(keyName[numPos]))
      {
         Data::size_type i = numPos + 1;
         while(i < keyName.size() && isdigit(keyName[i]))
         {
            i++;
         }
         if(keyName.size() - i < 1)
         {
            stringstream err_text;
            err_text << "Configuration key " << keyName << " missing subkey name";
            Data err_data(err_text.str());
            throw Exception(err_data, __FILE__, __LINE__);
         }
         Data index = keyName.substr(numPos, i - numPos);
         Data nestedKey = keyName.substr(i, keyName.size() - i);
         NestedConfigParse& nested = m[index.convertInt()];
         nested.insertConfigValue(nestedKey, it->second);
      }
   }
   return m;
}

void 
ConfigParse::insertConfigValue(const Data& source, ConfigValuesMap& configValues, const resip::Data& name, const resip::Data& value)
{
   resip::Data lowerName(name);
   lowerName.lowercase();
   if(configValues.find(lowerName) != configValues.end())
   {
      stringstream err_text;
      err_text << "Duplicate configuration key " << name << " while parsing " << source;
      Data err_data(err_text.str());
      throw Exception(err_data, __FILE__, __LINE__);
   }
   configValues.insert(ConfigValuesMap::value_type(lowerName, value));
}

void 
ConfigParse::insertConfigValue(const resip::Data& name, const resip::Data& value)
{
    insertConfigValue("manually added setting", mConfigValues, name, value);
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

bool 
ConfigParse::AddBasePathIfRequired(Data& filename)
{
   if(!filename.empty())
   {
      // If filename already has a path specified, then don't touch it
      ParseBuffer pb(filename);
      pb.skipToOneOf("/\\");
      if(pb.eof())
      {
         // No slashes in filename, so no path present
         filename = mConfigBasePath + filename;
         return true;
      }
   }
   return false;
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
