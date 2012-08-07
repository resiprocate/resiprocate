
#include <iostream>
#include <stdexcept>
#include <string>

#include "rutil/Data.hxx"

#include "DialerConfiguration.hxx"


using namespace resip;
using namespace std;

DialerConfiguration::DialerConfiguration() :
   mDialerIdentity("sip:anonymous@localhost"),
   mAuthRealm(""),
   mAuthUser(""),
   mAuthPassword(""),
   mCallerUserAgentAddress("sip:anonymous@localhost"),
   mCallerUserAgentVariety(Generic),
   mTargetPrefix(""),
   mTargetDomain("localhost"),
   mCertPath(""),
   mCADirectory("")
{
}

void DialerConfiguration::parseConfig(int argc, char** argv, const resip::Data& defaultConfigFilename, int skipCount) 
{
   resip::ConfigParse::parseConfig(argc, argv, defaultConfigFilename, skipCount);

   NameAddr _dialerIdentity(getConfigData("dialerIdentity", "sip:anonymous@localhost"));
   mDialerIdentity = _dialerIdentity;
   mAuthRealm = getConfigData("authRealm", "");
   mAuthUser = getConfigData("authUser", "");
   mAuthPassword = getConfigData("authPassword", "");
   Uri _callerUserAgentAddress(getConfigData("callerUserAgentAddress", "sip:anonymous@localhost"));
   mCallerUserAgentAddress = _callerUserAgentAddress;
   mCallerUserAgentVariety = Generic;
   mTargetPrefix = getConfigData("targetPrefix", "");
   mTargetDomain = getConfigData("targetDomain", "localhost");
   mCertPath = getConfigData("certPath", "");
   mCADirectory = getConfigData("CADirectory", "");

   Data value(getConfigData("callerUserAgentVariety", "Generic"));
   if(value == "LinksysSPA941")
      setCallerUserAgentVariety(LinksysSPA941);
   else if(value == "AlertInfo")
      setCallerUserAgentVariety(AlertInfo);
   else if(value == "Cisco7940")
      setCallerUserAgentVariety(Cisco7940);
   else if(value == "Generic")
      setCallerUserAgentVariety(Generic);
   else
      throw std::runtime_error("Unexpected value for config setting callerUserAgentVariety");
}

DialerConfiguration::~DialerConfiguration()
{
}

void
DialerConfiguration::printHelpText(int argc, char **argv)
{
   std::cerr << "Command line format is:" << std::endl;
   std::cerr << "  " << argv[0] << " <targetUri> [<ConfigFilename>] [--<ConfigValueName>=<ConfigValue>] [--<ConfigValueName>=<ConfigValue>] ..." << std::endl;
   std::cerr << "Sample Command line(s):" << std::endl;
   std::cerr << "  " << argv[0] << " user@example.org" << std::endl;
}

/* ====================================================================
 *
 * Copyright 2012 Daniel Pocock.  All rights reserved.
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

