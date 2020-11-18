#if !defined(RECON_CONFIG_HXX)
#define RECON_CONFIG_HXX 

#include <sys/ioctl.h>
#include <stdio.h>

#include <map>
#include <asio.hpp>
#include <rutil/ConfigParse.hxx>
#include <rutil/Data.hxx>
#include <rutil/Log.hxx>
#include <rutil/BaseException.hxx>
#include <resip/stack/SipConfigParse.hxx>
#include <resip/recon/UserAgent.hxx>

namespace reconserver {

class ReConServerConfig : public resip::SipConfigParse
{
public:

   typedef enum
   {
      None,
      B2BUA
   } Application;

   ReConServerConfig();
   virtual ~ReConServerConfig();

   void printHelpText(int argc, char **argv);
   using resip::ConfigParse::getConfigValue;

   bool getConfigValue(const resip::Data& name, recon::ConversationProfile::SecureMediaMode &value);
   recon::ConversationProfile::SecureMediaMode getConfigSecureMediaMode(const resip::Data& name, const recon::ConversationProfile::SecureMediaMode defaultValue);
   bool isSecureMediaModeRequired();

   bool getConfigValue(const resip::Data& name, recon::ConversationProfile::NatTraversalMode &value);
   recon::ConversationProfile::NatTraversalMode getConfigNatTraversalMode(const resip::Data& name, const recon::ConversationProfile::NatTraversalMode defaultValue);

   bool getConfigValue(const resip::Data& name, Application& defaultValue);
   Application getConfigApplication(const resip::Data& name, const Application defaultValue);


   
private:

   bool mSecureMediaRequired;

};

} // namespace

#endif


/* ====================================================================
 *
 * Copyright 2013 Catalin Constantin Usurelu.  All rights reserved.
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

