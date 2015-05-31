#if !defined(ConfigParse_hxx)
#define ConfigParse_hxx

#include <set>
#include <vector>
#include "rutil/BaseException.hxx"
#include "rutil/HashMap.hxx"
#include "rutil/Data.hxx"

namespace resip
{

class ConfigParse
{
private:
   class NestedConfigParse;
public:
   class Exception : public BaseException
   {
      public:
         Exception(const Data& msg,
                   const Data& file,
                   const int line)
            : BaseException(msg, file, line) {}            
      protected:
         virtual const char* name() const { return "ConfigParse::Exception"; }
   };

   ConfigParse();
   virtual ~ConfigParse();

   // parses both command line and config file - using config file name from first command line parameter (if present)
   // skipcount represents the number of command line options to skip before looking for config filename or name value pairs
   virtual void parseConfig(int argc, char** argv, const resip::Data& defaultConfigFilename);
   virtual void parseConfig(int argc, char** argv, const resip::Data& defaultConfigFilename, int skipCount); 
   virtual void parseCommandLine(int argc, char** argv, int skipCount = 0);
   virtual void parseConfigFile(const resip::Data& filename);

   virtual void printHelpText(int argc, char **argv) = 0;

   void getConfigIndexKeys(const resip::Data& indexName, std::set<resip::Data>& keys);

   bool getConfigValue(const resip::Data& name, resip::Data &value);
   resip::Data getConfigData(const resip::Data& name, const resip::Data& defaultValue, bool useDefaultIfEmpty=false);

   bool getConfigValue(const resip::Data& name, bool &value);
   bool getConfigBool(const resip::Data& name, bool defaultValue);
   
   bool getConfigValue(const resip::Data& name, unsigned long &value);
   unsigned long getConfigUnsignedLong(const resip::Data& name, unsigned long defaultValue);

   bool getConfigValue(const resip::Data& name, int &value);
   int getConfigInt(const resip::Data& name, int defaultValue);

   bool getConfigValue(const resip::Data& name, unsigned short &value);
   unsigned short getConfigUnsignedShort(const resip::Data& name, int defaultValue);

   bool getConfigValue(const resip::Data& name, std::vector<resip::Data> &value);
   bool getConfigValue(const resip::Data& name, std::set<resip::Data> &value);

   typedef HashMap<int, NestedConfigParse> NestedConfigMap;
   NestedConfigMap getConfigNested(const resip::Data& mapsPrefix);

   bool AddBasePathIfRequired(Data& filename);

   void insertConfigValue(const resip::Data& name, const resip::Data& value);

protected:
   typedef HashMultiMap<resip::Data, resip::Data> ConfigValuesMap;

   void insertConfigValue(const Data& source, ConfigValuesMap& configValues, const resip::Data& name, const resip::Data& value);

   ConfigValuesMap mCmdLineConfigValues;
   ConfigValuesMap mFileConfigValues;
   ConfigValuesMap mConfigValues;

   resip::Data removePath(const resip::Data& fileAndPath);

   // Config filename from command line
   resip::Data mCmdLineConfigFilename;

   resip::Data mConfigBasePath;

private:
   friend EncodeStream& operator<<(EncodeStream& strm, const ConfigParse& config);
};

class ConfigParse::NestedConfigParse : public ConfigParse
{
public:
   NestedConfigParse() {};
   virtual void printHelpText(int argc, char **argv) {};
};

EncodeStream& operator<<(EncodeStream& strm, const ConfigParse& config);
 
}

#endif

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
