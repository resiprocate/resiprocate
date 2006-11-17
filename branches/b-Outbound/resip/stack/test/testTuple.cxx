#include <iostream>
#include "rutil/Inserter.hxx"
#include "resip/stack/Connection.hxx"
#include "resip/stack/Tuple.hxx"

using namespace resip;
using namespace std;

int
main()
{
   typedef HashMap<Tuple, Connection*> AddrMap;
   //typedef std::map<Tuple, Connection*> AddrMap;

#ifdef USE_IPV6
   {
      AddrMap mMap;
      Tuple t("2000:1::203:baff:fe30:1176", 5100, V6, TCP);
      Tuple s = t;
      assert(s == t);
      assert(s.hash() == t.hash());
      mMap[t] = 0;
      std::cerr << mMap.count(t) << endl;
      std::cerr << Inserter(mMap) << std::endl;
      
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
      
      Tuple::binaryToken(t1,token1);
      Tuple::binaryToken(t2,token2);
      Tuple::binaryToken(t3,token3);
      Tuple::binaryToken(t4,token4);
      Tuple::binaryToken(t5,token5);
      Tuple::binaryToken(t6,token6);
      Tuple::binaryToken(loopback,tokenloopback);
            
      Tuple t1prime=Tuple::makeTuple(token1);
      Tuple t2prime=Tuple::makeTuple(token2);
      Tuple t3prime=Tuple::makeTuple(token3);
      Tuple t4prime=Tuple::makeTuple(token4);
      Tuple t5prime=Tuple::makeTuple(token5);
      Tuple t6prime=Tuple::makeTuple(token6);
      Tuple loopbackprime=Tuple::makeTuple(tokenloopback);
      
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

#ifdef USE_IPV6
   {
      Tuple t1("2000:1::203:baff:fe30:1176", 2060, UDP);
      Tuple t2("2000:1::203:baff:fe30:1176", 2060, UDP);
      Tuple t3("2000:1::203:1111:2222:3333", 2060, UDP);
      Tuple t4("2000:1::0000:1111:2222:3333", 2061, UDP);
      Tuple t5("2000:1::204:1111:2222:3333", 2061, TCP);
      Tuple t6("2000:1::203:baff:fe30:1177", 2060, UDP);
      Tuple loopback("::1",2062,TCP);
      
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
      
      Tuple::binaryToken(t1,token1);
      Tuple::binaryToken(t2,token2);
      Tuple::binaryToken(t3,token3);
      Tuple::binaryToken(t4,token4);
      Tuple::binaryToken(t5,token5);
      Tuple::binaryToken(t6,token6);
      Tuple::binaryToken(loopback,tokenloopback);
            
      Tuple t1prime=Tuple::makeTuple(token1);
      Tuple t2prime=Tuple::makeTuple(token2);
      Tuple t3prime=Tuple::makeTuple(token3);
      Tuple t4prime=Tuple::makeTuple(token4);
      Tuple t5prime=Tuple::makeTuple(token5);
      Tuple t6prime=Tuple::makeTuple(token6);
      Tuple loopbackprime=Tuple::makeTuple(tokenloopback);
      
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

   {
      
   }

   std::cerr << "ALL OK" << std::endl;
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
