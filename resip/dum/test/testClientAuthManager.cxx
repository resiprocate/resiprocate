// Unit tests for ClientAuthManager's handling of digest challenges (GitHub issue #517):
//  - a challenge carrying several realms answers every realm we can answer, and a realm we can't
//    answer neither asserts nor stays failed for later challenges
//  - a realm challenged with several algorithms answers the topmost usable one, also when it is
//    re-challenged after the first success (the unusable algorithm listed first must not fail the realm)
//  - WWW-Authenticate and Proxy-Authenticate in the same (aggregated) response are both answered
//  - a challenge whose algorithm only a ClientAuthExtension supports reaches the extension's A1 handler,
//    without the credential being checked against the built-in algorithms
//
// ClientAuthManager is driven directly, as DialogSet does:  handle() with the request and the 401/407,
// then addAuthentication() on the retried request, whose decorators add the Authorization headers.

#include "resip/stack/SipMessage.hxx"
#include "resip/dum/ClientAuthExtension.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/UserProfile.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"

#include <iostream>
#include <memory>

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;
using namespace std;

static int failures = 0;

#define CHECK(cond)                                                                    \
   do                                                                                  \
   {                                                                                   \
      if (!(cond))                                                                     \
      {                                                                                \
         cerr << "FAILED: " << #cond << " at " << __FILE__ << ":" << __LINE__ << endl; \
         ++failures;                                                                   \
      }                                                                                \
   } while (false)

static unique_ptr<SipMessage>
makeMessage(const Data& text)
{
   unique_ptr<SipMessage> msg(SipMessage::make(text, false /* isExternal */));
   if (!msg)
   {
      cerr << "Could not parse:" << endl << text << endl;
      exit(1);
   }
   return msg;
}

static unique_ptr<SipMessage>
makeRegister(const Data& callId)
{
   return makeMessage("REGISTER sip:example.invalid SIP/2.0\r\n"
                      "Via: SIP/2.0/UDP 127.0.0.1:5060;branch=z9hG4bK-authtest\r\n"
                      "Max-Forwards: 70\r\n"
                      "To: <sip:alice@example.invalid>\r\n"
                      "From: <sip:alice@example.invalid>;tag=authtest\r\n"
                      "Call-ID: " + callId + "\r\n"
                      "CSeq: 1 REGISTER\r\n"
                      "Contact: <sip:alice@127.0.0.1:5060>\r\n"
                      "Content-Length: 0\r\n"
                      "\r\n");
}

// challengeHeaders: complete WWW-Authenticate / Proxy-Authenticate lines, each ending in \r\n
static unique_ptr<SipMessage>
makeResponse(const Data& callId, int code, const Data& challengeHeaders = Data::Empty)
{
   return makeMessage("SIP/2.0 " + Data(code) + " Test\r\n"
                      "Via: SIP/2.0/UDP 127.0.0.1:5060;branch=z9hG4bK-authtest\r\n"
                      "To: <sip:alice@example.invalid>;tag=server\r\n"
                      "From: <sip:alice@example.invalid>;tag=authtest\r\n"
                      "Call-ID: " + callId + "\r\n"
                      "CSeq: 1 REGISTER\r\n" +
                      challengeHeaders +
                      "Content-Length: 0\r\n"
                      "\r\n");
}

static Data
challenge(const char* header, const char* realm, const char* nonce, const char* algorithm, bool stale = false)
{
   Data result(header);
   result += ": Digest realm=\"";
   result += realm;
   result += "\",nonce=\"";
   result += nonce;
   result += "\",qop=\"auth\"";
   if (algorithm)
   {
      result += ",algorithm=";
      result += algorithm;
   }
   if (stale)
   {
      result += ",stale=true";
   }
   result += "\r\n";
   return result;
}

static Data
www(const char* realm, const char* nonce, const char* algorithm = "MD5", bool stale = false)
{
   return challenge("WWW-Authenticate", realm, nonce, algorithm, stale);
}

static Data
proxy(const char* realm, const char* nonce, const char* algorithm = "MD5", bool stale = false)
{
   return challenge("Proxy-Authenticate", realm, nonce, algorithm, stale);
}

