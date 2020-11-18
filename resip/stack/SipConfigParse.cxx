
#include "resip/stack/NameAddr.hxx"
#ifdef USE_SSL
#include "resip/stack/ssl/Security.hxx"
#endif
#include "resip/stack/SipConfigParse.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

namespace resip
{

SipConfigParse::SipConfigParse()
{
}

SipConfigParse::~SipConfigParse()
{
}

bool
SipConfigParse::getConfigValue(const resip::Data& name, resip::Uri &value)
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
SipConfigParse::getConfigUri(const resip::Data& name, const resip::Uri defaultValue, bool useDefaultIfEmpty)
{
   Uri ret(defaultValue);
   if(getConfigValue(name, ret) && ret.host().empty() && useDefaultIfEmpty)
   {
      return defaultValue;
   }
   return ret;
}

bool
SipConfigParse::getConfigValue(const resip::Data& name, resip::NameAddr &value)
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
SipConfigParse::getConfigNameAddr(const resip::Data& name, const resip::NameAddr defaultValue,  bool useDefaultIfEmpty)
{
   resip::NameAddr ret(defaultValue);
   if(getConfigValue(name, ret) && ret.uri().host().empty() && useDefaultIfEmpty)
   {
      return defaultValue;
   }
   return ret;
}

bool
SipConfigParse::getConfigValue(const resip::Data& name, SecurityTypes::SSLType &value)
{
   Data lowerName(name);  lowerName.lowercase();
   ConfigValuesMap::iterator it = mConfigValues.find(lowerName);
   if(it != mConfigValues.end())
   {
#ifdef USE_SSL
      value = Security::parseSSLType(it->second);
#else
      value = SecurityTypes::NoSSL;
#endif
   }
   return false;
}

SecurityTypes::SSLType
SipConfigParse::getConfigSSLType(const resip::Data& name, SecurityTypes::SSLType defaultValue)
{
   SecurityTypes::SSLType ret = defaultValue;
   getConfigValue(name, ret);
   return ret;
}

bool
SipConfigParse::getConfigValue(const resip::Data& name, SecurityTypes::TlsClientVerificationMode &value)
{
   Data lowerName(name);  lowerName.lowercase();
   ConfigValuesMap::iterator it = mConfigValues.find(lowerName);
   if(it != mConfigValues.end())
   {
      if(isEqualNoCase(it->second, "Optional"))
      {
         value = SecurityTypes::Optional;
      }
      else if(isEqualNoCase(it->second, "Mandatory"))
      {
         value = SecurityTypes::Mandatory;
      }
      else if(isEqualNoCase(it->second, "None"))
      {
         value = SecurityTypes::None;
      }
      else
      {
         Data exceptionString("Unknown TLS client verification mode found in " + name + " setting: " + it->second);
         throw Exception(exceptionString,  __FILE__, __LINE__);
      }
      return true;
   }
   return false;
}

SecurityTypes::TlsClientVerificationMode
SipConfigParse::getConfigClientVerificationMode(const resip::Data& name, SecurityTypes::TlsClientVerificationMode defaultValue)
{
   SecurityTypes::TlsClientVerificationMode ret = defaultValue;
   getConfigValue(name, ret);
   return ret;
}

}

/* ====================================================================
 *
 * Copyright 2016 Daniel Pocock http://danielpocock.com  All rights reserved.
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

