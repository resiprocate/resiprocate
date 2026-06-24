#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <vector>
#include "rutil/resipfaststreams.hxx"
#include "rutil/Inserter.hxx"
#include "resip/stack/Connection.hxx"
#include "resip/stack/Tuple.hxx"
#ifdef USE_NETNS
#   include "rutil/NetNs.hxx"
#endif

using namespace resip;
using namespace std;

// Builds an AddressMask for the isAddressAllowed tests.
static AddressMask
makeAddressMask(const char* addr, IpVersion ver, short maskBits)
{
   AddressMask m;
   m.mAddress = Tuple(addr, 0 /*port*/, ver, TCP);
   m.mMaskBits = maskBits;
   return m;
}

int
main()
{
   typedef HashMap<Tuple, Connection*> AddrMap;
   //typedef std::map<Tuple, Connection*> AddrMap;

   // Test isSpecialPurposeAddress function - v4
   {
      Tuple testTupleNotSpecial("1.1.1.1", 5069, V4, TCP);
      assert(!testTupleNotSpecial.isSpecialPurposeAddress());

      Tuple testTuple1("10.0.0.1", 5069, V4, TCP);
      assert(testTuple1.isSpecialPurposeAddress());
      Tuple testTuple2("172.16.5.20", 5069, V4, TCP);
      assert(testTuple2.isSpecialPurposeAddress());
      Tuple testTuple3("192.168.0.55", 5069, V4, TCP);
      assert(testTuple3.isSpecialPurposeAddress());
      Tuple testTuple4("127.0.0.1", 5069, V4, TCP);
      assert(testTuple4.isSpecialPurposeAddress());
      Tuple testTuple5("169.254.10.20", 5069, V4, TCP);
      assert(testTuple5.isSpecialPurposeAddress());
      Tuple testTuple6("100.64.10.1", 5069, V4, TCP);
      assert(testTuple6.isSpecialPurposeAddress());
      Tuple testTuple7("198.18.0.5", 5069, V4, TCP);
      assert(testTuple7.isSpecialPurposeAddress());
      Tuple testTuple8("192.0.2.10", 5069, V4, TCP);
      assert(testTuple8.isSpecialPurposeAddress());
      Tuple testTuple9("198.51.100.20", 5069, V4, TCP);
      assert(testTuple9.isSpecialPurposeAddress());
      Tuple testTuple10("203.0.113.5", 5069, V4, TCP);
      assert(testTuple10.isSpecialPurposeAddress());
      Tuple testTuple11("224.0.0.1", 5069, V4, TCP);
      assert(testTuple11.isSpecialPurposeAddress());
      Tuple testTuple12("239.255.255.250", 5069, V4, TCP);
      assert(testTuple12.isSpecialPurposeAddress());
      Tuple testTuple13("240.0.0.1", 5069, V4, TCP);
      assert(testTuple13.isSpecialPurposeAddress());
   }

#ifdef USE_IPV6
   // Test isSpecialPurposeAddress function - v6
   {
      Tuple testTupleV6NotSpecial("2001:4860:4860::8888", 5069, V6, TCP);
      assert(!testTupleV6NotSpecial.isSpecialPurposeAddress());

      Tuple testTupleV61("::1", 5069, V6, TCP);
      assert(testTupleV61.isSpecialPurposeAddress());
      Tuple testTupleV62("::", 5069, V6, TCP);
      assert(testTupleV62.isSpecialPurposeAddress());
      Tuple testTupleV63("fe80::1", 5069, V6, TCP);
      assert(testTupleV63.isSpecialPurposeAddress());
      Tuple testTupleV64("fc00::1", 5069, V6, TCP);
      assert(testTupleV64.isSpecialPurposeAddress());
      Tuple testTupleV65("fd12:3456:789a::1", 5069, V6, TCP);
      assert(testTupleV65.isSpecialPurposeAddress());
      Tuple testTupleV66("ff02::1", 5069, V6, TCP);
      assert(testTupleV66.isSpecialPurposeAddress());
      Tuple testTupleV67("ff05::2", 5069, V6, TCP);
      assert(testTupleV67.isSpecialPurposeAddress());
      Tuple testTupleV68("2001:db8::1", 5069, V6, TCP);
      assert(testTupleV68.isSpecialPurposeAddress());
      Tuple testTupleV69("2001:2::1", 5069, V6, TCP);
      assert(testTupleV69.isSpecialPurposeAddress());
      Tuple testTupleV70("64:ff9b::1", 5069, V6, TCP);
      assert(testTupleV70.isSpecialPurposeAddress());
      Tuple testTupleV71("100::1", 5069, V6, TCP);
      assert(testTupleV71.isSpecialPurposeAddress());
      Tuple testTupleV72("::ffff:192.168.1.1", 5069, V6, TCP);
      assert(testTupleV72.isSpecialPurposeAddress());
   }
#endif

   // Test isPrivateAddress function - v4
   {
      Tuple testTuple("192.168.1.106", 5069, V4, TCP);
      assert(testTuple.isPrivateAddress());
      Tuple testTuple2("10.0.0.5", 5069, V4, TCP);
      assert(testTuple2.isPrivateAddress());
      Tuple testTuple3("172.16.10.5", 5069, V4, TCP);
      assert(testTuple3.isPrivateAddress());
      Tuple testTuple4("150.1.1.106", 5069, V4, TCP);
      assert(!testTuple4.isPrivateAddress());
      Tuple testTuple5("127.0.0.1", 5069, V4, TCP);
      assert(testTuple5.isPrivateAddress());
   }

#ifdef USE_IPV6
   // Test isPrivateAddress function - v6
   {
      Tuple testTuple("fd00::1", 5069, V6, TCP);
      assert(testTuple.isPrivateAddress());
      Tuple testTuple1("fd9b:70f6:0a18:31cc::1", 5069, V6, TCP);
      assert(testTuple1.isPrivateAddress());
      Tuple testTuple2("fe80::1", 5069, V6, TCP);          // Link Local ?slg? do we want this to return private or not?  For now we do only RFC 4193
      assert(!testTuple2.isPrivateAddress());
      Tuple testTuple3("::ffff:10.0.0.5", 5069, V6, TCP);  // V4 mapped
      assert(!testTuple3.isPrivateAddress());
      Tuple testTuple4("::ffff:150.1.1.106", 5069, V6, TCP); // ?slg? do we want this to return private or not?    For now we do only RFC 4193
      assert(!testTuple4.isPrivateAddress());
      Tuple testTuple5("::10.0.0.5", 5069, V6, TCP);       // V4 compatible
      assert(!testTuple5.isPrivateAddress());
      Tuple testTuple6("::150.1.1.106", 5069, V6, TCP);   // ?slg? do we want this to return private or not?  For now we do only RFC 4193
      assert(!testTuple6.isPrivateAddress());
      Tuple testTuple7("2000:1::203:baff:fe30:1176", 5069, V6, TCP);
      assert(!testTuple7.isPrivateAddress());
   }
#endif

   {
      Tuple testTuple("192.168.1.106", 5069, V4, TCP);
      Data binaryToken;
      Tuple::writeBinaryToken(testTuple, binaryToken);
      Data binaryTokenWithSalt;
      Tuple::writeBinaryToken(testTuple, binaryTokenWithSalt, "salt");
      Tuple madeTestTuple = Tuple::makeTupleFromBinaryToken(binaryToken);
      Tuple madeTestTupleWithSalt = Tuple::makeTupleFromBinaryToken(binaryTokenWithSalt, "salt");
      Tuple madeTestTupleWithBadSalt = Tuple::makeTupleFromBinaryToken(binaryTokenWithSalt, "badsalt");
      assert(testTuple == madeTestTuple);
      assert(testTuple.onlyUseExistingConnection == madeTestTuple.onlyUseExistingConnection);
      assert(testTuple.mFlowKey == madeTestTuple.mFlowKey);
      assert(testTuple == madeTestTupleWithSalt);
      assert(testTuple.onlyUseExistingConnection == madeTestTupleWithSalt.onlyUseExistingConnection);
      assert(testTuple.mFlowKey == madeTestTupleWithSalt.mFlowKey);
      assert(madeTestTupleWithBadSalt == Tuple());
   }

#ifdef USE_IPV6
   {
      Tuple testTuple("2000:1::203:baff:fe30:1176", 5069, V6, TCP);
      Data binaryToken;
      Tuple::writeBinaryToken(testTuple, binaryToken);
      Data binaryTokenWithSalt;
      Tuple::writeBinaryToken(testTuple, binaryTokenWithSalt, "salt");
      Tuple madeTestTuple = Tuple::makeTupleFromBinaryToken(binaryToken);
      Tuple madeTestTupleWithSalt = Tuple::makeTupleFromBinaryToken(binaryTokenWithSalt, "salt");
      Tuple madeTestTupleWithBadSalt = Tuple::makeTupleFromBinaryToken(binaryTokenWithSalt, "badsalt");
      assert(testTuple == madeTestTuple);
      assert(testTuple.onlyUseExistingConnection == madeTestTuple.onlyUseExistingConnection);
      assert(testTuple.mFlowKey == madeTestTuple.mFlowKey);
      assert(testTuple == madeTestTupleWithSalt);
      assert(testTuple.onlyUseExistingConnection == madeTestTupleWithSalt.onlyUseExistingConnection);
      assert(testTuple.mFlowKey == madeTestTupleWithSalt.mFlowKey);
      assert(madeTestTupleWithBadSalt == Tuple());
   }
#endif

#ifdef USE_IPV6
   {
      AddrMap mMap;
      Tuple t("2000:1::203:baff:fe30:1176", 5100, V6, TCP);
      Tuple s = t;
      assert(s == t);
      assert(s.hash() == t.hash());
      mMap[t] = 0;
      resipCerr << mMap.count(t) << endl;
      resipCerr << Inserter(mMap) << std::endl;
      
      assert(mMap.count(t) == 1);
   }
#endif

   {
      Tuple t1("192.168.1.2", 2060, UDP);
      Tuple t2("192.168.1.2", 2060, UDP);
      Tuple t3("192.168.1.3", 2060, UDP);
      Tuple t4("192.1.2.3", 2061, UDP);
      Tuple t5("192.168.1.2", 2060, TCP);
      Tuple t6("192.168.1.2", 2061, UDP);
      Tuple loopback("127.0.0.1",2062,TCP);

      assert(t1.isEqual(t2, false /* ignorePort? */, false /* ignoreTransport? */));
      assert(t1.isEqual(t2, true /* ignorePort? */, true /* ignoreTransport? */));
      assert(t1.isEqual(t5, false /* ignorePort? */, true /* ignoreTransport? */));
      assert(t1.isEqual(t5, true /* ignorePort? */, true /* ignoreTransport? */));
      assert(t1.isEqual(t6, true /* ignorePort? */, false /* ignoreTransport? */));
      assert(t1.isEqual(t6, true /* ignorePort? */, true /* ignoreTransport? */));
      assert(!t1.isEqual(t3, true /* ignorePort? */, true /* ignoreTransport? */));

      assert(t1.isEqualWithMask(t2, 32, false /* ignorePort? */));
      assert(!t1.isEqualWithMask(t3, 32, false));  // address is different
      assert(t1.isEqualWithMask(t3, 24, false));
      assert(t1.isEqualWithMask(t4, 8, true));
      assert(!t1.isEqualWithMask(t5, 8, true));    // transport type is different
      assert(!t1.isEqualWithMask(t6, 8, false));   // port is different
      assert(loopback.isLoopback());
      assert(!t1.isLoopback());
      
      resip::Data token1;
      resip::Data token2;
      resip::Data token3;
      resip::Data token4;
      resip::Data token5;
      resip::Data token6;
      resip::Data tokenloopback;
      
      Tuple::writeBinaryToken(t1,token1);
      Tuple::writeBinaryToken(t2,token2);
      Tuple::writeBinaryToken(t3,token3);
      Tuple::writeBinaryToken(t4,token4);
      Tuple::writeBinaryToken(t5,token5);
      Tuple::writeBinaryToken(t6,token6);
      Tuple::writeBinaryToken(loopback,tokenloopback);
            
      Tuple t1prime=Tuple::makeTupleFromBinaryToken(token1);
      Tuple t2prime=Tuple::makeTupleFromBinaryToken(token2);
      Tuple t3prime=Tuple::makeTupleFromBinaryToken(token3);
      Tuple t4prime=Tuple::makeTupleFromBinaryToken(token4);
      Tuple t5prime=Tuple::makeTupleFromBinaryToken(token5);
      Tuple t6prime=Tuple::makeTupleFromBinaryToken(token6);
      Tuple loopbackprime=Tuple::makeTupleFromBinaryToken(tokenloopback);
      
      assert(t1==t1prime);
      assert(t2==t2prime);
      assert(t3==t3prime);
      assert(t4==t4prime);
      assert(t5==t5prime);
      assert(t6==t6prime);
      assert(loopback==loopbackprime);
      assert(t1.onlyUseExistingConnection == t1prime.onlyUseExistingConnection);
      assert(t2.onlyUseExistingConnection == t2prime.onlyUseExistingConnection);
      assert(t3.onlyUseExistingConnection == t3prime.onlyUseExistingConnection);
      assert(t4.onlyUseExistingConnection == t4prime.onlyUseExistingConnection);
      assert(t5.onlyUseExistingConnection == t5prime.onlyUseExistingConnection);
      assert(t6.onlyUseExistingConnection == t6prime.onlyUseExistingConnection);
      assert(loopback.onlyUseExistingConnection == loopbackprime.onlyUseExistingConnection);
      assert(t1.mFlowKey == t1prime.mFlowKey);
      assert(t2.mFlowKey == t2prime.mFlowKey);
      assert(t3.mFlowKey == t3prime.mFlowKey);
      assert(t4.mFlowKey == t4prime.mFlowKey);
      assert(t5.mFlowKey == t5prime.mFlowKey);
      assert(t6.mFlowKey == t6prime.mFlowKey);
      assert(loopback.mFlowKey == loopbackprime.mFlowKey);
   }

   // Test isAddressAllowed - address ACL matching (deny wins; a non-empty allow list requires a
   // match; port and transport are ignored; address family must match for an entry to apply).
   {
      std::vector<AddressMask> none;

      // No ACL -> allowed
      assert(Tuple("203.0.113.7", 0, V4, TCP).isAddressAllowed(none, none));

      // Deny wins (CIDR)
      std::vector<AddressMask> denyOnly;
      denyOnly.push_back(makeAddressMask("192.0.2.0", V4, 24));
      assert(!Tuple("192.0.2.55", 0, V4, TCP).isAddressAllowed(none, denyOnly));
      assert(Tuple("203.0.113.7", 0, V4, TCP).isAddressAllowed(none, denyOnly));

      // Allow list restricts: must match
      std::vector<AddressMask> allowOnly;
      allowOnly.push_back(makeAddressMask("198.51.100.0", V4, 24));
      assert(Tuple("198.51.100.9", 0, V4, TCP).isAddressAllowed(allowOnly, none));
      assert(!Tuple("203.0.113.7", 0, V4, TCP).isAddressAllowed(allowOnly, none));

      // Deny takes precedence over allow (allow a /24, deny a /32 inside it)
      std::vector<AddressMask> allowBroad;
      allowBroad.push_back(makeAddressMask("198.51.100.0", V4, 24));
      std::vector<AddressMask> denyNarrow;
      denyNarrow.push_back(makeAddressMask("198.51.100.9", V4, 32));
      assert(!Tuple("198.51.100.9", 0, V4, TCP).isAddressAllowed(allowBroad, denyNarrow));
      assert(Tuple("198.51.100.10", 0, V4, TCP).isAddressAllowed(allowBroad, denyNarrow));
   }

#ifdef USE_IPV6
   {
      std::vector<AddressMask> none;

      // IPv6 matching
      std::vector<AddressMask> allowV6;
      allowV6.push_back(makeAddressMask("2001:db8::", V6, 32));
      assert(Tuple("2001:db8::1234", 0, V6, TCP).isAddressAllowed(allowV6, none));
      assert(!Tuple("2001:dead::1", 0, V6, TCP).isAddressAllowed(allowV6, none));

      // Family isolation: a v4 address is not matched by a v6 allow entry -> not allowed
      assert(!Tuple("203.0.113.7", 0, V4, TCP).isAddressAllowed(allowV6, none));
   }
#endif

#ifdef USE_IPV6
   {
      Tuple t1("2000:1::203:baff:fe30:1176", 2060, UDP);
      Tuple t2("2000:1::203:baff:fe30:1176", 2060, UDP);
      Tuple t3("2000:1::203:1111:2222:3333", 2060, UDP);
      Tuple t4("2000:1::0000:1111:2222:3333", 2061, UDP);
      Tuple t5("2000:1::204:1111:2222:3333", 2061, TCP);
      Tuple t6("2000:1::203:baff:fe30:1177", 2060, UDP);
      Tuple t7("2000:1::203:baff:fe30:1176", 2061, TCP);
      Tuple t8("2000:1::203:baff:fe30:1176", 2061, UDP);
      Tuple loopback("::1",2062,TCP);
      
      assert(t1.isEqual(t2, false /* ignorePort? */, false /* ignoreTransport? */));
      assert(t1.isEqual(t2, true /* ignorePort? */, true /* ignoreTransport? */));
      assert(t1.isEqual(t7, true /* ignorePort? */, true /* ignoreTransport? */));
      assert(t1.isEqual(t8, true /* ignorePort? */, false /* ignoreTransport? */));

      assert(t1.isEqualWithMask(t2, 128, false /* ignorePort? */));
      assert(t1.isEqualWithMask(t3, 80, false));
      assert(t1.isEqualWithMask(t4, 64, true));
      assert(!t1.isEqualWithMask(t5, 64, true));    // transport type is different
      assert(t1.isEqualWithMask(t6, 120, false)); 
      assert(loopback.isLoopback());
      assert(!t1.isLoopback());
      
      resip::Data token1;
      resip::Data token2;
      resip::Data token3;
      resip::Data token4;
      resip::Data token5;
      resip::Data token6;
      resip::Data tokenloopback;
      
      Tuple::writeBinaryToken(t1,token1);
      Tuple::writeBinaryToken(t2,token2);
      Tuple::writeBinaryToken(t3,token3);
      Tuple::writeBinaryToken(t4,token4);
      Tuple::writeBinaryToken(t5,token5);
      Tuple::writeBinaryToken(t6,token6);
      Tuple::writeBinaryToken(loopback,tokenloopback);
            
      Tuple t1prime=Tuple::makeTupleFromBinaryToken(token1);
      Tuple t2prime=Tuple::makeTupleFromBinaryToken(token2);
      Tuple t3prime=Tuple::makeTupleFromBinaryToken(token3);
      Tuple t4prime=Tuple::makeTupleFromBinaryToken(token4);
      Tuple t5prime=Tuple::makeTupleFromBinaryToken(token5);
      Tuple t6prime=Tuple::makeTupleFromBinaryToken(token6);
      Tuple loopbackprime=Tuple::makeTupleFromBinaryToken(tokenloopback);
      
      assert(t1==t1prime);
      assert(t2==t2prime);
      assert(t3==t3prime);
      assert(t4==t4prime);
      assert(t5==t5prime);
      assert(t6==t6prime);
      assert(loopback==loopbackprime);
      assert(t1.onlyUseExistingConnection == t1prime.onlyUseExistingConnection);
      assert(t2.onlyUseExistingConnection == t2prime.onlyUseExistingConnection);
      assert(t3.onlyUseExistingConnection == t3prime.onlyUseExistingConnection);
      assert(t4.onlyUseExistingConnection == t4prime.onlyUseExistingConnection);
      assert(t5.onlyUseExistingConnection == t5prime.onlyUseExistingConnection);
      assert(t6.onlyUseExistingConnection == t6prime.onlyUseExistingConnection);
      assert(loopback.onlyUseExistingConnection == loopbackprime.onlyUseExistingConnection);
      assert(t1.mFlowKey == t1prime.mFlowKey);
      assert(t2.mFlowKey == t2prime.mFlowKey);
      assert(t3.mFlowKey == t3prime.mFlowKey);
      assert(t4.mFlowKey == t4prime.mFlowKey);
      assert(t5.mFlowKey == t5prime.mFlowKey);
      assert(t6.mFlowKey == t6prime.mFlowKey);
      assert(loopback.mFlowKey == loopbackprime.mFlowKey);
   }
#endif

#ifdef USE_NETNS
   {
      Tuple testNetNsTuple("192.168.1.106", 5069, V4, TCP, Data::Empty, "namespace1");
      assert(testNetNsTuple.getNetNs() == "namespace1");
      // Check assignment copies netns
      Tuple netNsTupleCopy = testNetNsTuple;
      assert(netNsTupleCopy.getNetNs() == "namespace1");
      assert(testNetNsTuple == netNsTupleCopy);
      assert(!(testNetNsTuple < netNsTupleCopy));
      assert(!(netNsTupleCopy < testNetNsTuple));
      assert(!(Tuple::AnyPortCompare().operator()(testNetNsTuple, netNsTupleCopy)));
      assert(!(Tuple::AnyPortCompare().operator()(netNsTupleCopy, testNetNsTuple)));

      netNsTupleCopy.setNetNs("namespace2");
      assert(netNsTupleCopy.getNetNs() == "namespace2");
      assert(!(testNetNsTuple == netNsTupleCopy));
      assert(testNetNsTuple < netNsTupleCopy);
      assert(!(netNsTupleCopy < testNetNsTuple));
      assert((Tuple::AnyPortCompare().operator()(testNetNsTuple, netNsTupleCopy)));
      assert(!(Tuple::AnyPortCompare().operator()(netNsTupleCopy, testNetNsTuple)));

      Tuple copyTuple2(netNsTupleCopy);
      assert(copyTuple2.getNetNs() == "namespace2");

      Data binaryToken;
      // NetNs keeps a dictionary of netns.  If we have not yet used a
      // netns, its not in the dictionary.  So we need to prime the dictionary
      // here.
      NetNs::setNs("namespace1");
      NetNs::setNs("");
      //cout << "testNetNsTuple: " << testNetNsTuple << endl;
      Tuple::writeBinaryToken(testNetNsTuple, binaryToken);
      Tuple reconstructed = Tuple::makeTupleFromBinaryToken(binaryToken);
      //cout << "reconstructed: " << reconstructed<< endl;
      assert(reconstructed.getNetNs() == "namespace1");
      assert(reconstructed == testNetNsTuple);
      Data binaryTokenWithSalt;
      Tuple::writeBinaryToken(testNetNsTuple, binaryTokenWithSalt, "iLikePeperToo");
      Tuple reconstructedWithSalt = Tuple::makeTupleFromBinaryToken(binaryTokenWithSalt, "iLikePeperToo");
      //cout << "reconstructedWithSalt: " << reconstructedWithSalt << endl;
      assert(reconstructedWithSalt.getNetNs() == "namespace1");
      assert(reconstructedWithSalt == testNetNsTuple);

      resipCerr << "NETNS OK and tested" << std::endl;
   }
#endif

   resipCerr << "ALL OK" << std::endl;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
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
 */
