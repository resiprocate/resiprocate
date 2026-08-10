#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

#include "rutil/Data.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/WsFrameExtractor.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

static int sFailures = 0;

#define CHECK(expr)                                                     \
   do                                                                   \
   {                                                                    \
      if(!(expr))                                                       \
      {                                                                 \
         cerr << "FAILED " << __FILE__ << ":" << __LINE__               \
              << ": " #expr << endl;                                    \
         sFailures++;                                                   \
      }                                                                 \
   } while(false)

typedef vector<uint8_t> Wire;

static const uint8_t OpContinuation = 0x00;
static const uint8_t OpText = 0x01;

// The extractor bounds the number of fragments a single message may be
// split into; this must match WsFrameExtractor::mMaxFrames, which is
// private, so it is repeated here.
static const size_t MaxFrames = 1024;

// How the payload length is written into the frame header.  RFC 6455
// allows a 7 bit, a 16 bit or a 64 bit field; a sender is supposed to use
// the shortest that fits, but a hostile peer is not obliged to.
enum LengthEncoding
{
   LenAuto,
   Len7,
   Len16,
   Len64
};

static void
appendHeader(Wire& wire, bool fin, uint8_t opcode, uint64_t declaredLen,
             const uint8_t* maskKey, LengthEncoding enc = LenAuto)
{
   if(enc == LenAuto)
   {
      enc = declaredLen < 126 ? Len7 : (declaredLen <= 0xFFFF ? Len16 : Len64);
   }

   wire.push_back((uint8_t)((fin ? 0x80 : 0x00) | (opcode & 0x0F)));

   const uint8_t maskBit = maskKey ? 0x80 : 0x00;
   switch(enc)
   {
      case Len7:
         wire.push_back((uint8_t)(maskBit | (uint8_t)declaredLen));
         break;
      case Len16:
         wire.push_back((uint8_t)(maskBit | 126));
         wire.push_back((uint8_t)(declaredLen >> 8));
         wire.push_back((uint8_t)declaredLen);
         break;
      default:
         wire.push_back((uint8_t)(maskBit | 127));
         for(int shift = 56; shift >= 0; shift -= 8)
         {
            wire.push_back((uint8_t)(declaredLen >> shift));
         }
         break;
   }

   if(maskKey)
   {
      wire.insert(wire.end(), maskKey, maskKey + 4);
   }
}

// Append a complete frame.  declaredLen defaults to the real payload
// length; the tests that exercise the size checks pass a bigger value so
// that the header lies about how much data follows.
static void
appendFrame(Wire& wire, bool fin, uint8_t opcode, const Data& payload,
            const uint8_t* maskKey = 0, LengthEncoding enc = LenAuto)
{
   appendHeader(wire, fin, opcode, payload.size(), maskKey, enc);
   for(Data::size_type i = 0; i < payload.size(); i++)
   {
      uint8_t byte = (uint8_t)payload[i];
      if(maskKey)
      {
         byte ^= maskKey[i & 3];
      }
      wire.push_back(byte);
   }
}

struct FeedResult
{
   FeedResult() : dropped(false) {}

   vector<Data> messages;
   bool dropped;
};

// Push the bytes through the extractor in chunkSize pieces, draining
// completed messages the same way ConnectionBase::wsProcessData() does.
static FeedResult
feed(WsFrameExtractor& extractor, Wire wire, size_t chunkSize = 0)
{
   FeedResult result;
   if(chunkSize == 0)
   {
      chunkSize = wire.size();
   }

   size_t pos = 0;
   while(pos < wire.size() && !result.dropped)
   {
      const size_t take = min(chunkSize, wire.size() - pos);
      bool drop = false;
      unique_ptr<Data> msg =
         extractor.processBytes(&wire[pos], (Data::size_type)take, drop);
      if(drop)
      {
         result.dropped = true;
      }
      while(msg.get())
      {
         result.messages.push_back(*msg);
         // the extractor hands the buffer over with the message
         delete [] (char*)msg->data();

         bool drainDrop = false;
         msg = extractor.processBytes(0, 0, drainDrop);
         if(drainDrop)
         {
            result.dropped = true;
         }
      }
      pos += take;
   }

   return result;
}

static Data
repeated(char c, Data::size_type len)
{
   return Data(string(len, c).c_str(), len);
}

static void
testSingleUnmaskedFrame()
{
   WsFrameExtractor extractor(8192);
   Wire wire;
   appendFrame(wire, true, OpText, Data("hello"));

   FeedResult result = feed(extractor, wire);
   CHECK(!result.dropped);
   CHECK(result.messages.size() == 1);
   if(result.messages.size() == 1)
   {
      CHECK(result.messages[0] == Data("hello"));
   }
}

