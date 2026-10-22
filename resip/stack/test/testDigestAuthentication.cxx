#include <assert.h>
#include <iostream>
#include <string.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <memory>
#include <vector>

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
#include "rutil/Logger.hxx"
#include "rutil/DigestStream.hxx"
#include "digcalc.hxx"

using namespace std;
using namespace resip;

int
main(int arc, char** argv)
{
   Log::initialize(Log::Cout, Log::Stack, argv[0]);

   // Test Helper::isDigestAlgorithmSupported() 
   {
      //static bool isDigestAlgorithmSupported(const Data & algorithmName, DigestType & digestType);
      DigestType digestType;
      assert(!Helper::isDigestAlgorithmSupported("blah", digestType));
      assert(Helper::isDigestAlgorithmSupported("MD5", digestType));
      assert(digestType == MD5);
      assert(Helper::isDigestAlgorithmSupported("md5", digestType));
      assert(digestType == MD5);
#ifdef USE_SSL
      assert(!Helper::isDigestAlgorithmSupported("SHA256", digestType));
      assert(Helper::isDigestAlgorithmSupported("SHA-256", digestType));
      assert(digestType == SHA256);
      assert(Helper::isDigestAlgorithmSupported("SHA-512-256", digestType));
      assert(digestType == SHA512_256);
#endif

      //static bool isDigestAlgorithmSupported(const Auth & auth, DigestType & digestType);
      Auth auth;
      assert(Helper::isDigestAlgorithmSupported(auth, digestType));  // No algorithm, defaults to MD5
      assert(digestType == MD5);

      auth.param(p_algorithm) = "MD5";
      assert(Helper::isDigestAlgorithmSupported(auth, digestType));
      assert(digestType == MD5);

      auth.param(p_algorithm) = "blah";
      assert(!Helper::isDigestAlgorithmSupported(auth, digestType));

#ifdef USE_SSL
      auth.param(p_algorithm) = "SHA-256";
      assert(Helper::isDigestAlgorithmSupported(auth, digestType));
      assert(digestType == SHA256);

      auth.param(p_algorithm) = "SHA-512-256";
      assert(Helper::isDigestAlgorithmSupported(auth, digestType));
      assert(digestType == SHA512_256);
#endif
   }

   // Test Helper::createResipA1HashString() and Helper::extractA1FromResipA1HashString()
   {
      Data resipA1String = Helper::createResipA1HashString("user", "example.com", "secret");
      assert(resipA1String == "[MD5]30969131580e626606ce70ceaab77719[SHA-256]de24c1fac8b54ea8380bb2299c3bbcf6489084c44deb242d18fe409698148935[SHA-512-256]28c22ce0f545ffd15d749d51fad525b918ffbda8183303241fff3166a2e53cc0");

      assert(Helper::extractA1FromResipA1HashString(resipA1String, MD5) == "30969131580e626606ce70ceaab77719");
      assert(Helper::extractA1FromResipA1HashString(resipA1String, SHA256) == "de24c1fac8b54ea8380bb2299c3bbcf6489084c44deb242d18fe409698148935");
      assert(Helper::extractA1FromResipA1HashString(resipA1String, SHA512_256) == "28c22ce0f545ffd15d749d51fad525b918ffbda8183303241fff3166a2e53cc0");

      // Test legacy string with no algorithm tag
      assert(Helper::extractA1FromResipA1HashString("30969131580e626606ce70ceaab77719", MD5) == "30969131580e626606ce70ceaab77719");
      assert(Helper::extractA1FromResipA1HashString("30969131580e626606ce70ceaab77719", SHA256) == Data::Empty);
      assert(Helper::extractA1FromResipA1HashString("30969131580e626606ce70ceaab77719", SHA512_256) == Data::Empty);

      // Test malformed strings
      assert(Helper::extractA1FromResipA1HashString("30969131580e626606ce70ceaab7771", MD5) == Data::Empty); // Hash too short
      assert(Helper::extractA1FromResipA1HashString("30969131580e626606ce70ceaab77719blah", MD5) == Data::Empty); // Hash too long
      assert(Helper::extractA1FromResipA1HashString("", MD5) == Data::Empty); // Hash empty
      assert(Helper::extractA1FromResipA1HashString("", SHA256) == Data::Empty); // Hash empty
      assert(Helper::extractA1FromResipA1HashString("[MD5]30969131580e626606ce70ceaab77719[SHA-256]de24c1fac8b54ea8380bb2299c3bbcf6489084c44deb242d18fe409698148935", SHA512_256) == Data::Empty); // No SHA-512-256 tag
   }

   // Test Helper::makeChallenge
   {
      //SipMessage* Helper::makeChallenge(const SipMessage & request, const Data & realm, bool useAuth, bool stale, bool proxy, const std::vector<DigestType>&digestTypes)
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

      unique_ptr<SipMessage> request(TestSupport::makeMessage(txt.c_str()));

      Data realm = "localhost";
      unique_ptr<SipMessage> challenge(Helper::makeChallenge(*request, realm, false /* useAuth? */, true /* stale? */, false /* proxy? */, std::vector<DigestType>{ MD5, SHA256, SHA512_256}));

      assert(challenge->exists(h_WWWAuthenticates));
      assert(challenge->header(h_WWWAuthenticates).size() == 3);

      assert(!challenge->header(h_WWWAuthenticates).front().exists(p_qopOptions));
      assert(challenge->header(h_WWWAuthenticates).front().param(p_algorithm) == "MD5");
      assert(challenge->header(h_WWWAuthenticates).front().param(p_stale) == "true");
      assert(challenge->header(h_WWWAuthenticates).front().scheme() == "Digest");

      assert(!challenge->header(h_WWWAuthenticates).back().exists(p_qopOptions));
      assert(challenge->header(h_WWWAuthenticates).back().param(p_algorithm) == "SHA-512-256");
      assert(challenge->header(h_WWWAuthenticates).back().param(p_stale) == "true");
      assert(challenge->header(h_WWWAuthenticates).back().scheme() == "Digest");
   }

   // Test Helper::qopOption
   {
      //Data Helper::qopOption(const Auth& challenge)
      Auth auth;

      assert(Helper::qopOption(auth) == Data::Empty); // No qopOptions parameter
      auth.param(p_qopOptions) = "";
      assert(Helper::qopOption(auth) == Data::Empty); // Empty qopOptions parameter
      auth.param(p_qopOptions) = "auth";
      assert(Helper::qopOption(auth) == "auth");
      auth.param(p_qopOptions) = "auth-int";
      assert(Helper::qopOption(auth) == "auth-int");
      auth.param(p_qopOptions) = "auth,auth-int";
      assert(Helper::qopOption(auth) == "auth-int");
      auth.param(p_qopOptions) = "auth-int,auth";
      assert(Helper::qopOption(auth) == "auth-int");
      auth.param(p_qopOptions) = "a,auth,b,c";
      assert(Helper::qopOption(auth) == "auth");
      auth.param(p_qopOptions) = " a,auth,b,c ";
      assert(Helper::qopOption(auth) == "auth");
      auth.param(p_qopOptions) = " auth,b,c "; // We only tolerate whitespace before first item
      assert(Helper::qopOption(auth) == "auth");
      auth.param(p_qopOptions) = "auth,auth-int,auth";
      assert(Helper::qopOption(auth) == "auth-int");
   }

   // Test Helper::algorithmAndQopSupported()
   {
      Auth auth;
      auth.scheme() = "Digest";
      auth.param(p_nonce) = "askdfjhaslkjhf498hw98hw98hfsf";
      auth.param(p_algorithm) = "MD5";
      auth.param(p_realm) = "example.com";
      
      DigestType digestType;
      assert(Helper::algorithmAndQopSupported(auth));
      auth.param(p_algorithm) = "MD5-sess";
      assert(!Helper::algorithmAndQopSupported(auth));
      auth.param(p_algorithm) = "monkey";
      assert(!Helper::algorithmAndQopSupported(auth));
#ifndef USE_SSL
      auth.param(p_algorithm) = "SHA-256";
      assert(!Helper::algorithmAndQopSupported(auth));
      auth.param(p_algorithm) = "SHA-512-256";
      assert(!Helper::algorithmAndQopSupported(auth));
#endif

      auth.param(p_algorithm) = "MD5";
      auth.param(p_qop) = Symbols::auth;
      assert(Helper::algorithmAndQopSupported(auth, digestType));
      assert(digestType == MD5);
#ifdef USE_SSL
      auth.param(p_algorithm) = "SHA-256";
      assert(Helper::algorithmAndQopSupported(auth, digestType));
      assert(digestType == SHA256);
      auth.param(p_algorithm) = "SHA-512-256";
      assert(Helper::algorithmAndQopSupported(auth, digestType));
      assert(digestType == SHA512_256);
#endif

      auth.param(p_qop) = Symbols::authInt;
      assert(Helper::algorithmAndQopSupported(auth));
      
      auth.param(p_qop) = "monkey";
      assert(!Helper::algorithmAndQopSupported(auth));

      cerr << "algorithmAndQopSupported passed" << endl;
   }
   
   // Test Data::md5() - not really used in DigestAuthentication, but we will verify anyway
   {
      assert(Data("").md5() == "d41d8cd98f00b204e9800998ecf8427e");
      assert(Data("a").md5() == "0cc175b9c0f1b6a831c399e269772661");
      assert(Data("abc").md5() == "900150983cd24fb0d6963f7d28e17f72");
      assert(Data("message digest").md5() == "f96b697d7cb7938d525a2f31aaf161d0");
      assert(Data("abcdefghijklmnopqrstuvwxyz").md5() == "c3fcd3d76192e4007dfb496cca67e13b");
      assert(Data("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789").md5() == "d174ab98d277d9f5a5611c2c9f419d9f");
      assert(Data("12345678901234567890123456789012345678901234567890123456789012345678901234567890").md5() == "57edf4a22be3c955ac49da2e2107b67a");
   }
   
   // Verify Data::md5() output matches DigestStream output
   {
      {
         DigestStream s;
         assert(s.getHex() == Data("").md5());
      }
      {
         DigestStream s;
         s << "a";
         assert(s.getHex() == Data("a").md5());
      }
      {
         DigestStream s;
         s << "abc";
         assert(s.getHex() == Data("abc").md5());
      }
      {
         DigestStream s;
         s << "message digest";
         assert(s.getHex() == Data("message digest").md5());
      }
      {
         DigestStream s;
         s << "12345678901234567890123456789012345678901234567890123456789012345678901234567890";
         assert(s.getHex() == Data("12345678901234567890123456789012345678901234567890123456789012345678901234567890").md5());
      }
      {
         Data d;
         DataStream ds(d);
         DigestStream s;

         s << "this involves" << 7.8 << "foo" << 34653453 << -6 << "hike";
         ds << "this involves" << 7.8 << "foo" << 34653453 << -6 << "hike";
         ds.flush();

         assert(d.md5() == s.getHex());
      }
   }

   // Test Helper::makeResponseMD5()
   {
      const char* alg = "MD5";
      const char* username = "user";
      const char* password = "secret";
      const char* realm = "localhost";
      const char* method = "REGISTER";
      const char* uri = "user@host.com";
      const char* nonce = "92347fea23";

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

   // Test Helper::makeResponseMD5() with qop
   {
      const char* alg = "MD5";
      const char* username = "user";
      const char* password = "secret";
      const char* realm = "localhost";
      const char* method = "REGISTER";
      const char* uri = "user@host.com";
      const char* nonce = "92347fea23";
      const char* cnonce = "72345hef";
      const char* cnonceCount = "00000001";
      const char* qop = "auth";

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
      const char* alg = "MD5";
      const char* username = "sip:100@kelowna.gloo.net:5060";
      const char* password = "secret";
      const char* realm = "kelowna.gloo.net";
      const char* method = "REGISTER";
      const char* uri = "sip:kelowna.gloo.net";
      const char* nonce = "1039227719:9e17fc5e10c30f162e7a21c9f6a4d2a7";
      const char* cnonce = "";
      const char* cnonceCount = "";
      const char* qop = "";

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
      const char* alg = "MD5";
      const char* username = "sip:100@kelowna.gloo.net:5060";
      const char* password = "secret";
      const char* realm = "kelowna.gloo.net";
      const char* method = "REGISTER";
      const char* uri = "sip:kelowna.gloo.net";
      const char* nonce = "1039063045";

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

   // Test MD5 end-to-end challenge and authorization (no qop/auth)
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

      unique_ptr<SipMessage> request(TestSupport::makeMessage(txt.c_str()));

      Data realm = "localhost";
      unique_ptr<SipMessage> challenge(Helper::makeProxyChallenge(*request, realm, false /* useAuth? */));

      cout << "MD5/NoAuth Challenge:\n" << *challenge << endl;

      assert(challenge->exists(h_ProxyAuthenticates));
      assert(challenge->header(h_ProxyAuthenticates).size() == 1);

      Data username = "bob";
      Data password = "secret";
      Data cnonce = "366fead6";
      unsigned int nc = 0;

      Data passwordResipA1 = Helper::createResipA1HashString(username, realm, password);

      Helper::addAuthorization(*request,
                               *challenge,
                               username,
                               password,
                               cnonce,
                               nc);

      cout << "MD5/NoAuth Request with Proxy-Authorization:\n" << *request << endl;

      assert(request->exists(h_ProxyAuthorizations));
      assert(request->header(h_ProxyAuthorizations).size() == 1);

      const Auth& auth = request->header(h_ProxyAuthorizations).front();

      assert(!auth.exists(p_qop));
      assert(auth.param(p_username) == "bob");
      assert(auth.param(p_uri) == "sip:bob@biloxi.com");
      assert(auth.param(p_algorithm) == "MD5");

      // Test makeChallengeResponseAuth directly - also get's called in addAuthorization above
      resip::Data nonceCountString;
      Auth makeChallengeResponseAuth = Helper::makeChallengeResponseAuth(*request, username, password, challenge->header(h_ProxyAuthenticates).front(), cnonce, nc, nonceCountString);
      assert(nonceCountString == Data::Empty);
      assert(!makeChallengeResponseAuth.exists(p_qop));
      assert(makeChallengeResponseAuth.param(p_username) == username);
      assert(makeChallengeResponseAuth.param(p_realm) == realm);
      assert(makeChallengeResponseAuth.param(p_uri) == "sip:bob@biloxi.com");
      assert(makeChallengeResponseAuth.param(p_algorithm) == "MD5");
      Auth makeChallengeResponseAuthWithA1 = Helper::makeChallengeResponseAuthWithA1(*request, username, passwordResipA1, challenge->header(h_ProxyAuthenticates).front(), cnonce, nc, nonceCountString);
      assert(makeChallengeResponseAuthWithA1.param(p_response) == makeChallengeResponseAuth.param(p_response));

      // Note: We are testing 3 variants of the authenticateRequest logic below

      Helper::AuthResult res = Helper::authenticateRequest(*request, realm, password);
      assert(res == Helper::Authenticated);
      res = Helper::authenticateRequestWithA1(*request, realm, passwordResipA1);
      assert(res == Helper::Authenticated);
      auto respair = Helper::advancedAuthenticateRequest(*request, realm, passwordResipA1);
      assert(respair.first == Helper::Authenticated);

      res = Helper::authenticateRequest(*request, realm, password, 5);
      assert(res == Helper::Authenticated);
      res = Helper::authenticateRequestWithA1(*request, realm, passwordResipA1, 5);
      assert(res == Helper::Authenticated);
      respair = Helper::advancedAuthenticateRequest(*request, realm, passwordResipA1, 5);
      assert(respair.first == Helper::Authenticated);

      // Sleep for 2 seconds, then test ExpiresDelta of 1
      sleep(2);

      res = Helper::authenticateRequest(*request, realm, password, 1);
      assert(res == Helper::Expired);
      res = Helper::authenticateRequestWithA1(*request, realm, passwordResipA1, 1);
      assert(res == Helper::Expired);
      respair = Helper::advancedAuthenticateRequest(*request, realm, passwordResipA1, 1);
      assert(respair.first == Helper::Expired);
   }

   // Test MD5 end-to-end challenge and authorization (with qop/auth)
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

      unique_ptr<SipMessage> request(TestSupport::makeMessage(txt.c_str()));

      Data realm = "localhost";
      unique_ptr<SipMessage> challenge(Helper::makeProxyChallenge(*request, realm, true /* useAuth? */));

      cout << "MD5/Auth Challenge:\n" << *challenge << endl;

      assert(challenge->exists(h_ProxyAuthenticates));
      assert(challenge->header(h_ProxyAuthenticates).size() == 1);
      assert(challenge->header(h_ProxyAuthenticates).front().exists(p_qopOptions));
      assert(challenge->header(h_ProxyAuthenticates).front().param(p_algorithm) == "MD5");
      assert(challenge->header(h_ProxyAuthenticates).front().scheme() == "Digest");

      Data username = "bob";
      Data password = "secret";
      Data cnonce = "366fead6";
      unsigned int nc = 9;
      
      Data passwordResipA1 = Helper::createResipA1HashString(username, realm, password);

      Helper::addAuthorization(*request,
                               *challenge,
                               username,
                               password,
                               cnonce,
                               nc);

      assert(nc == 10);

      cout << "MD5/Auth Request with Proxy-Authorization:\n" << *request << endl;

      assert(request->exists(h_ProxyAuthorizations));
      assert(request->header(h_ProxyAuthorizations).size() == 1);

      const Auth& auth = request->header(h_ProxyAuthorizations).front();
      
      assert(auth.exists(p_qop));
      assert(auth.param(p_qop) == "auth-int");
      assert(auth.param(p_nc) == "0000000a");
      assert(auth.param(p_username) == "bob");
      assert(auth.param(p_uri) == "sip:bob@biloxi.com");
      assert(auth.param(p_algorithm) == "MD5");

      // Test makeChallengeResponseAuth directly - also get's called in addAuthorization above
      nc = 9;  // Put back to 9, so results are same as above
      resip::Data nonceCountString;
      Auth makeChallengeResponseAuth = Helper::makeChallengeResponseAuth(*request, username, password, challenge->header(h_ProxyAuthenticates).front(), cnonce, nc, nonceCountString);
      assert(nc == 10);
      assert(nonceCountString == "0000000a");
      assert(makeChallengeResponseAuth.exists(p_qop));
      assert(makeChallengeResponseAuth.param(p_qop) == "auth-int");
      assert(makeChallengeResponseAuth.param(p_username) == username);
      assert(makeChallengeResponseAuth.param(p_realm) == realm);
      assert(makeChallengeResponseAuth.param(p_uri) == "sip:bob@biloxi.com");
      assert(makeChallengeResponseAuth.param(p_algorithm) == "MD5");
      nc = 9;  // Put back to 9, so results are same as above
      Auth makeChallengeResponseAuthWithA1 = Helper::makeChallengeResponseAuthWithA1(*request, username, passwordResipA1, challenge->header(h_ProxyAuthenticates).front(), cnonce, nc, nonceCountString);
      assert(makeChallengeResponseAuthWithA1.param(p_response) == makeChallengeResponseAuth.param(p_response));

      // Note: We are testing 3 variants of the authenticateRequest logic below

      Helper::AuthResult res = Helper::authenticateRequest(*request, realm, password);
      assert(res == Helper::Authenticated);
      res = Helper::authenticateRequestWithA1(*request, realm, passwordResipA1);
      assert(res == Helper::Authenticated);
      auto respair = Helper::advancedAuthenticateRequest(*request, realm, passwordResipA1);
      assert(respair.first == Helper::Authenticated);

      res = Helper::authenticateRequest(*request, realm, password, 5);
      assert(res == Helper::Authenticated);
      res = Helper::authenticateRequestWithA1(*request, realm, passwordResipA1, 5);
      assert(res == Helper::Authenticated);
      respair = Helper::advancedAuthenticateRequest(*request, realm, passwordResipA1, 5);
      assert(respair.first == Helper::Authenticated);

      // Sleep for 2 seconds, then test ExpiresDelta of 1
      sleep(2);

      res = Helper::authenticateRequest(*request, realm, password, 1); 
      assert(res == Helper::Expired);
      res = Helper::authenticateRequestWithA1(*request, realm, passwordResipA1, 1);
      assert(res == Helper::Expired);
      respair = Helper::advancedAuthenticateRequest(*request, realm, passwordResipA1, 1);
      assert(respair.first == Helper::Expired);
   }

   // Test MD5 auth-int - against known test vector
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

      unique_ptr<SipMessage> request(TestSupport::makeMessage(txt.c_str()));

      Data realm = "localhost";
      unique_ptr<SipMessage> challenge(Helper::makeProxyChallenge(*request, realm, true /* useAuth? */));

      // Use fixed nonce for test vector
      challenge->header(h_ProxyAuthenticates).front().param(p_nonce) = "1627567:31f7bffca3fc119082ad0750f496a4f8";
      cout << "MD5/Auth(known response) Challenge:\n" << *challenge << endl;

      assert(challenge->exists(h_ProxyAuthenticates));
      assert(challenge->header(h_ProxyAuthenticates).size() == 1);
      assert(challenge->header(h_ProxyAuthenticates).front().exists(p_qopOptions));
      assert(challenge->header(h_ProxyAuthenticates).front().param(p_algorithm) == "MD5");
      assert(challenge->header(h_ProxyAuthenticates).front().scheme() == "Digest");

      Data username = "bob";
      Data password = "secret";
      Data cnonce = "366fead6";
      unsigned int nc = 8;
      
      Data passwordResipA1 = Helper::createResipA1HashString(username, realm, password);

      Helper::addAuthorization(*request,
                               *challenge,
                               username,
                               password,
                               cnonce,
                               nc);

      assert(nc == 9);

      cout << "MD5/Auth(known response) Request with Proxy-Authorization:\n" << *request << endl;

      assert(request->exists(h_ProxyAuthorizations));
      assert(request->header(h_ProxyAuthorizations).size() == 1);

      const Auth& auth = request->header(h_ProxyAuthorizations).front();
      
      assert(auth.param(p_username) == username);
      assert(auth.param(p_realm) == realm);
      assert(auth.param(p_nonce) == "1627567:31f7bffca3fc119082ad0750f496a4f8");
      assert(auth.param(p_uri) == "sip:bob@biloxi.com");
      assert(auth.param(p_algorithm) == "MD5");
      assert(auth.param(p_cnonce) == cnonce);
      assert(auth.param(p_nc) == "00000009");
      assert(auth.param(p_qop) == "auth-int");
      assert(auth.param(p_response) == "6ca4920e53a335f982edb3223f4aad6c"); // known test vector response

      // Calculations for verification
      // 1. Intermediate Values
      //    HA1: MD5(username:realm:password)
      //      String: bob:localhost:secret
      //      Result: 50241c3d4a9d6922e84d3e9434ab8bf8
      //    H(Entity-Body): The hash of the 150-byte SDP payload.
      //      Result: 5c415a72f6c4214fc9d20684c5bf3d89
      //    HA2: MD5(method:digestURI:H(Entity-Body))
      //      String: INVITE:sip:bob@biloxi.com:5c415a72f6c4214fc9d20684c5bf3d89
      //      Result: 05de463e4a1f5d240aca097b9106be2e
      // 2. The Final Response Calculation
      //    The auth-int response uses the following concatenation: MD5(HA1:nonce:nc:cnonce:qop:HA2)
      //    Combined String: 50241c3d4a9d6922e84d3e9434ab8bf8:1627567:31f7bffca3fc119082ad0750f496a4f8:00000009:366fead6:auth-int:05de463e4a1f5d240aca097b9106be2e
      //    Final MD5 Response: 6ca4920e53a335f982edb3223f4aad6c
      // 3. The Resulting Proxy-Authorization Header
      //    Proxy-Authorization: Digest username="bob",
      //       realm="localhost",
      //       nonce="1627567:31f7bffca3fc119082ad0750f496a4f8",
      //       uri="sip:bob@biloxi.com",
      //       algorithm=MD5,
      //       cnonce="366fead6",
      //       nc=00000009,
      //       qop=auth-int,
      //       response="6ca4920e53a335f982edb3223f4aad6c"
   }