// The request as it would go on the wire after the challenge was handled
static SipMessage
retried(ClientAuthManager& manager, const SipMessage& request)
{
   SipMessage msg(request);
   msg.clearOutboundDecorators();
   manager.addAuthentication(msg);
   msg.callOutboundDecorators(Tuple(), Tuple(), Data::Empty);
   return msg;
}

static const Auth*
findAuth(const SipMessage& msg, bool proxyAuth, const char* realm)
{
   if (proxyAuth ? !msg.exists(h_ProxyAuthorizations) : !msg.exists(h_Authorizations))
   {
      return 0;
   }
   const Auths& auths = proxyAuth ? msg.header(h_ProxyAuthorizations) : msg.header(h_Authorizations);
   for (Auths::const_iterator i = auths.begin(); i != auths.end(); ++i)
   {
      if (i->exists(p_realm) && i->param(p_realm) == realm)
      {
         return &(*i);
      }
   }
   return 0;
}

static size_t
authCount(const SipMessage& msg, bool proxyAuth)
{
   if (proxyAuth)
   {
      return msg.exists(h_ProxyAuthorizations) ? msg.header(h_ProxyAuthorizations).size() : 0;
   }
   return msg.exists(h_Authorizations) ? msg.header(h_Authorizations).size() : 0;
}

static bool
answers(const Auth* auth, const char* nonce, const char* algorithm)
{
   return auth &&
          auth->exists(p_nonce) && auth->param(p_nonce) == nonce &&
          auth->exists(p_algorithm) && isEqualNoCase(auth->param(p_algorithm), algorithm) &&
          auth->exists(p_response) && !auth->param(p_response).empty();
}

static void
testTwoRealmsBothCredentials()
{
   cerr << "testTwoRealmsBothCredentials" << endl;
   const Data callId("two-realms-both");
   UserProfile profile;
   profile.setDigestCredential("a.invalid", "alice", "secretA");
   profile.setDigestCredential("b.invalid", "alice", "secretB");
   ClientAuthManager manager;
   unique_ptr<SipMessage> request = makeRegister(callId);

   CHECK(manager.handle(profile, *request, *makeResponse(callId, 401, www("a.invalid", "na") + www("b.invalid", "nb"))));
   SipMessage msg = retried(manager, *request);
   CHECK(authCount(msg, false) == 2);
   CHECK(answers(findAuth(msg, false, "a.invalid"), "na", "MD5"));
   CHECK(answers(findAuth(msg, false, "b.invalid"), "nb", "MD5"));

   CHECK(!manager.handle(profile, *request, *makeResponse(callId, 200)));  // authSucceeded
}

static void
testTwoRealmsOneUsable()
{
   cerr << "testTwoRealmsOneUsable" << endl;
   // Note:  UserProfile falls back to its first credential for a realm it has none for, so a realm is unusable
   // here because of its algorithm, not a missing credential
   const Data callId("two-realms-one");
   UserProfile profile;
   profile.setDigestCredential("a.invalid", "alice", "secretA");
   profile.setDigestCredential("b.invalid", "alice", "secretB");
   ClientAuthManager manager;
   unique_ptr<SipMessage> request = makeRegister(callId);

   // a.invalid can't be answered:  b.invalid is answered, and a.invalid is left out - it used to be marked
   // Failed, which asserted when the retry was built and again on the 200
   CHECK(manager.handle(profile, *request, *makeResponse(callId, 401, www("a.invalid", "na1", "X-UNSUPPORTED") + www("b.invalid", "nb1"))));
   SipMessage msg = retried(manager, *request);
   CHECK(authCount(msg, false) == 1);
   CHECK(answers(findAuth(msg, false, "b.invalid"), "nb1", "MD5"));
   CHECK(!manager.handle(profile, *request, *makeResponse(callId, 200)));

   // Same with the realm we can answer listed first - the old code's first loop never even looked at
   // a.invalid, but its second still marked it Failed
   const Data callId2("two-realms-one-reversed");
   unique_ptr<SipMessage> request2 = makeRegister(callId2);
   CHECK(manager.handle(profile, *request2, *makeResponse(callId2, 401, www("b.invalid", "nb2") + www("a.invalid", "na2", "X-UNSUPPORTED"))));
   SipMessage msg2 = retried(manager, *request2);
   CHECK(authCount(msg2, false) == 1);
   CHECK(answers(findAuth(msg2, false, "b.invalid"), "nb2", "MD5"));

   // a.invalid is later challenged with an algorithm we support:  it is answered, it isn't stuck as Failed
   CHECK(manager.handle(profile, *request2, *makeResponse(callId2, 401, www("a.invalid", "na3") + www("b.invalid", "nb3", "MD5", true))));
   SipMessage msg3 = retried(manager, *request2);
   CHECK(authCount(msg3, false) == 2);
   CHECK(answers(findAuth(msg3, false, "a.invalid"), "na3", "MD5"));
   CHECK(answers(findAuth(msg3, false, "b.invalid"), "nb3", "MD5"));
   CHECK(!manager.handle(profile, *request2, *makeResponse(callId2, 200)));
}