static void
testSingleMaskedFrame()
{
   const uint8_t key[4] = { 0x37, 0xfa, 0x21, 0x3d };
   WsFrameExtractor extractor(8192);
   Wire wire;
   appendFrame(wire, true, OpText, Data("REGISTER sip:example.org SIP/2.0"), key);

   FeedResult result = feed(extractor, wire);
   CHECK(!result.dropped);
   CHECK(result.messages.size() == 1);
   if(result.messages.size() == 1)
   {
      CHECK(result.messages[0] == Data("REGISTER sip:example.org SIP/2.0"));
   }
}

static void
testFragmentedMessage()
{
   const uint8_t key[4] = { 0x01, 0x02, 0x03, 0x04 };
   WsFrameExtractor extractor(8192);
   Wire wire;
   appendFrame(wire, false, OpText, Data("part one, "), key);
   appendFrame(wire, false, OpContinuation, Data("part two, "), key);
   appendFrame(wire, true, OpContinuation, Data("part three"), key);

   FeedResult result = feed(extractor, wire);
   CHECK(!result.dropped);
   CHECK(result.messages.size() == 1);
   if(result.messages.size() == 1)
   {
      CHECK(result.messages[0] == Data("part one, part two, part three"));
   }
}

static void
testTwoMessagesInOneBuffer()
{
   WsFrameExtractor extractor(8192);
   Wire wire;
   appendFrame(wire, true, OpText, Data("first"));
   appendFrame(wire, true, OpText, Data("second"));

   FeedResult result = feed(extractor, wire);
   CHECK(!result.dropped);
   CHECK(result.messages.size() == 2);
   if(result.messages.size() == 2)
   {
      CHECK(result.messages[0] == Data("first"));
      CHECK(result.messages[1] == Data("second"));
   }
}

// 16 bit extended length, delivered a few bytes at a time so that the
// header itself has to be reassembled across calls
static void
testExtended16BitLength()
{
   const uint8_t key[4] = { 0xaa, 0xbb, 0xcc, 0xdd };
   const Data payload = repeated('x', 300);

   for(size_t chunk = 1; chunk <= 5; chunk++)
   {
      WsFrameExtractor extractor(8192);
      Wire wire;
      appendFrame(wire, true, OpText, payload, key);

      FeedResult result = feed(extractor, wire, chunk);
      CHECK(!result.dropped);
      CHECK(result.messages.size() == 1);
      if(result.messages.size() == 1)
      {
         CHECK(result.messages[0] == payload);
      }
   }
}

// 64 bit extended length.  A masked frame using this encoding has a 14
// byte header, the longest the extractor accepts, and every byte of the
// length field has to be assembled correctly.  Feeding it in small chunks
// stops the header from ever arriving in one piece.
static void
testExtended64BitLength()
{
   const uint8_t key[4] = { 0x11, 0x22, 0x33, 0x44 };
   const Data payload = repeated('y', 70000);

   for(size_t chunk = 1; chunk <= 4; chunk++)
   {
      WsFrameExtractor extractor(1000000);
      Wire wire;
      appendFrame(wire, true, OpText, payload, key);

      FeedResult result = feed(extractor, wire, chunk == 1 ? 1 : chunk * 1000 - 1);
      CHECK(!result.dropped);
      CHECK(result.messages.size() == 1);
      if(result.messages.size() == 1)
      {
         CHECK(result.messages[0].size() == payload.size());
         CHECK(result.messages[0] == payload);
      }
   }

   // and unmasked, where the header is 10 bytes
   WsFrameExtractor extractor(1000000);
   Wire wire;
   appendFrame(wire, true, OpText, payload, 0, Len64);
   FeedResult result = feed(extractor, wire, 7);
   CHECK(!result.dropped);
   CHECK(result.messages.size() == 1);
   if(result.messages.size() == 1)
   {
      CHECK(result.messages[0] == payload);
   }
}

// A small payload announced with the 64 bit encoding: the extractor must
// read the whole 8 byte field, not just part of it.
static void
testSmallPayloadWith64BitLength()
{
   const uint8_t key[4] = { 0x5a, 0x5a, 0x5a, 0x5a };
   WsFrameExtractor extractor(8192);
   Wire wire;
   appendFrame(wire, true, OpText, Data("tiny"), key, Len64);

   FeedResult result = feed(extractor, wire, 3);
   CHECK(!result.dropped);
   CHECK(result.messages.size() == 1);
   if(result.messages.size() == 1)
   {
      CHECK(result.messages[0] == Data("tiny"));
   }
}

