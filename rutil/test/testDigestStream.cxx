// For testing - also uncomment this in DigestStream.cxx to test non-SSL MD5 and SHA1
//#undef USE_SSL

#include "rutil/Data.hxx"
#include "rutil/DigestStream.hxx"
#include "rutil/Log.hxx"
#include "assert.h"

#include "rutil/MD5Stream.hxx"
#ifdef USE_SSL
#include "rutil/ssl/SHA1Stream.hxx"
#endif

// Disable warnings about using deprecated MD5Stream and SHA1Stream in these tests
#pragma warning(disable : 4996)

using namespace resip;
using namespace std;

void testDigestStream(DigestBuffer::DigestType digestType, const Data& input, const Data& expectedResult)
{
   DigestStream str(digestType);
   if (input.size() > 0)
   {
      str << input;
   }
   assert(str.getHex() == expectedResult);

   if (digestType == DigestBuffer::MD5)
   {
      // Validate result matches MD5Stream implementation
      MD5Stream md5str;
      if (input.size() > 0)
      {
         md5str << input;
      }
      assert(md5str.getHex() == str.getHex());
   }
#ifdef USE_SSL
   else if (digestType == DigestBuffer::SHA1)
   {
      // Validate result matches SHA1Stream implementation
      SHA1Stream sha1str;
      if (input.size() > 0)
      {
         sha1str << input;
      }
      assert(sha1str.getHex() == str.getHex());
   }
#endif
}

int
main()
{
   // Test vectors generated using openssl command line tool, for example:
   // echo -n "abc" | openssl dgst -sha512-256
   // This site also has a nice tool, even though it doesn't do SHA512/256: https://www.browserling.com/tools/all-hashes
   
   // Test empty streams
   Data input = Data::Empty;
   testDigestStream(DigestBuffer::MD5, input, Data("d41d8cd98f00b204e9800998ecf8427e"));
   testDigestStream(DigestBuffer::SHA1, input, Data("da39a3ee5e6b4b0d3255bfef95601890afd80709"));
#ifdef USE_SSL
   testDigestStream(DigestBuffer::SHA256, input, Data("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
   testDigestStream(DigestBuffer::SHA512, input, Data("cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"));
   testDigestStream(DigestBuffer::SHA512_256, input, Data("c672b8d1ef56ed28ab87c3622c5114069bdd3ad7b8f9737498d0c01ecef0967a"));
#endif

   // Test "abc" input
   input = "abc";
   testDigestStream(DigestBuffer::MD5, input, Data("900150983cd24fb0d6963f7d28e17f72"));
   testDigestStream(DigestBuffer::SHA1, input, Data("a9993e364706816aba3e25717850c26c9cd0d89d"));
#ifdef USE_SSL
   testDigestStream(DigestBuffer::SHA256, input, Data("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
   testDigestStream(DigestBuffer::SHA512, input, Data("ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f"));
   testDigestStream(DigestBuffer::SHA512_256, input, Data("53048e2681941ef99b2e29b76b4c7dabe4c2d0c634fc6d46e0e2f13107e7af23"));
#endif

   // Test long input (112 bytes)
   input = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
   testDigestStream(DigestBuffer::MD5, input, Data("03dd8807a93175fb062dfb55dc7d359c"));
   testDigestStream(DigestBuffer::SHA1, input, Data("a49b2446a02c645bf419f995b67091253a04a259"));
#ifdef USE_SSL
   testDigestStream(DigestBuffer::SHA256, input, Data("cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1"));
   testDigestStream(DigestBuffer::SHA512, input, Data("8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb6889018501d289e4900f7e4331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909"));
   testDigestStream(DigestBuffer::SHA512_256, input, Data("3928e184fb8690f840da3988121d31be65cb9d3ef83ee6146feac861e19b563a"));
#endif

   // Test SHA1 blob
   {
      Data input("sip:alice@atlanta.example.com"
                 ":a84b4c76e66710"
                 ":314159 INVITE"
                 ":Thu, 21 Feb 2002 13:02:03 GMT"
                 ":sip:alice@pc33.atlanta.example.com"
                 ":v=0\r\n"
                 "o=UserA 2890844526 2890844526 IN IP4 pc33.atlanta.example.com\r\n"
                 "s=Session SDP\r\n"
                 "c=IN IP4 pc33.atlanta.example.com\r\n"
                 "t=0 0\r\n"
                 "m=audio 49172 RTP/AVP 0\r\n"
                 "a=rtpmap:0 PCMU/8000\r\n");
      testDigestStream(DigestBuffer::SHA1, input, "f2eec616bd43c75fd1cd300691e57301b52b3ad3");
   }

   cerr << "All OK" << endl;

   return 0;
}
/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
 * Copyright (c) 2005 Vovida Networks, Inc.  All rights reserved.
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
