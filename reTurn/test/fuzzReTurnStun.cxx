#include <iostream>
#include <vector>

#include <asio.hpp>

#include <rutil/Data.hxx>
#include <rutil/Logger.hxx>

#include "../StunMessage.hxx"
#include "../StunTuple.hxx"

#include "FuzzStrict.h"

// reTurn has a STUN / TURN parser of its own (reTurn/StunMessage.cxx),
// separate from rutil/stun, which fuzzStun covers.  reTurnServer and the
// reTurn clients (and so reflow and recon) run it on every datagram and
// stream frame they receive.  This harness parses the input the way they do
// and then runs the checks they make on a parsed message: FINGERPRINT,
// MESSAGE-INTEGRITY (and -SHA256), the long-term credential key, and the
// RFC 3489 shared-secret username.  It finishes by encoding the parsed
// message and parsing the result, which covers the encoder too.

using namespace reTurn;

extern "C" int LLVMFuzzerInitialize(int* /*argc*/, char*** argv)
{
   resip::Log::initialize(resip::Log::Cout, resip::Log::None, *argv[0]);
   return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const unsigned char* data, size_t size)
{
   // STUN messages carry a 16-bit length, so nothing larger can be valid.
   if (size > 0xFFFF + 20)
   {
      fuzzRejected("input is larger than any STUN message");
      return 0;
   }

   static const StunTuple local(StunTuple::UDP, asio::ip::make_address("10.0.0.1"), 3478);
   static const StunTuple remote(StunTuple::UDP, asio::ip::make_address("10.0.0.2"), 5000);

   // StunMessage copies the buffer, but takes it as a char*.
   std::vector<char> buf(data, data + size);
   StunMessage msg(local, remote, buf.data(), static_cast<unsigned int>(buf.size()));
   if (!msg.isValid())
   {
      fuzzRejected("StunMessage did not parse");
      return 0;
   }

   msg.checkFingerprint();
   msg.checkMessageIntegrity("VOkJxbRl1RmTxUk/WvJxBt");
   msg.checkMessageIntegritySha256("VOkJxbRl1RmTxUk/WvJxBt");

   // As RequestHandler does for long-term credentials.
   if (msg.mHasUsername && msg.mHasRealm)
   {
      resip::Data hmacKey;
      msg.calculateHmacKey(hmacKey, *msg.mUsername, *msg.mRealm, "password");
      msg.checkMessageIntegrity(hmacKey);
   }

   // As RequestHandler does for an RFC 3489 binding request with a
   // RESPONSE-ADDRESS and a username from a shared-secret response.
   if (msg.mHasResponseAddress && msg.mHasUsername)
   {
      StunTuple tuple;
      msg.getTupleFromUsername(tuple);
   }

   // Encode the parsed message and parse it again.  An encoding is never
   // much larger than the message it was parsed from.
   std::vector<char> out(2 * size + 1024);
   unsigned int outLen = msg.stunEncodeMessage(out.data(), static_cast<unsigned int>(out.size()));
   StunMessage reparsed(local, remote, out.data(), outLen);

   return 0;
}

/* ====================================================================
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 */
