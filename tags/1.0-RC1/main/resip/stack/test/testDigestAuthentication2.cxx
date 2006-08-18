#include <assert.h>
#include <iostream>
#include <string.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <memory>

#ifdef WIN32
#define usleep(x) Sleep(x/1000)
#define sleep(x) Sleep(x*1000)
#endif

#include "resip/stack/HeaderFieldValue.hxx"
#include "resip/stack/HeaderTypes.hxx"
#include "resip/stack/ParserCategories.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/test/TestSupport.hxx"
#include "rutil/Timer.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/MD5Stream.hxx"
#include "digcalc.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace std;
using namespace resip;

int
main(int arc, char** argv)
{

   {
      char* alg = "MD5";
      char* username = "user";
      char* password = "secret";
      char* realm = "localhost";
      char* method = "REGISTER";
      char* uri = "user@host.com";
      char* nonce = "92347fea23";


      MD5Stream a1;
      a1 << username
         << Symbols::COLON
         << realm
         << Symbols::COLON
         << password;
      Data passwordHashA1 = a1.getHex();


      Data responseMD5withA1 = Helper::makeResponseMD5WithA1(passwordHashA1,
                                                       method,
                                                       uri,
                                                       nonce);


      Data responseMD5 = Helper::makeResponseMD5(username,
                                                 password,
                                                 realm,
                                                 method,
                                                 uri,
                                                 nonce);
      
      HASHHEX a1Hash;
      HASHHEX response;

      DigestCalcHA1(alg,
                    username,
                    realm,
                    password,
                    nonce,
                    (char*)"",
                    a1Hash);

      DigestCalcResponse(a1Hash,
                         nonce,
                         (char*)"",
                         (char*)"",
                         (char*)"",
                         method,
                         uri,
                         (char*)"",
                         response);

      assert(responseMD5 == response);
      assert(responseMD5withA1 == response);
   }



   {
      Data txt("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
               "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8\r\n"
               "To: Bob <sip:bob@biloxi.com>\r\n"
               "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
               "Call-ID: a84b4c76e66710\r\n"
               "CSeq: 314159 INVITE\r\n"
               "Max-Forwards: 70\r\n"
               "Contact: <sip:alice@pc33.atlanta.com>\r\n"
               "Content-Type: application/sdp\r\n"
               "Content-Length: 150\r\n"
               "\r\n"
               "v=0\r\n"
               "o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
               "s=-\r\n"
               "c=IN IP4 pc33.atlanta.com\r\n"
               "t=0 0\r\n"
               "m=audio 3456 RTP/AVP 0 1 3 99\r\n"
               "a=rtpmap:0 PCMU/8000\r\n");
      
      auto_ptr<SipMessage> request(TestSupport::makeMessage(txt.c_str()));      

      Data realm = "localhost";
      auto_ptr<SipMessage> challenge(Helper::makeProxyChallenge(*request, realm, false));

      assert(challenge->exists(h_ProxyAuthenticates));
      assert(challenge->header(h_ProxyAuthenticates).size() == 1);

      Data username = "bob";
      Data password = "secret";
      Data cnonce;// = "366fead6";
      unsigned int nc = 0;
      MD5Stream a1;
      a1 << username
         << Symbols::COLON
         << realm
         << Symbols::COLON
         << password;
      Data passwordHashA1 = a1.getHex();
      InfoLog (<< "passwordHashA1=" << passwordHashA1);
      Data cnonceRet;
      request->header(h_ProxyAuthorizations).push_back( Helper::makeChallengeResponseAuthWithA1(*request,
                                                                                                username, 
                                                                                                passwordHashA1,
                                                                                                *(challenge->header(h_ProxyAuthenticates).begin()),
                                                                                                cnonce,
                                                                                                nc,
                                                                                                cnonceRet));

      assert(request->exists(h_ProxyAuthorizations));
      assert(request->header(h_ProxyAuthorizations).size() == 1);
      assert(!request->header(h_ProxyAuthorizations).front().exists(p_qop));

      const Auth& auth = request->header(h_ProxyAuthorizations).front();
      
      assert(auth.param(p_username) == "bob");
      assert(auth.param(p_uri) == "sip:bob@biloxi.com");
      assert(auth.param(p_algorithm) == "MD5");

      Helper::AuthResult res = Helper::authenticateRequest(*request, 
                                                           realm,
                                                           password);
      assert(res == Helper::Authenticated);

      res = Helper::authenticateRequest(*request, 
                                        realm,
                                        password.md5(),
                                        5);

      assert(res == Helper::Authenticated);

      sleep(2);
      res = Helper::authenticateRequest(*request, 
                                        realm,
                                        password.md5(),
                                        1);
 
      assert(res == Helper::Expired);
      
   }

   cerr << "ALL OK" << endl;
   return 0;
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