#ifdef USE_SSL
   // Test SHA256 end-to-end challenge and authorization (with qop/auth)
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

      unique_ptr<SipMessage> request(TestSupport::makeMessage(txt.c_str()));

      Data realm = "localhost";
      unique_ptr<SipMessage> challenge(Helper::makeProxyChallenge(*request, realm, true /* useAuth? */, false /* stale? */, std::vector<DigestType>{SHA256,SHA512_256,MD5})); // SHA256 first

      cout << "SHA256/Auth Challenge:\n" << *challenge << endl;

      assert(challenge->exists(h_ProxyAuthenticates));
      assert(challenge->header(h_ProxyAuthenticates).size() == 3);  // 3 algorithms offered
      assert(challenge->header(h_ProxyAuthenticates).front().exists(p_qopOptions));
      assert(challenge->header(h_ProxyAuthenticates).front().param(p_algorithm) == "SHA-256");
      assert(challenge->header(h_ProxyAuthenticates).front().scheme() == "Digest");

      Data username = "bob";
      Data password = "secret";
      Data cnonce = "366fead6";
      unsigned int nc = 9;
      
      Data passwordResipA1 = Helper::createResipA1HashString(username, realm, password);

      Helper::addAuthorization(*request,
                               *challenge,
                               username,
                               password,
                               cnonce,
                               nc);

      assert(nc == 10);

      cout << "SHA256/Auth Request with Proxy-Authorization:\n" << *request << endl;

      assert(request->exists(h_ProxyAuthorizations));
      assert(request->header(h_ProxyAuthorizations).size() == 1);
      assert(request->header(h_ProxyAuthorizations).front().exists(p_qop));
      assert(request->header(h_ProxyAuthorizations).front().param(p_nc) == "0000000a");

      const Auth& auth = request->header(h_ProxyAuthorizations).front();
      
      assert(auth.param(p_username) == "bob");
      assert(auth.param(p_uri) == "sip:bob@biloxi.com");
      assert(auth.param(p_algorithm) == "SHA-256");

      Helper::AuthResult res = Helper::authenticateRequest(*request, realm, password);
      assert(res == Helper::Authenticated);

      auto respair = Helper::advancedAuthenticateRequest(*request, realm, passwordResipA1);
      assert(respair.first == Helper::Authenticated);

      res = Helper::authenticateRequest(*request, realm, password, 5);
      assert(res == Helper::Authenticated);

      respair = Helper::advancedAuthenticateRequest(*request, realm, passwordResipA1, 5);
      assert(respair.first == Helper::Authenticated);

      // Sleep for 2 seconds, then test ExpiresDelta of 1
      sleep(2);

      res = Helper::authenticateRequest(*request, realm, password, 1); 
      assert(res == Helper::Expired);

      respair = Helper::advancedAuthenticateRequest(*request, realm, passwordResipA1, 1);
      assert(respair.first == Helper::Expired);
   }

   // Test SHA256 auth-int - against known test vector
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

      unique_ptr<SipMessage> request(TestSupport::makeMessage(txt.c_str()));

      Data realm = "localhost";
      unique_ptr<SipMessage> challenge(Helper::makeProxyChallenge(*request, realm, true /* useAuth? */, false /* stale? */, std::vector<DigestType>{SHA256}));

      // Use fixed nonce for test vector
      challenge->header(h_ProxyAuthenticates).front().param(p_nonce) = "1627567:31f7bffca3fc119082ad0750f496a4f8";
      cout << "SHA256/Auth(known response) Challenge:\n" << *challenge << endl;

      assert(challenge->exists(h_ProxyAuthenticates));
      assert(challenge->header(h_ProxyAuthenticates).size() == 1);
      assert(challenge->header(h_ProxyAuthenticates).front().exists(p_qopOptions));
      assert(challenge->header(h_ProxyAuthenticates).front().param(p_algorithm) == "SHA-256");
      assert(challenge->header(h_ProxyAuthenticates).front().scheme() == "Digest");

      Data username = "bob";
      Data password = "secret";
      Data cnonce = "366fead6";
      unsigned int nc = 8;
      
      Data passwordResipA1 = Helper::createResipA1HashString(username, realm, password);

      Helper::addAuthorization(*request,
                               *challenge,
                               username,
                               password,
                               cnonce,
                               nc);

      assert(nc == 9);

      cout << "SHA256/Auth(known response) Request with Proxy-Authorization:\n" << *request << endl;

      assert(request->exists(h_ProxyAuthorizations));
      assert(request->header(h_ProxyAuthorizations).size() == 1);

      const Auth& auth = request->header(h_ProxyAuthorizations).front();
      
      assert(auth.param(p_username) == username);
      assert(auth.param(p_realm) == realm);
      assert(auth.param(p_nonce) == "1627567:31f7bffca3fc119082ad0750f496a4f8");
      assert(auth.param(p_uri) == "sip:bob@biloxi.com");
      assert(auth.param(p_algorithm) == "SHA-256");
      assert(auth.param(p_cnonce) == cnonce);
      assert(auth.param(p_nc) == "00000009");
      assert(auth.param(p_qop) == "auth-int");
      assert(auth.param(p_response) == "d1249eb66905f97aebaa7e007e539080f184267ab708c69dfdd4f6f2cd1ba4aa"); // known test vector response

      // Calculations for verification
      // 1. Intermediate Values
      //    HA1: SHA256(username:realm:password)
      //      String: bob:localhost:secret
      //      Result: db28b72b050b644511dde4a76c0677ef1d6c1c3fa82ab100f673083de3c4092d
      //    H(Entity-Body): The hash of the 150-byte SDP payload.
      //      Result: 8ca692e739f8f51fa034ba5bc5dde33ee8f56f8dac8887faf8a1ae3e05377dce
      //    HA2: SHA256(method:digestURI:H(Entity-Body))
      //      String: INVITE:sip:bob@biloxi.com:8ca692e739f8f51fa034ba5bc5dde33ee8f56f8dac8887faf8a1ae3e05377dce
      //      Result: 926fb12c21ee931aaa9a8f078b60784594214ca70a5645eb88ba0633b67feb87
      // 2. The Final Response Calculation
      //    The auth-int response uses the following concatenation: SHA256(HA1:nonce:nc:cnonce:qop:HA2)
      //    Combined String: db28b72b050b644511dde4a76c0677ef1d6c1c3fa82ab100f673083de3c4092d:1627567:31f7bffca3fc119082ad0750f496a4f8:00000009:366fead6:auth-int:926fb12c21ee931aaa9a8f078b60784594214ca70a5645eb88ba0633b67feb87
      //    Final SHA256 Response: d1249eb66905f97aebaa7e007e539080f184267ab708c69dfdd4f6f2cd1ba4aa
      // 3. The Resulting Proxy-Authorization Header
      //    Proxy-Authorization: Digest username="bob",
      //       realm="localhost",
      //       nonce="1627567:31f7bffca3fc119082ad0750f496a4f8",
      //       uri="sip:bob@biloxi.com",
      //       algorithm=SHA-256,
      //       cnonce="366fead6",
      //       nc=00000009,
      //       qop=auth-int,
      //       response="d1249eb66905f97aebaa7e007e539080f184267ab708c69dfdd4f6f2cd1ba4aa"
   }

   // Test SHA512_256 end-to-end challenge and authorization (with qop/auth)
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

      unique_ptr<SipMessage> request(TestSupport::makeMessage(txt.c_str()));

      Data realm = "localhost";
      unique_ptr<SipMessage> challenge(Helper::makeProxyChallenge(*request, realm, true /* useAuth? */, false /* stale? */, std::vector<DigestType>{SHA512_256,SHA256,MD5})); // SHA512_256 first

      cout << "SHA512_256/Auth Challenge:\n" << *challenge << endl;

      assert(challenge->exists(h_ProxyAuthenticates));
      assert(challenge->header(h_ProxyAuthenticates).size() == 3);  // 3 algorithms offered
      assert(challenge->header(h_ProxyAuthenticates).front().exists(p_qopOptions));
      assert(challenge->header(h_ProxyAuthenticates).front().param(p_algorithm) == "SHA-512-256");
      assert(challenge->header(h_ProxyAuthenticates).front().scheme() == "Digest");

      Data username = "bob";
      Data password = "secret";
      Data cnonce = "366fead6";
      unsigned int nc = 9;
      
      Data passwordResipA1 = Helper::createResipA1HashString(username, realm, password);

      Helper::addAuthorization(*request,
                               *challenge,
                               username,
                               password,
                               cnonce,
                               nc);

      assert(nc == 10);

      cout << "SHA512_256/Auth Request with Proxy-Authorization:\n" << *request << endl;

      assert(request->exists(h_ProxyAuthorizations));
      assert(request->header(h_ProxyAuthorizations).size() == 1);
      assert(request->header(h_ProxyAuthorizations).front().exists(p_qop));
      assert(request->header(h_ProxyAuthorizations).front().param(p_nc) == "0000000a");

      const Auth& auth = request->header(h_ProxyAuthorizations).front();
      
      assert(auth.param(p_username) == "bob");
      assert(auth.param(p_uri) == "sip:bob@biloxi.com");
      assert(auth.param(p_algorithm) == "SHA-512-256");

      Helper::AuthResult res = Helper::authenticateRequest(*request, realm, password);
      assert(res == Helper::Authenticated);

      auto respair = Helper::advancedAuthenticateRequest(*request, realm, passwordResipA1);
      assert(respair.first == Helper::Authenticated);

      res = Helper::authenticateRequest(*request, realm, password, 5);
      assert(res == Helper::Authenticated);

      respair = Helper::advancedAuthenticateRequest(*request, realm, passwordResipA1, 5);
      assert(respair.first == Helper::Authenticated);

      // Sleep for 2 seconds, then test ExpiresDelta of 1
      sleep(2);

      res = Helper::authenticateRequest(*request, realm, password, 1); 
      assert(res == Helper::Expired);

      respair = Helper::advancedAuthenticateRequest(*request, realm, passwordResipA1, 1);
      assert(respair.first == Helper::Expired);
   }

   // Test SHA512-256 auth-int - against known test vector
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

      unique_ptr<SipMessage> request(TestSupport::makeMessage(txt.c_str()));

      Data realm = "localhost";
      unique_ptr<SipMessage> challenge(Helper::makeProxyChallenge(*request, realm, true /* useAuth? */, false /* stale? */, std::vector<DigestType>{SHA512_256}));

      // Use fixed nonce for test vector
      challenge->header(h_ProxyAuthenticates).front().param(p_nonce) = "1627567:31f7bffca3fc119082ad0750f496a4f8";
      cout << "SHA512-256/Auth(known response) Challenge:\n" << *challenge << endl;

      assert(challenge->exists(h_ProxyAuthenticates));
      assert(challenge->header(h_ProxyAuthenticates).size() == 1);
      assert(challenge->header(h_ProxyAuthenticates).front().exists(p_qopOptions));
      assert(challenge->header(h_ProxyAuthenticates).front().param(p_algorithm) == "SHA-512-256");
      assert(challenge->header(h_ProxyAuthenticates).front().scheme() == "Digest");

      Data username = "bob";
      Data password = "secret";
      Data cnonce = "366fead6";
      unsigned int nc = 8;
      
      Data passwordResipA1 = Helper::createResipA1HashString(username, realm, password);

      Helper::addAuthorization(*request,
                               *challenge,
                               username,
                               password,
                               cnonce,
                               nc);

      assert(nc == 9);

      cout << "SHA512-256/Auth(known response) Request with Proxy-Authorization:\n" << *request << endl;

      assert(request->exists(h_ProxyAuthorizations));
      assert(request->header(h_ProxyAuthorizations).size() == 1);

      const Auth& auth = request->header(h_ProxyAuthorizations).front();
      
      assert(auth.param(p_username) == username);
      assert(auth.param(p_realm) == realm);
      assert(auth.param(p_nonce) == "1627567:31f7bffca3fc119082ad0750f496a4f8");
      assert(auth.param(p_uri) == "sip:bob@biloxi.com");
      assert(auth.param(p_algorithm) == "SHA-512-256");
      assert(auth.param(p_cnonce) == cnonce);
      assert(auth.param(p_nc) == "00000009");
      assert(auth.param(p_qop) == "auth-int");
      assert(auth.param(p_response) == "ef3b0b1bac042cc5d21822eb8207dae4e73f2b5c0f7157bddb494252294358d1"); // known test vector response

      // Calculations for verification
      // 1. Intermediate Values
      //    HA1: SHA512-256(username:realm:password)
      //      String: bob:localhost:secret
      //      Result: 545d02b1d6cb635d8dc19cd943f37ab9c7f62bc1ff7a1cce7596a18b060be552
      //    H(Entity-Body): The hash of the 150-byte SDP payload.
      //      Result: 3338342e50f7828ab7b6f807df0612737542503e666df37436d101f14c936dcd
      //    HA2: SHA512-256(method:digestURI:H(Entity-Body))
      //      String: INVITE:sip:bob@biloxi.com:3338342e50f7828ab7b6f807df0612737542503e666df37436d101f14c936dcd
      //      Result: 19a6080e8ac22ae7b793c5ccb6e255e7f540879921322b2126e897045e599d20
      // 2. The Final Response Calculation
      //    The auth-int response uses the following concatenation: SHA512-256(HA1:nonce:nc:cnonce:qop:HA2)
      //    Combined String: 545d02b1d6cb635d8dc19cd943f37ab9c7f62bc1ff7a1cce7596a18b060be552:1627567:31f7bffca3fc119082ad0750f496a4f8:00000009:366fead6:auth-int:19a6080e8ac22ae7b793c5ccb6e255e7f540879921322b2126e897045e599d20
      //    Final SHA512-256 Response: ef3b0b1bac042cc5d21822eb8207dae4e73f2b5c0f7157bddb494252294358d1
      // 3. The Resulting Proxy-Authorization Header
      //    Proxy-Authorization: Digest username="bob",
      //       realm="localhost",
      //       nonce="1627567:31f7bffca3fc119082ad0750f496a4f8",
      //       uri="sip:bob@biloxi.com",
      //       algorithm=SHA-512-256,
      //       cnonce="366fead6",
      //       nc=00000009,
      //       qop=auth-int,
      //       response="ef3b0b1bac042cc5d21822eb8207dae4e73f2b5c0f7157bddb494252294358d1"
   }
