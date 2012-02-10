#if !defined(ConfigParse_hxx)
#define ConfigParse_hxx

#include <map>
#include <vector>
#include <rutil/Data.hxx>

namespace resip
{

class ConfigParse
{
public:
   ConfigParse();
   ConfigParse(int argc, char** argv, const resip::Data& defaultConfigFilename);
   virtual ~ConfigParse();

   virtual void printHelpText(int argc, char **argv) = 0;

   void parseCommandLine(int argc, char** argv);
   void parseConfigFile(const resip::Data& filename);

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

// FIXME: subclass into resip
//   bool getConfigValue(const resip::Data& name, resip::Uri &value);
//   resip::Uri getConfigUri(const resip::Data& name, const resip::Uri defaultValue, bool useDefaultIfEmpty=false);

   bool getConfigValue(const resip::Data& name, std::vector<resip::Data> &value);
   
protected:
   void insertConfigValue(const resip::Data& name, const resip::Data& value);

   typedef std::multimap<resip::Data, resip::Data> ConfigValuesMap;
   ConfigValuesMap mConfigValues;

   // Config filename from command line
   resip::Data mCmdLineConfigFilename;

private:
   friend EncodeStream& operator<<(EncodeStream& strm, const ConfigParse& config);
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
