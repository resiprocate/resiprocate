
#ifndef __DIALERCONFIGURATION_H
#define __DIALERCONFIGURATION_H

#include <iostream>
#include <map>

#include <rutil/ConfigParse.hxx>

#include "resip/stack/NameAddr.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Data.hxx"

class DialerConfiguration : public resip::ConfigParse {

public:
   DialerConfiguration();
   virtual ~DialerConfiguration();

   virtual void parseConfig(int argc, char** argv, const resip::Data& defaultConfigFilename, int skipCount = 0);

   void printHelpText(int argc, char **argv);
   using resip::ConfigParse::getConfigValue;

   void setDialerIdentity(const resip::NameAddr& dialerIdentity)
      { mDialerIdentity = dialerIdentity; };
   const resip::NameAddr& getDialerIdentity() 
      { return mDialerIdentity; };
   void setAuthRealm(const resip::Data& authRealm)
      { mAuthRealm = authRealm; };
   const resip::Data& getAuthRealm()
      { return mAuthRealm; };
   void setAuthUser(const resip::Data& authUser) 
      { mAuthUser = authUser; };
   const resip::Data& getAuthUser() 
      { return mAuthUser; };
   void setAuthPassword(const resip::Data& authPassword)
      { mAuthPassword = authPassword; };
   const resip::Data& getAuthPassword()
      { return mAuthPassword; };
   void setCallerUserAgentAddress(const resip::Uri& callerUserAgentAddress)
      { mCallerUserAgentAddress = callerUserAgentAddress; };
   const resip::Uri& getCallerUserAgentAddress()
      { return mCallerUserAgentAddress; };
   typedef enum
   {
      Generic,
      LinksysSPA941,
      AlertInfo,
      Cisco7940
   } UserAgentVariety;
   void setCallerUserAgentVariety(UserAgentVariety callerUserAgentVariety)
      { mCallerUserAgentVariety = callerUserAgentVariety; };
   const UserAgentVariety getCallerUserAgentVariety()
      { return mCallerUserAgentVariety; };
   void setTargetPrefix(const resip::Data& targetPrefix)
      { mTargetPrefix = targetPrefix; };
   const resip::Data& getTargetPrefix() 
      { return mTargetPrefix; };
   void setTargetDomain(const resip::Data& targetDomain)
      { mTargetDomain = targetDomain; };
   const resip::Data& getTargetDomain() 
      { return mTargetDomain; };
   void setCertPath(const resip::Data& certPath)
      { mCertPath = certPath; };
   const resip::Data& getCertPath()
      { return mCertPath; };
   void setCADirectory(const resip::Data& caDirectory)
      { mCADirectory = caDirectory; };
   const resip::Data& getCADirectory()
      { return mCADirectory; };

protected:

   // Used for the `From' field of the INVITE
   resip::NameAddr mDialerIdentity;
   // Credentials we must send if challenged in the given realm
   // Todo: allow a hashmap of credentials for multiple realms
   resip::Data mAuthRealm;
   resip::Data mAuthUser;
   resip::Data mAuthPassword;
   // The SIP URI of the device which is being forced to make a call
   resip::Uri mCallerUserAgentAddress;
   // the type of UA which we are sending a REFER to, depending on the
   // type of device, we may need to force auto-answer with a specific
   // value of the Alert-Info header, put specific SDP in the INVITE,
   // and construct the REFER in a particular way
   UserAgentVariety mCallerUserAgentVariety;

   // The following parameters determine the way the REFER will be addressed

   // If the target URI is a tel: URI and it begins with a +, which 
   // signifies a full international number in E.164 format, 
   // then the + will be replaced with the value of mTargetPrefix
   resip::Data mTargetPrefix;
   // For any tel: URI, the targetDomain will be appended to the number, to
   // construct a SIP URI
   resip::Data mTargetDomain;

   resip::Data mCertPath;
   resip::Data mCADirectory;


};


#endif


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

