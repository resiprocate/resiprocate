#include <assert.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <unistd.h>
#include <memory>

#include "resiprocate/sipstack/HeaderFieldValue.hxx"
#include "resiprocate/sipstack/HeaderTypes.hxx"
#include "resiprocate/sipstack/ParserCategories.hxx"
#include "resiprocate/sipstack/Uri.hxx"
#include "resiprocate/sipstack/Helper.hxx"
#include "resiprocate/sipstack/test/TestSupport.hxx"
#include "resiprocate/util/Timer.hxx"
#include "resiprocate/util/DataStream.hxx"
#include "resiprocate/util/MD5Stream.hxx"
#include "digcalc.hxx"

using namespace std;
using namespace Vocal2;

int
main(int arc, char** argv)
{
   {
      assert(Data("").md5() == "d41d8cd98f00b204e9800998ecf8427e");
      assert(Data("a").md5() == "0cc175b9c0f1b6a831c399e269772661");
      assert(Data("abc").md5() == "900150983cd24fb0d6963f7d28e17f72");
      assert(Data("message digest").md5() == "f96b697d7cb7938d525a2f31aaf161d0");
      assert(Data("abcdefghijklmnopqrstuvwxyz").md5() == "c3fcd3d76192e4007dfb496cca67e13b");
      assert(Data("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789").md5() == "d174ab98d277d9f5a5611c2c9f419d9f");
      assert(Data("12345678901234567890123456789012345678901234567890123456789012345678901234567890").md5() == "57edf4a22be3c955ac49da2e2107b67a");
   }
   
   {
      {
         MD5Stream s;
         assert(s.getHex() == Data("").md5());
      }
      {
         MD5Stream s;
         s << "a";
         assert(s.getHex() == Data("a").md5());
      }
      {
         MD5Stream s;
         s << "abc";
         assert(s.getHex() == Data("abc").md5());
      }
      {
         MD5Stream s;
         s << "message digest";
         assert(s.getHex() == Data("message digest").md5());
      }
      {
         MD5Stream s;
         s << "12345678901234567890123456789012345678901234567890123456789012345678901234567890";
         assert(s.getHex() == Data("12345678901234567890123456789012345678901234567890123456789012345678901234567890").md5());
      }
      {
         Data d;
         DataStream ds(d);
         MD5Stream s;

         s << "this involves" << 7.8 << "foo" << 34653453 << -6 << "hike";
         ds << "this involves" << 7.8 << "foo" << 34653453 << -6 << "hike";
         ds.flush();

         assert(d.md5() == s.getHex());
      }
   }

   {
      char* alg = "MD5";
      char* username = "user";
      char* password = "secret";
      char* realm = "localhost";
      char* method = "REGISTER";
      char* uri = "user@host.com";
      char* nonce = "92347fea23";

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
   }

   {
      char* alg = "MD5";
      char* username = "user";
      char* password = "secret";
      char* realm = "localhost";
      char* method = "REGISTER";
      char* uri = "user@host.com";
      char* nonce = "92347fea23";
      char* cnonce = "72345hef";
      char* cnonceCount = "00000001";
      char* qop = "auth";

      Data responseMD5 = Helper::makeResponseMD5(username,
                                                 password,
                                                 realm,
                                                 method,
                                                 uri,
                                                 nonce,
                                                 qop,
                                                 cnonce,
                                                 cnonceCount);
      
      HASHHEX a1Hash;
      HASHHEX response;

      DigestCalcHA1(alg,
                    username,
                    realm,
                    password,
                    nonce,
                    cnonce,
                    a1Hash);

      DigestCalcResponse(a1Hash,
                         nonce,
                         cnonceCount,
                         cnonce,
                         qop,
                         method,
                         uri,
                         (char*)"",
                         response);

      assert(responseMD5 == response);
   }

   {
      char* alg = "MD5";
      char* username = "user";
      char* password = "secret";
      char* realm = "localhost";
      char* method = "REGISTER";
      char* uri = "user@host.com";
      char* nonce = "92347fea23";

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
   }

/*
REGISTER sip:kelowna.gloo.net SIP/2.0
To: sip:100@kelowna.gloo.net
From: <sip:100@kelowna.gloo.net>
Call-ID: 000532ff-828108c2-79016ad7-69ac4815@192.168.2.233
CSeq: 102 REGISTER
Contact: sip:100@192.168.2.233:5060
Via: SIP/2.0/UDP 192.168.2.233:5060;received=192.168.2.233;rport=5060
Expires: 3600
Date: Sat, 07 Dec 2002 02:21:59 GMT
Proxy-Authorization: Digest username="sip:100@kelowna.gloo.net:5060",realm="kelowna.gloo.net",uri="sip:kelowna.gloo.net",response="8485db84088e6be6c55717d2eb891eca",nonce="1039227719:9e17fc5e10c30f162e7a21c9f6a4d2a7",algorithm=MD5
User-Agent: CSCO/4
Content-Length: 0
*/

/*
Proxy-Authorization: Digest username="sip:100@kelowna.gloo.net:5060",realm="kelowna.gloo.net",uri="sip:kelowna.gloo.net",response="8485db84088e6be6c55717d2eb891eca",nonce="1039227719:9e17fc5e10c30f162e7a21c9f6a4d2a7",algorithm=MD5
*/
   {
      char* alg = "MD5";
      char* username = "sip:100@kelowna.gloo.net:5060";
      char* password = "secret";
      char* realm = "kelowna.gloo.net";
      char* method = "REGISTER";
      char* uri = "sip:kelowna.gloo.net";
      char* nonce = "1039227719:9e17fc5e10c30f162e7a21c9f6a4d2a7";
      char* cnonce = "";
      char* cnonceCount = "";
      char* qop = "";

      Data responseMD5 = Helper::makeResponseMD5(username,
                                                 password,
                                                 realm,
                                                 method,
                                                 uri,
                                                 nonce,
                                                 qop,
                                                 cnonce,
                                                 cnonceCount);
      
      HASHHEX a1Hash;
      HASHHEX response;

      DigestCalcHA1(alg,
                    username,
                    realm,
                    password,
                    nonce,
                    cnonce,
                    a1Hash);

      DigestCalcResponse(a1Hash,
                         nonce,
                         cnonceCount,
                         cnonce,
                         qop,
                         method,
                         uri,
                         (char*)"",
                         response);

      assert(responseMD5 == response);
   }


/*
Calling form_SIPdigest with:
  nonce    = 1039063045
  user     = sip:100@kelowna.gloo.net:5060
  pswd     = secret
  method   = REGISTER
  uri      = sip:kelowna.gloo.net
  realm    = kelowna.gloo.net
  algorithm= MD5
Message digest    == 575a9ecd3a6f1989a978748217b24a25
Calculated digest == 575a9ecd3a6f1989a978748217b24a25
*/
   {
      char* alg = "MD5";
      char* username = "sip:100@kelowna.gloo.net:5060";
      char* password = "secret";
      char* realm = "kelowna.gloo.net";
      char* method = "REGISTER";
      char* uri = "sip:kelowna.gloo.net";
      char* nonce = "1039063045";

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
      assert(responseMD5 == "575a9ecd3a6f1989a978748217b24a25");
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
      Data cnonce = "366fead6";
      unsigned int nc = 0;
      
      Data encodedPassword = password.md5();

      Helper::addAuthorization(*request,
                               *challenge,
                               username,
                               encodedPassword,
                               cnonce,
                               nc);

      assert(request->exists(h_ProxyAuthorizations));
      assert(request->header(h_ProxyAuthorizations).size() == 1);
      assert(!request->header(h_ProxyAuthorizations).front().exists(p_qop));

      const Auth& auth = request->header(h_ProxyAuthorizations).front();
      
      assert(auth.param(p_username) == "bob");
      assert(auth.param(p_uri) == "sip:bob@biloxi.com");
      assert(auth.param(p_algorithm) == "MD5");

      Helper::AuthResult res = Helper::authenticateRequest(*request, 
                                                           realm,
                                                           encodedPassword);
      assert(res == Helper::Authenticated);

      res = Helper::authenticateRequest(*request, 
                                        realm,
                                        encodedPassword,
                                        5);

      assert(res == Helper::Authenticated);

      sleep(2);
      res = Helper::authenticateRequest(*request, 
                                        realm,
                                        encodedPassword,
                                        1);
 
      assert(res == Helper::Expired);
      
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
      auto_ptr<SipMessage> challenge(Helper::makeProxyChallenge(*request, realm, true));

      cerr << *challenge << endl;

      assert(challenge->exists(h_ProxyAuthenticates));
      assert(challenge->header(h_ProxyAuthenticates).size() == 1);
      assert(challenge->header(h_ProxyAuthenticates).front().exists(p_qopOptions));
      assert(challenge->header(h_ProxyAuthenticates).front().param(p_algorithm) == "MD5");
      assert(challenge->header(h_ProxyAuthenticates).front().scheme() == "Digest");

      Data username = "bob";
      Data password = "secret";
      Data cnonce = "366fead6";
      unsigned int nc = 9;
      
      Data encodedPassword = password.md5();

      Helper::addAuthorization(*request,
                               *challenge,
                               username,
                               encodedPassword,
                               cnonce,
                               nc);

      assert(nc == 10);

      cerr << *request << endl;

      assert(request->exists(h_ProxyAuthorizations));
      assert(request->header(h_ProxyAuthorizations).size() == 1);
      assert(request->header(h_ProxyAuthorizations).front().exists(p_qop));
      assert(request->header(h_ProxyAuthorizations).front().param(p_nc) == "0000000a");

      const Auth& auth = request->header(h_ProxyAuthorizations).front();
      
      assert(auth.param(p_username) == "bob");
      assert(auth.param(p_uri) == "sip:bob@biloxi.com");
      assert(auth.param(p_algorithm) == "MD5");

      Helper::AuthResult res = Helper::authenticateRequest(*request, 
                                                           realm,
                                                           encodedPassword);
      assert(res == Helper::Authenticated);

      res = Helper::authenticateRequest(*request, 
                                        realm,
                                        encodedPassword,
                                        5);

      assert(res == Helper::Authenticated);

      sleep(2);
      res = Helper::authenticateRequest(*request, 
                                        realm,
                                        encodedPassword,
                                        1);
 
      assert(res == Helper::Expired);
      
   }
   cerr << "ALL OK" << endl;
}
