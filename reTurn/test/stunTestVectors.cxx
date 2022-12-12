// This file implements test vectors from: RFC5769

#include <iostream>
#include <string>
#include <asio.hpp>

#include <rutil/MD5Stream.hxx>

#include "../StunTuple.hxx"
#include "../StunMessage.hxx"
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>

using namespace reTurn;
using namespace std;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

int main(int argc, char* argv[])
{
   StunTuple local(StunTuple::UDP, asio::ip::address::from_string("10.0.0.1"), 5000);
   StunTuple remote(StunTuple::UDP, asio::ip::address::from_string("10.0.0.2"), 5001);

   resip::Log::initialize(resip::Log::Cout, resip::Log::Info, "");

   const unsigned char req[] =
     "\x00\x01\x00\x58"
     "\x21\x12\xa4\x42"
     "\xb7\xe7\xa7\x01\xbc\x34\xd6\x86\xfa\x87\xdf\xae"
     "\x80\x22\x00\x10"
       "STUN test client"
     "\x00\x24\x00\x04"
       "\x6e\x00\x01\xff"
     "\x80\x29\x00\x08"
       "\x93\x2f\xf9\xb1\x51\x26\x3b\x36"
     "\x00\x06\x00\x09"
       "\x65\x76\x74\x6a\x3a\x68\x36\x76\x59\x20\x20\x20"
     "\x00\x08\x00\x14"
       "\x9a\xea\xa7\x0c\xbf\xd8\xcb\x56\x78\x1e\xf2\xb5"
       "\xb2\xd3\xf2\x49\xc1\xb5\x71\xa2"
     "\x80\x28\x00\x04"
       "\xe5\x7a\x3b\xcf";

   StunMessage reqMessage(local, remote, (char*)req, sizeof(req)-1);

   assert(reqMessage.isValid());
   assert(reqMessage.mHasMagicCookie);
   assert(reqMessage.mHasIcePriority);
   assert(reqMessage.mIcePriority == 1845494271);
   assert(reqMessage.mHasIceControlled);
   assert(reqMessage.mIceControlledTieBreaker == 10605970187446795062ULL);
   assert(reqMessage.mHasSoftware);
   assert(*reqMessage.mSoftware == "STUN test client");
   assert(reqMessage.mHasUsername);
   assert(*reqMessage.mUsername == "evtj:h6vY");
   assert(reqMessage.mHasMessageIntegrity);
   assert(reqMessage.checkMessageIntegrity("VOkJxbRl1RmTxUk/WvJxBt"));  // Note:  expect this to fail if you build without OpenSSL
   assert(reqMessage.mHasFingerprint);
   assert(reqMessage.checkFingerprint());  

   const unsigned char respv4[] =
     "\x01\x01\x00\x3c"
     "\x21\x12\xa4\x42"
     "\xb7\xe7\xa7\x01\xbc\x34\xd6\x86\xfa\x87\xdf\xae"
     "\x80\x22\x00\x0b"
       "\x74\x65\x73\x74\x20\x76\x65\x63\x74\x6f\x72\x20"
     "\x00\x20\x00\x08"
       "\x00\x01\xa1\x47\xe1\x12\xa6\x43"
     "\x00\x08\x00\x14"
       "\x2b\x91\xf5\x99\xfd\x9e\x90\xc3\x8c\x74\x89\xf9"
       "\x2a\xf9\xba\x53\xf0\x6b\xe7\xd7"
     "\x80\x28\x00\x04"
       "\xc0\x7d\x4c\x96";

   StunMessage respv4Message(local, remote, (char*)respv4, sizeof(respv4)-1);

   assert(respv4Message.isValid());
   assert(respv4Message.mHasMagicCookie);
   assert(respv4Message.mHasSoftware);
   assert(*respv4Message.mSoftware == "test vector");
   assert(respv4Message.mHasXorMappedAddress);
   assert(respv4Message.mXorMappedAddress.family == StunMessage::IPv4Family);
   assert(respv4Message.mXorMappedAddress.port == 32853);
   assert(respv4Message.mXorMappedAddress.addr.ipv4 == asio::ip::address::from_string("192.0.2.1").to_v4().to_ulong());
   assert(respv4Message.mHasMessageIntegrity);
   assert(respv4Message.checkMessageIntegrity("VOkJxbRl1RmTxUk/WvJxBt"));
   assert(respv4Message.mHasFingerprint);
   assert(respv4Message.checkFingerprint());  

   const unsigned char respv6[] =
     "\x01\x01\x00\x48"
     "\x21\x12\xa4\x42"
     "\xb7\xe7\xa7\x01\xbc\x34\xd6\x86\xfa\x87\xdf\xae"
     "\x80\x22\x00\x0b"
       "\x74\x65\x73\x74\x20\x76\x65\x63\x74\x6f\x72\x20"
     "\x00\x20\x00\x14"
       "\x00\x02\xa1\x47"
       "\x01\x13\xa9\xfa\xa5\xd3\xf1\x79"
       "\xbc\x25\xf4\xb5\xbe\xd2\xb9\xd9"
     "\x00\x08\x00\x14"
       "\xa3\x82\x95\x4e\x4b\xe6\x7b\xf1\x17\x84\xc9\x7c"
       "\x82\x92\xc2\x75\xbf\xe3\xed\x41"
     "\x80\x28\x00\x04"
       "\xc8\xfb\x0b\x4c";

   StunMessage respv6Message(local, remote, (char*)respv6, sizeof(respv6)-1);

   assert(respv6Message.isValid());
   assert(respv6Message.mHasMagicCookie);
   assert(respv6Message.mHasSoftware);
   assert(*respv6Message.mSoftware == "test vector");
   assert(respv6Message.mHasXorMappedAddress);
   assert(respv6Message.mXorMappedAddress.family == StunMessage::IPv6Family);
   assert(respv6Message.mXorMappedAddress.port == 32853);
   asio::ip::address_v6::bytes_type v6addr = asio::ip::address::from_string("2001:db8:1234:5678:11:2233:4455:6677").to_v6().to_bytes();
   assert(memcmp(&respv6Message.mXorMappedAddress.addr.ipv6, v6addr.data(), sizeof(respv6Message.mXorMappedAddress.addr.ipv6)) == 0);
   assert(respv6Message.mHasMessageIntegrity);
   assert(respv6Message.checkMessageIntegrity("VOkJxbRl1RmTxUk/WvJxBt"));
   assert(respv6Message.mHasFingerprint);
   assert(respv6Message.checkFingerprint());  

   const unsigned char reqltc[] =
     "\x00\x01\x00\x60"
     "\x21\x12\xa4\x42"
     "\x78\xad\x34\x33\xc6\xad\x72\xc0\x29\xda\x41\x2e"
     "\x00\x06\x00\x12"
       "\xe3\x83\x9e\xe3\x83\x88\xe3\x83\xaa\xe3\x83\x83"
       "\xe3\x82\xaf\xe3\x82\xb9\x00\x00"
     "\x00\x15\x00\x1c"
       "\x66\x2f\x2f\x34\x39\x39\x6b\x39\x35\x34\x64\x36"
       "\x4f\x4c\x33\x34\x6f\x4c\x39\x46\x53\x54\x76\x79"
       "\x36\x34\x73\x41"
     "\x00\x14\x00\x0b"
       "\x65\x78\x61\x6d\x70\x6c\x65\x2e\x6f\x72\x67\x00"
     "\x00\x08\x00\x14"
       "\xf6\x70\x24\x65\x6d\xd6\x4a\x3e\x02\xb8\xe0\x71"
       "\x2e\x85\xc9\xa2\x8c\xa8\x96\x66";

   StunMessage reqltcMessage(local, remote, (char*)reqltc, sizeof(reqltc)-1);

   // Username:  "<U+30DE><U+30C8><U+30EA><U+30C3><U+30AF><U+30B9>"
   //            (without quotes) unaffected by SASLprep[RFC4013] processing
   char username[] = "\xe3\x83\x9e\xe3\x83\x88\xe3\x83\xaa\xe3\x83\x83\xe3\x82\xaf\xe3\x82\xb9";

   // Password:  "The<U+00AD>M<U+00AA>tr<U+2168>" resp "TheMatrIX" (without
   //            quotes) before resp after SASLprep processing
   char password[] = "TheMatrIX";

   assert(reqltcMessage.isValid());
   assert(reqltcMessage.mHasMagicCookie);
   assert(reqltcMessage.mHasUsername);
   assert(*reqltcMessage.mUsername == username);
   assert(reqltcMessage.mHasRealm);
   assert(*reqltcMessage.mRealm == "example.org");
   assert(reqltcMessage.mHasNonce);
   assert(*reqltcMessage.mNonce == "f//499k954d6OL34oL9FSTvy64sA");
   assert(reqltcMessage.mHasMessageIntegrity);
   resip::Data hmacKey;
   reqltcMessage.calculateHmacKey(hmacKey, password);
   assert(reqltcMessage.checkMessageIntegrity(hmacKey));

   resip::MD5Stream r;
   r << username << ":example.org:" << password;
   resip::Data password_ha1 = r.getBin();
   reqltcMessage.calculateHmacKeyForHa1(hmacKey, password_ha1);
   assert(reqltcMessage.checkMessageIntegrity(hmacKey));  

   InfoLog(<< "All tests passed!");
   return 0;
}


/* ====================================================================

 Copyright (c) 2007-2008, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