static void
testUnsupportedAlgorithmListedFirst()
{
   cerr << "testUnsupportedAlgorithmListedFirst" << endl;
   const Data callId("unsupported-first");
   UserProfile profile;
   profile.setDigestCredential("r.invalid", "alice", "secret");
   ClientAuthManager manager;
   unique_ptr<SipMessage> request = makeRegister(callId);

   // RFC 8760 section 2.4:  the topmost header we can use, here the second one
   CHECK(manager.handle(profile, *request, *makeResponse(callId, 401, www("r.invalid", "n1", "X-UNSUPPORTED") + www("r.invalid", "n1", "MD5"))));
   SipMessage msg = retried(manager, *request);
   CHECK(authCount(msg, false) == 1);
   CHECK(answers(findAuth(msg, false, "r.invalid"), "n1", "MD5"));
   CHECK(!manager.handle(profile, *request, *makeResponse(callId, 200)));  // realm is now Cached

   // The next request is challenged again (a refresh with a stale nonce).  The realm is no longer Initial, and
   // the unusable header listed first used to move it to Failed, so the refresh failed.
   CHECK(manager.handle(profile, *request, *makeResponse(callId, 401, www("r.invalid", "n2", "X-UNSUPPORTED", true) + www("r.invalid", "n2", "MD5", true))));
   msg = retried(manager, *request);
   CHECK(authCount(msg, false) == 1);
   CHECK(answers(findAuth(msg, false, "r.invalid"), "n2", "MD5"));

   // Our answer to nonce n2 is rejected outright (same nonce, not stale) - wrong password, give up
   CHECK(!manager.handle(profile, *request, *makeResponse(callId, 401, www("r.invalid", "n2", "X-UNSUPPORTED") + www("r.invalid", "n2", "MD5"))));
}

static void
testA1HashWithoutTheAlgorithm()
{
   cerr << "testA1HashWithoutTheAlgorithm" << endl;
   const Data callId("a1-without-algorithm");
   UserProfile profile;
   // An MD5 A1 hash only - the SHA-256 challenge listed first can't be answered with it (whether or not this
   // build supports SHA-256 at all)
   profile.setDigestCredential("r.invalid", "alice", "[MD5]" + Data("alice:r.invalid:secret").md5(),
                               true /* isPasswordA1Hash */);
   ClientAuthManager manager;
   unique_ptr<SipMessage> request = makeRegister(callId);

   CHECK(manager.handle(profile, *request, *makeResponse(callId, 401, www("r.invalid", "n1", "SHA-256") + www("r.invalid", "n1", "MD5"))));
   SipMessage msg = retried(manager, *request);
   CHECK(answers(findAuth(msg, false, "r.invalid"), "n1", "MD5"));
   CHECK(!manager.handle(profile, *request, *makeResponse(callId, 200)));

   CHECK(manager.handle(profile, *request, *makeResponse(callId, 401, www("r.invalid", "n2", "SHA-256", true) + www("r.invalid", "n2", "MD5", true))));
   msg = retried(manager, *request);
   CHECK(answers(findAuth(msg, false, "r.invalid"), "n2", "MD5"));
}

