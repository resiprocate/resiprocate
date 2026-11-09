#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "rutil/stun/Stun.hxx"

using namespace resip;

namespace
{
   void put16(std::vector<char>& b, uint16_t v)
   {
      b.push_back(static_cast<char>(v >> 8));
      b.push_back(static_cast<char>(v & 0xff));
   }

   // STUN header (type, length, magic cookie + 12 byte transaction id)
   std::vector<char> header(uint16_t type, uint16_t length)
   {
      std::vector<char> b;
      put16(b, type);
      put16(b, length);
      const char cookieAndId[16] = { 0x21, 0x12, (char)0xa4, 0x42, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
      b.insert(b.end(), cookieAndId, cookieAndId + sizeof(cookieAndId));
      return b;
   }

   void addAttr(std::vector<char>& b, uint16_t type, const char* value, uint16_t len)
   {
      put16(b, type);
      put16(b, len);
      b.insert(b.end(), value, value + len);
      while (len++ % 4) b.push_back(0);
   }
}

int
main(int argc, char* argv[])
{
   // Repeated TURN DATA attributes: the first is kept and the rest ignored,
   // the same as for REALM and NONCE (and nothing may be leaked).
   {
      std::vector<char> b = header(TurnSendRequest, 16);
      addAttr(b, TurnData, "AAAA", 4);
      addAttr(b, TurnData, "BBBB", 4);

      StunMessage msg;
      assert(stunParseMessage(b.data(), (unsigned int)b.size(), msg, false));
      assert(msg.hasTurnData);
      assert(*msg.turnData == "AAAA");
   }

   // Repeated REALM / NONCE: first one wins.
   {
      std::vector<char> b = header(BindRequestMsg, 32);
      addAttr(b, Realm, "one", 3);
      addAttr(b, Realm, "two", 3);
      addAttr(b, Nonce, "n1", 2);
      addAttr(b, Nonce, "n2", 2);

      StunMessage msg;
      assert(stunParseMessage(b.data(), (unsigned int)b.size(), msg, false));
      assert(msg.hasRealm && *msg.realm == "one");
      assert(msg.hasNonce && *msg.nonce == "n1");
   }

   // Length field that does not match the datagram is rejected.
   {
      std::vector<char> b = header(BindRequestMsg, 8);
      addAttr(b, TurnData, "AAAA", 4);
      b.push_back(0);

      StunMessage msg;
      assert(!stunParseMessage(b.data(), (unsigned int)b.size(), msg, false));
   }

   std::cerr << "All OK" << std::endl;
   return 0;
}

/* ====================================================================

 Copyright (c) 2026, SIP Spectrum, Inc. https://www.sipspectrum.com
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of Plantronics nor the names of its contributors
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