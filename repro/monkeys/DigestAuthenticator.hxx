#if !defined(RESIP_DIGEST_AUTHENTICATOR_HXX)
#define RESIP_DIGEST_AUTHENTICATOR_HXX 

#include "rutil/Data.hxx"
#include "resip/stack/Auth.hxx"
#include "repro/Processor.hxx"
#include "repro/Dispatcher.hxx"
#include "repro/ProxyConfig.hxx"
#include "repro/UserInfoMessage.hxx"

namespace resip
{
   class SipStack;
}

namespace repro
{
  class DigestAuthenticator : public Processor
  {
    public:
      DigestAuthenticator(ProxyConfig& config, Dispatcher* authRequestDispatcher, const resip::Data& staticRealm = resip::Data::Empty);
      ~DigestAuthenticator();

      virtual processor_action_t process(RequestContext &);

    protected:
      virtual bool authorizedForThisIdentity(const resip::Data &user, const resip::Data &realm, resip::Uri &fromUri);
      virtual resip::NameAddr getDefaultIdentity(const resip::Data &user, const resip::Data &realm, resip::NameAddr &from);
      virtual void challengeRequest(RequestContext &, bool stale = false);
      virtual processor_action_t requestUserAuthInfo(RequestContext &, resip::Data & realm);
      virtual processor_action_t requestUserAuthInfo(RequestContext &, const resip::Auth& auth, UserInfoMessage *userInfo);
      virtual resip::Data getRealm(RequestContext &);
      virtual bool isMyRealm(RequestContext &, const resip::Data& realm);
      
    private:
      Dispatcher* mAuthRequestDispatcher;
      resip::Data mStaticRealm;
      bool mNoIdentityHeaders;
      resip::Data mHttpHostname;  // Used in identity headers
      int  mHttpPort;
      bool mUseAuthInt;
      bool mRejectBadNonces;
  };
  
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
