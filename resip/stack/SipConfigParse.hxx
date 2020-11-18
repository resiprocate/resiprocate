#if !defined(SipConfigParse_hxx)
#define SipConfigParse_hxx


#include "rutil/ConfigParse.hxx"

#include "resip/stack/Uri.hxx"
#include "resip/stack/SecurityTypes.hxx"

namespace resip
{

class SipConfigParse : public ConfigParse
{

public:
   SipConfigParse();
   SipConfigParse(const ConfigParse& c) : ConfigParse(c) {};
   virtual ~SipConfigParse();

   using resip::ConfigParse::getConfigValue;
   bool getConfigValue(const resip::Data& name, resip::Uri &value);
   resip::Uri getConfigUri(const resip::Data& name, const resip::Uri defaultValue, bool useDefaultIfEmpty=false);

   bool getConfigValue(const resip::Data& name, resip::NameAddr &value);
   resip::NameAddr getConfigNameAddr(const resip::Data& name, const resip::NameAddr defaultValue, bool useDefaultIfEmpty=false);

   bool getConfigValue(const resip::Data& name, SecurityTypes::SSLType &value);
   SecurityTypes::SSLType getConfigSSLType(const resip::Data& name, SecurityTypes::SSLType defaultValue);

   bool getConfigValue(const resip::Data& name, SecurityTypes::TlsClientVerificationMode &value);
   SecurityTypes::TlsClientVerificationMode getConfigClientVerificationMode(const resip::Data& name, SecurityTypes::TlsClientVerificationMode defaultValue);

   // Must define this so instances of SipConfigParse can be instantiated
   virtual void printHelpText(int argc, char **argv) {};
};

}

#endif

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