// github issue #478: a fragment followed by a frame whose 64 bit length
// makes mMessageSize + mPayloadLength wrap around.  The frame must be
// rejected rather than passing the capacity check and being allocated.
static void
testFragmentSizeOverflow()
{
   static const uint64_t overflowingLengths[] =
   {
      UINT64_C(0xFFFFFFFFFFFF0001),   // wraps to a small positive value
      UINT64_C(0xFFFFFFFFFFFFFFFF),   // wraps to zero
      UINT64_C(0xFFFFFFFF00000000),
      UINT64_C(0x8000000000000000),   // MSB set, forbidden by RFC 6455
      UINT64_C(0x0000000100000000)    // truncates to zero in 32 bits
   };
   const size_t nLengths =
      sizeof(overflowingLengths) / sizeof(overflowingLengths[0]);

   for(size_t i = 0; i < nLengths; i++)
   {
      for(Data::size_type firstFragment = 0; firstFragment <= 4; firstFragment++)
      {
         // the attacker never has to send the payload it claims is
         // coming, so try both the header on its own and a header with a
         // few bytes trailing it
         for(size_t trailing = 0; trailing <= 3; trailing++)
         {
            WsFrameExtractor extractor(8192);
            Wire wire;
            appendFrame(wire, false, OpText, repeated('a', firstFragment));
            appendHeader(wire, true, OpContinuation, overflowingLengths[i], 0,
                         Len64);
            wire.insert(wire.end(), trailing, 'b');

            FeedResult result = feed(extractor, wire);
            CHECK(result.dropped);
            CHECK(result.messages.empty());
         }
      }
   }
}

// The same lengths on an unfragmented frame
static void
testSingleFrameSizeOverflow()
{
   WsFrameExtractor extractor(8192);
   Wire wire;
   appendHeader(wire, true, OpText, UINT64_C(0xFFFFFFFFFFFFFFFF), 0, Len64);

   FeedResult result = feed(extractor, wire);
   CHECK(result.dropped);
   CHECK(result.messages.empty());
}

static void
testMaxMessageBoundary()
{
   const Data::size_type maxMessage = 1000;

   // exactly at the limit is accepted
   {
      WsFrameExtractor extractor(maxMessage);
      Wire wire;
      const Data payload = repeated('z', maxMessage);
      appendFrame(wire, true, OpText, payload);

      FeedResult result = feed(extractor, wire);
      CHECK(!result.dropped);
      CHECK(result.messages.size() == 1);
      if(result.messages.size() == 1)
      {
         CHECK(result.messages[0] == payload);
      }
   }

   // one byte over is rejected
   {
      WsFrameExtractor extractor(maxMessage);
      Wire wire;
      appendHeader(wire, true, OpText, maxMessage + 1, 0);

      FeedResult result = feed(extractor, wire);
      CHECK(result.dropped);
   }

   // and the limit applies to the total of all fragments, not per frame
   {
      WsFrameExtractor extractor(maxMessage);
      Wire wire;
      appendFrame(wire, false, OpText, repeated('a', 600));
      appendFrame(wire, true, OpContinuation, repeated('b', 600));

      FeedResult result = feed(extractor, wire);
      CHECK(result.dropped);
      CHECK(result.messages.empty());
   }
}

// Empty continuation frames never advance mMessageSize, so the size limit
// alone can never stop them; the frame count has to be bounded as well.
static void
testEmptyFragmentFlood()
{
   WsFrameExtractor extractor(8192);
   Wire wire;
   for(size_t i = 0; i < MaxFrames * 2; i++)
   {
      appendFrame(wire, false, i == 0 ? OpText : OpContinuation, Data::Empty);
   }

   FeedResult result = feed(extractor, wire);
   CHECK(result.dropped);
   CHECK(result.messages.empty());
}

// ...but a message split into a legal number of fragments still works
static void
testManyFragmentsWithinLimit()
{
   WsFrameExtractor extractor(8192);
   Wire wire;
   Data expected;
   for(size_t i = 0; i < MaxFrames; i++)
   {
      const Data fragment(i + 1 == MaxFrames ? "!" : ".");
      appendFrame(wire, i + 1 == MaxFrames, i == 0 ? OpText : OpContinuation,
                  fragment);
      expected += fragment;
   }

   FeedResult result = feed(extractor, wire);
   CHECK(!result.dropped);
   CHECK(result.messages.size() == 1);
   if(result.messages.size() == 1)
   {
      CHECK(result.messages[0] == expected);
   }
}