#endif

   // Test MD5 end-to-end challenge and authorization (with qop/auth) - WWW Auth version
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

      unique_ptr<SipMessage> request(TestSupport::makeMessage(txt.c_str()));

      Data realm = "localhost";
      unique_ptr<SipMessage> challenge(Helper::makeWWWChallenge(*request, realm, true /* useAuth? */));

      cout << "MD5/Auth WWW Challenge:\n" << *challenge << endl;

      assert(!challenge->exists(h_ProxyAuthenticates));
      assert(challenge->exists(h_WWWAuthenticates));
      assert(challenge->header(h_WWWAuthenticates).size() == 1);
      assert(challenge->header(h_WWWAuthenticates).front().exists(p_qopOptions));
      assert(challenge->header(h_WWWAuthenticates).front().param(p_algorithm) == "MD5");
      assert(challenge->header(h_WWWAuthenticates).front().scheme() == "Digest");

      Data username = "bob";
      Data password = "secret";
      Data cnonce = "366fead6";
      unsigned int nc = 9;

      Data passwordResipA1 = Helper::createResipA1HashString(username, realm, password);

      Helper::addAuthorization(*request,
         *challenge,
         username,
         password,
         cnonce,
         nc);

      assert(nc == 10);

      cout << "MD5/Auth Request with Authorization:\n" << *request << endl;

      assert(request->exists(h_Authorizations));
      assert(request->header(h_Authorizations).size() == 1);

      const Auth& auth = request->header(h_Authorizations).front();

      assert(auth.exists(p_qop));
      assert(auth.param(p_nc) == "0000000a");
      assert(auth.param(p_username) == "bob");
      assert(auth.param(p_uri) == "sip:bob@biloxi.com");
      assert(auth.param(p_algorithm) == "MD5");

      // Note: We are testing 3 variants of the authenticateRequest logic below

      Helper::AuthResult res = Helper::authenticateRequest(*request, realm, password);
      assert(res == Helper::Authenticated);
      res = Helper::authenticateRequestWithA1(*request, realm, passwordResipA1);
      assert(res == Helper::Authenticated);
      auto respair = Helper::advancedAuthenticateRequest(*request, realm, passwordResipA1, 0, false /* proxyAuthorization? */);
      assert(respair.first == Helper::Authenticated);

      res = Helper::authenticateRequest(*request, realm, password, 5);
      assert(res == Helper::Authenticated);
      res = Helper::authenticateRequestWithA1(*request, realm, passwordResipA1, 5);
      assert(res == Helper::Authenticated);
      respair = Helper::advancedAuthenticateRequest(*request, realm, passwordResipA1, 5, false /* proxyAuthorization? */);
      assert(respair.first == Helper::Authenticated);

      // Sleep for 2 seconds, then test ExpiresDelta of 1
      sleep(2);

      res = Helper::authenticateRequest(*request, realm, password, 1);
      assert(res == Helper::Expired);
      res = Helper::authenticateRequestWithA1(*request, realm, passwordResipA1, 1);
      assert(res == Helper::Expired);
      respair = Helper::advancedAuthenticateRequest(*request, realm, passwordResipA1, 1, false /* proxyAuthorization? */);
      assert(respair.first == Helper::Expired);
   }

   cerr << "ALL OK" << endl;
   return 0;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