static void
testWwwAndProxyInOneResponse()
{
   cerr << "testWwwAndProxyInOneResponse" << endl;
   const Data callId("www-and-proxy");
   UserProfile profile;
   profile.setDigestCredential("w.invalid", "alice", "secretW");
   profile.setDigestCredential("p.invalid", "alice", "secretP");
   ClientAuthManager manager;
   unique_ptr<SipMessage> request = makeRegister(callId);

   // A forking proxy aggregates the 401 from one branch and the 407 from another (RFC 3261 section 16.7)
   CHECK(manager.handle(profile, *request, *makeResponse(callId, 407, proxy("p.invalid", "np") + www("w.invalid", "nw"))));
   SipMessage msg = retried(manager, *request);
   CHECK(authCount(msg, false) == 1);
   CHECK(authCount(msg, true) == 1);
   CHECK(answers(findAuth(msg, false, "w.invalid"), "nw", "MD5"));
   CHECK(answers(findAuth(msg, true, "p.invalid"), "np", "MD5"));
}

class TestClientAuthExtension : public ClientAuthExtension
{
   public:
      TestClientAuthExtension(int& a1Calls) : mA1Calls(a1Calls) {}

      bool algorithmAndQopSupported(const Auth& challenge) override
      {
         return challenge.exists(p_algorithm) && isEqualNoCase(challenge.param(p_algorithm), "X-TEST");
      }

      void makeChallengeResponseAuth(const SipMessage& request, const Data& username, const Data& password,
                                     const Auth& challenge, const Data& cnonce, const Data& authQop,
                                     const Data& nonceCountString, Auth& auth) override
      {
         fill(username, challenge, "from-password", auth);
      }

      void makeChallengeResponseAuthWithA1(const SipMessage& request, const Data& username, const Data& passwordHashA1,
                                           const Auth& challenge, const Data& cnonce, const Data& authQop,
                                           const Data& nonceCountString, Auth& auth) override
      {
         ++mA1Calls;
         fill(username, challenge, "from-a1", auth);
      }

   private:
      static void fill(const Data& username, const Auth& challenge, const char* response, Auth& auth)
      {
         auth.scheme() = Symbols::Digest;
         auth.param(p_username) = username;
         auth.param(p_realm) = challenge.param(p_realm);
         auth.param(p_nonce) = challenge.param(p_nonce);
         auth.param(p_algorithm) = challenge.param(p_algorithm);
         auth.param(p_response) = response;
      }

      int& mA1Calls;
};

// Run last:  installs a ClientAuthExtension for the rest of the process
static void
testExtensionAlgorithmWithA1Credential()
{
   cerr << "testExtensionAlgorithmWithA1Credential" << endl;
   int a1Calls = 0;
   ClientAuthExtension::setInstance(unique_ptr<ClientAuthExtension>(new TestClientAuthExtension(a1Calls)));

   const Data callId("extension-a1");
   UserProfile profile;
   // A1 in a form only the extension understands - in particular it has no built-in algorithm hashes in it, so
   // checking it against the built-in algorithms rejects it
   profile.setDigestCredential("x.invalid", "alice", "[X-TEST]0123456789abcdef", true /* isPasswordA1Hash */);
   ClientAuthManager manager;
   unique_ptr<SipMessage> request = makeRegister(callId);

   CHECK(manager.handle(profile, *request, *makeResponse(callId, 401, www("x.invalid", "nx", "X-TEST"))));
   SipMessage msg = retried(manager, *request);
   const Auth* auth = findAuth(msg, false, "x.invalid");
   CHECK(auth && auth->exists(p_response) && auth->param(p_response) == "from-a1");
   CHECK(a1Calls == 1);
}

int
main(int argc, char* argv[])
{
   Log::initialize(Log::Cout, argc > 1 ? Log::toLevel(argv[1]) : Log::Warning, argv[0]);

   testTwoRealmsBothCredentials();
   testTwoRealmsOneUsable();
   testUnsupportedAlgorithmListedFirst();
   testA1HashWithoutTheAlgorithm();
   testWwwAndProxyInOneResponse();
   testExtensionAlgorithmWithA1Credential();

   if (failures)
   {
      cerr << failures << " check(s) FAILED" << endl;
      return 1;
   }
   cerr << "All OK" << endl;
   return 0;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
 * IN EXCESS OF $1,000 NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * ====================================================================
 */