// A masked frame with a 64 bit length carries a 14 byte header, which is
// exactly mMaxHeaderLen.  It must not be rejected as "header too long",
// and it must parse correctly no matter where the header is split across
// calls, since the length field and the mask key have to be reassembled
// from bytes that arrived separately.
static void
testMaximumLengthHeader()
{
   const uint8_t key[4] = { 0xde, 0xad, 0xbe, 0xef };
   const Data payload("maximum length header");

   for(size_t chunk = 1; chunk <= 16; chunk++)
   {
      WsFrameExtractor extractor(8192);
      Wire wire;
      appendFrame(wire, true, OpText, payload, key, Len64);
      CHECK(wire.size() == 14 + payload.size());

      FeedResult result = feed(extractor, wire, chunk);
      CHECK(!result.dropped);
      CHECK(result.messages.size() == 1);
      if(result.messages.size() == 1)
      {
         CHECK(result.messages[0] == payload);
      }
   }
}

// Malformed and truncated input must not corrupt memory or read past the
// end of the header buffer.  Nothing is asserted about the outcome beyond
// the extractor surviving it.
static void
testMalformedInput()
{
   for(size_t len = 1; len <= 40; len++)
   {
      for(int fill = 0; fill < 3; fill++)
      {
         WsFrameExtractor extractor(8192);
         Wire wire;
         for(size_t i = 0; i < len; i++)
         {
            static const uint8_t fills[3] = { 0x00, 0xFF, 0xAA };
            wire.push_back((uint8_t)(fills[fill] ^ (uint8_t)i));
         }
         feed(extractor, wire, 1);
      }
   }
}

// Destroying an extractor that is holding a half received frame must free
// the partial buffer without crashing.
static void
testDestructorWithPartialFrame()
{
   {
      WsFrameExtractor extractor(8192);
   }

   {
      // header only, no payload bytes yet
      WsFrameExtractor extractor(8192);
      Wire wire;
      appendHeader(wire, true, OpText, 4096, 0);
      FeedResult result = feed(extractor, wire);
      CHECK(!result.dropped);
      CHECK(result.messages.empty());
   }

   {
      // header plus part of the payload
      WsFrameExtractor extractor(8192);
      Wire wire;
      appendHeader(wire, true, OpText, 4096, 0);
      for(size_t i = 0; i < 100; i++)
      {
         wire.push_back('q');
      }
      FeedResult result = feed(extractor, wire);
      CHECK(!result.dropped);
      CHECK(result.messages.empty());
   }

   {
      // complete fragments queued but the final frame never arrives
      WsFrameExtractor extractor(8192);
      Wire wire;
      appendFrame(wire, false, OpText, repeated('r', 50));
      appendFrame(wire, false, OpContinuation, repeated('s', 50));
      FeedResult result = feed(extractor, wire);
      CHECK(!result.dropped);
      CHECK(result.messages.empty());
   }
}

// Every message the extractor produces must be null terminated one past
// its end, because MsgHeaderScanner relies on it.
static void
testNullTermination()
{
   WsFrameExtractor extractor(8192);
   Wire wire;
   appendFrame(wire, false, OpText, Data("INVITE "));
   appendFrame(wire, true, OpContinuation, Data("sip:bob@example.org"));

   bool drop = false;
   unique_ptr<Data> msg =
      extractor.processBytes(&wire[0], (Data::size_type)wire.size(), drop);
   CHECK(!drop);
   CHECK(msg.get() != 0);
   if(msg.get())
   {
      CHECK(*msg == Data("INVITE sip:bob@example.org"));
      CHECK(msg->data()[msg->size()] == 0);
      delete [] (char*)msg->data();
   }
}

int
main()
{
   Log::initialize(Log::Cout, Log::Err, "testWsFrameExtractor");

   testSingleUnmaskedFrame();
   testSingleMaskedFrame();
   testFragmentedMessage();
   testTwoMessagesInOneBuffer();
   testExtended16BitLength();
   testExtended64BitLength();
   testSmallPayloadWith64BitLength();
   testFragmentSizeOverflow();
   testSingleFrameSizeOverflow();
   testMaxMessageBoundary();
   testEmptyFragmentFlood();
   testManyFragmentsWithinLimit();
   testMaximumLengthHeader();
   testMalformedInput();
   testDestructorWithPartialFrame();
   testNullTermination();

   if(sFailures > 0)
   {
      cerr << sFailures << " check(s) failed" << endl;
      return 1;
   }

   cout << "testWsFrameExtractor: all checks passed" << endl;
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
 *
 *
 */
