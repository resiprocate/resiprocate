#include <cassert>
#include <iostream>

// Cajun JSON
#include "cajun/json/reader.h"
#include "cajun/json/writer.h"
#include "cajun/json/elements.h"

#include "rutil/hep/HepAgent.hxx"
#include "rutil/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

// a sample packet extracted from Wireshark
static const unsigned char packet1SenderReport[] = {
  0x81, 0xc8, 0x00, 0x0c, 0xa1, 0xa4, 0x21, 0x46,
  0x00, 0x08, 0x4d, 0xdb, 0x24, 0x28, 0x4d, 0xfc,
  0xe2, 0x42, 0xa1, 0xec, 0x00, 0x00, 0x00, 0x3a,
  0x00, 0x00, 0x24, 0x40, 0xf0, 0xca, 0x2f, 0x61,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x3a,
  0x00, 0x00, 0x00, 0x09, 0xa9, 0xfd, 0x0e, 0x08,
  0x00, 0x00, 0x24, 0x55
};

class MyHepAgent : public HepAgent
{
public:
   MyHepAgent() : HepAgent("127.0.0.1", 9050, 2005) {};

   virtual bool sendToWire(const Data& buf) const
   {
      DebugLog(<<"got packet for HOMER, discarding");
      return false;
   }
};

int
main(int argc, char *argv[])
{
   Log::initialize(Log::Cout, Log::Stack, argv[0]);

   MyHepAgent agent;

   sockaddr_in sa;
   sa.sin_addr.s_addr = inet_addr("127.0.0.1");
   GenericIPAddress anon(sa);

   // Values have been extracted using Wireshark
   // SSRC values presented in both decimal and hex for quick comparison
   // against other applications and log files.

   {
      Data packet1(packet1SenderReport, sizeof(packet1SenderReport));
      Data json(agent.convertRTCPtoJSON(packet1));
      ErrLog(<<"got result: " << json);

      json::Object j;
      try
      {
         DataStream stream(json);
         json::Reader::Read(j, stream);
      }
      catch(json::Reader::ScanException& ex)
      {
         ErrLog(<<"failed to scan JSON message: " << ex.what() << " message body: " << json);
         assert(0);
      }

      //assert(json::String(j[""]).Value() == "");

      assert(json::Number(j["ssrc"]).Value() == 2711888198);
      assert(json::Number(j["ssrc"]).Value() == 0xa1a42146);
      assert(json::Number(j["type"]).Value() == 200);

      json::Object& sender_info = j["sender_information"];
      assert(json::Number(sender_info["ntp_timestamp_sec"]).Value() == 544219);
      assert(json::Number(sender_info["ntp_timestamp_usec"]).Value() == 606621180);
      assert(json::Number(sender_info["octets"]).Value() == 9280);
      assert(json::Number(sender_info["rtp_timestamp"]).Value() == 3796017644);
      assert(json::Number(sender_info["packets"]).Value() == 58);

      assert(json::Array(j["report_blocks"]).Size() == 1);
      json::Object& rr0 = j["report_blocks"][0];
      assert(json::Number(rr0["source_ssrc"]).Value() == 4039782241);
      assert(json::Number(rr0["source_ssrc"]).Value() == 0xf0ca2f61);
      assert(json::Number(rr0["highest_seq_no"]).Value() == 6202);
      assert(json::Number(rr0["fraction_lost"]).Value() == 0);
      assert(json::Number(rr0["ia_jitter"]).Value() == 9);
      assert(json::Number(rr0["packets_lost"]).Value() == 0);
      assert(json::Number(rr0["lsr"]).Value() == 2851933704);
      assert(json::Number(rr0["dlsr"]).Value() == 9301);
   }

   return 0;
}

/* ====================================================================
 *
 * Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 * Copyright (c) 2022, Daniel Pocock https://danielpocock.com
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

