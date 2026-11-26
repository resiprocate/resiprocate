#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"

#include "resip/stack/Auth.hxx"
#include "resip/stack/CSeqCategory.hxx"
#include "resip/stack/CallId.hxx"
#include "resip/stack/CpimContents.hxx"
#include "resip/stack/DateCategory.hxx"
#include "resip/stack/DtmfPayloadContents.hxx"
#include "resip/stack/ExpiresCategory.hxx"
#include "resip/stack/GenericPidfContents.hxx"
#include "resip/stack/GenericUri.hxx"
#include "resip/stack/IntegerCategory.hxx"
#include "resip/stack/InvalidContents.hxx"
#include "resip/stack/MessageWaitingContents.hxx"
#include "resip/stack/Mime.hxx"
#include "resip/stack/MultipartMixedContents.hxx"
#include "resip/stack/NameAddr.hxx"
#include "resip/stack/OctetContents.hxx"
#include "resip/stack/Pkcs7Contents.hxx"
#include "resip/stack/Pkcs8Contents.hxx"
#include "resip/stack/PlainContents.hxx"
#include "resip/stack/PrivacyCategory.hxx"
#include "resip/stack/RAckCategory.hxx"
#include "resip/stack/RequestLine.hxx"
#include "resip/stack/Rlmi.hxx"
#include "resip/stack/SdpContents.hxx"
#include "resip/stack/SipFrag.hxx"
#include "resip/stack/StatusLine.hxx"
#include "resip/stack/StringCategory.hxx"
#include "resip/stack/Token.hxx"
#include "resip/stack/TokenOrQuotedStringCategory.hxx"
#include "resip/stack/UInt32Category.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/Via.hxx"
#include "resip/stack/WarningCategory.hxx"
#include "resip/stack/X509Contents.hxx"

#include "TestSupport.hxx"

static void fuzzAuth(const resip::HeaderFieldValue& hfv)
{
    resip::Auth payload(hfv, resip::Headers::UNKNOWN);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzCSeqCategory(const resip::HeaderFieldValue& hfv)
{
    resip::CSeqCategory payload(hfv, resip::Headers::UNKNOWN);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzCpimContents(const resip::HeaderFieldValue& hfv)
{
    static const resip::Mime type("message", "cpim");
    resip::CpimContents payload(hfv, type);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzCallId(const resip::HeaderFieldValue& hfv)
{
    resip::CallId payload(hfv, resip::Headers::UNKNOWN);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzDtmfPayloadContents(const resip::HeaderFieldValue& hfv)
{
  static const resip::Mime type("application", "dtmf-relay");
  resip::DtmfPayloadContents payload(hfv, type);
  try
  {
    payload.checkParsed();
  }
  catch (const resip::ParseException&)
  {
  }
}

static void fuzzDateCategory(const resip::HeaderFieldValue& hfv)
{
    resip::DateCategory payload(hfv, resip::Headers::UNKNOWN);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzExpiresCategory(const resip::HeaderFieldValue& hfv)
{
    resip::ExpiresCategory payload(hfv, resip::Headers::UNKNOWN);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzGenericUri(const resip::HeaderFieldValue& hfv)
{
    resip::GenericURI payload(hfv, resip::Headers::UNKNOWN);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzIntegerCategory(const resip::HeaderFieldValue& hfv)
{
    resip::IntegerCategory payload(hfv, resip::Headers::UNKNOWN);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzInvalidContents(const resip::HeaderFieldValue& hfv)
{
    static const resip::Mime type("Invalid", "Invalid");
    resip::InvalidContents payload(hfv, type, type);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzMessageWaitingContents(const resip::HeaderFieldValue& hfv)
{
    static const resip::Mime type("application", "simple-message-summary");
    resip::MessageWaitingContents payload(hfv, type);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzMime(const resip::HeaderFieldValue& hfv)
{
    resip::Mime payload(hfv, resip::Headers::UNKNOWN);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzMultipartMixedContents(const resip::HeaderFieldValue& hfv)
{
    // Without a boundary parameter the contents generate a random one, which
    // no input could match, so use a fixed boundary that the seeds also use.
    static const resip::Mime type = []()
    {
        resip::Mime t("multipart", "mixed");
        t.param(resip::p_boundary) = "boundary42";
        return t;
    }();
    resip::MultipartMixedContents payload(hfv, type);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
    catch (const resip::MultipartMixedContents::Exception&)
    {
        // Malformed line endings and over-deep nesting are reported with
        // this exception rather than ParseException.
    }
}

static void fuzzNameAddr(const resip::Data& buffer)
{
  try
  {
    resip::NameAddr illegal(buffer);
  }
  catch (const resip::ParseException&)
  {
  }
}

static void fuzzOctetContents(const resip::HeaderFieldValue& hfv)
{
    static const resip::Mime type("application", "octet-stream");
    resip::OctetContents payload(hfv, type);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzPkcs7Contents(const resip::HeaderFieldValue& hfv)
{
    static const resip::Mime type("application", "pkcs7-mime");
    resip::Pkcs7Contents payload(hfv, type);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzPkcs8Contents(const resip::HeaderFieldValue& hfv)
{
    static const resip::Mime type("application", "pkcs8");
    resip::Pkcs8Contents payload(hfv, type);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzPlainContents(const resip::HeaderFieldValue& hfv)
{
    static const resip::Mime type("text", "plain");
    resip::PlainContents payload(hfv, type);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzPrivacyCategory(const resip::HeaderFieldValue& hfv)
{
    resip::PrivacyCategory payload(hfv, resip::Headers::UNKNOWN);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzRAckCategory(const resip::HeaderFieldValue& hfv)
{
    resip::RAckCategory payload(hfv, resip::Headers::UNKNOWN);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzRequestLine(const resip::HeaderFieldValue& hfv)
{
    resip::RequestLine payload(hfv);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzRlmi(const resip::HeaderFieldValue& hfv)
{
    static const resip::Mime type("application", "rlmi+xml");
    resip::Rlmi payload(hfv, type);
    try
    {
        payload.checkParsed();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzSdp(const resip::HeaderFieldValue& hfv)
{
  static const resip::Mime type("application", "sdp");
  resip::SdpContents sdp(hfv, type);
  try
  {
    sdp.checkParsed();
  }
  catch (const resip::ParseException&)
  {
  }
}

static void fuzzSip(const resip::Data& buffer)
{
  try
  {
    std::unique_ptr<resip::SipMessage> msg(resip::TestSupport::makeMessage(buffer));
  }
  catch (const resip::ParseException&)
  {
  }
}

static void fuzzUri(const resip::HeaderFieldValue& hfv)
{
  resip::Uri uri(hfv, resip::Headers::UNKNOWN);
  try
  {
    uri.embedded();
  }
  catch (const resip::ParseException&)
  {
  }
}

// Parsers that previously had no fuzz coverage at all (Via.cxx, Token.cxx,
// TokenOrQuotedStringCategory.cxx, SipFrag.cxx, StatusLine.cxx,
// StringCategory.cxx, UInt32Category.cxx, WarningCategory.cxx and
// X509Contents.cxx all sat at or near 0% line coverage).  Each one is a
// header-field-value or contents parser whose input arrives unchanged from
// the wire, so the fuzz buffer goes straight into the parser constructor and
// is then round-tripped through encode() the same way the test suite does.
// Fuzzing rationale: these parsers run on every received SIP message, so
// the fuzz buffer exercises the same parse and encode paths that wire
// input takes.

static void fuzzSipFrag(const resip::HeaderFieldValue& hfv)
{
    static const resip::Mime type("message", "sipfrag");
    resip::SipFrag payload(hfv, type);
    try
    {
        payload.checkParsed();
        resip::Data encoded;
        resip::DataStream ds(encoded);
        payload.encode(ds);
        ds.flush();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzStatusLine(const resip::HeaderFieldValue& hfv)
{
    resip::StatusLine payload(hfv);
    try
    {
        payload.checkParsed();
        resip::Data encoded;
        resip::DataStream ds(encoded);
        payload.encode(ds);
        ds.flush();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzStringCategory(const resip::HeaderFieldValue& hfv)
{
    resip::StringCategory payload(hfv, resip::Headers::UNKNOWN);
    try
    {
        payload.checkParsed();
        resip::Data encoded;
        resip::DataStream ds(encoded);
        payload.encode(ds);
        ds.flush();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzToken(const resip::HeaderFieldValue& hfv)
{
    resip::Token payload(hfv, resip::Headers::UNKNOWN);
    try
    {
        payload.checkParsed();
        resip::Data encoded;
        resip::DataStream ds(encoded);
        payload.encode(ds);
        ds.flush();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzTokenOrQuotedStringCategory(const resip::HeaderFieldValue& hfv)
{
    resip::TokenOrQuotedStringCategory payload(hfv, resip::Headers::UNKNOWN);
    try
    {
        payload.checkParsed();
        resip::Data encoded;
        resip::DataStream ds(encoded);
        payload.encode(ds);
        ds.flush();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzUInt32Category(const resip::HeaderFieldValue& hfv)
{
    resip::UInt32Category payload(hfv, resip::Headers::UNKNOWN);
    try
    {
        payload.checkParsed();
        resip::Data encoded;
        resip::DataStream ds(encoded);
        payload.encode(ds);
        ds.flush();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzVia(const resip::HeaderFieldValue& hfv)
{
    resip::Via payload(hfv, resip::Headers::UNKNOWN);
    try
    {
        payload.checkParsed();
        resip::Data encoded;
        resip::DataStream ds(encoded);
        payload.encode(ds);
        ds.flush();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzWarningCategory(const resip::HeaderFieldValue& hfv)
{
    resip::WarningCategory payload(hfv, resip::Headers::UNKNOWN);
    try
    {
        payload.checkParsed();
        resip::Data encoded;
        resip::DataStream ds(encoded);
        payload.encode(ds);
        ds.flush();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzX509Contents(const resip::HeaderFieldValue& hfv)
{
    static const resip::Mime type("application", "pkix-cert");
    resip::X509Contents payload(hfv, type);
    try
    {
        payload.checkParsed();
        resip::Data encoded;
        resip::DataStream ds(encoded);
        payload.encode(ds);
        ds.flush();
    }
    catch (const resip::ParseException&)
    {
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv) {
  resip::Log::initialize(resip::Log::Cout, resip::Log::None, *argv[0]);
  return 0;
}

typedef void (*FuzzTarget)(const resip::Data& buffer, const resip::HeaderFieldValue& hfv);

#define FUZZ_HFV(f) [](const resip::Data&, const resip::HeaderFieldValue& hfv) { f(hfv); }
#define FUZZ_BUF(f) [](const resip::Data& buffer, const resip::HeaderFieldValue&) { f(buffer); }

// The first input byte selects one parser (modulo the table size) and the
// rest of the input is handed to it, so each run exercises a single parser
// and the fuzzer can mutate towards that parser's grammar.  Seed files start
// with the index of the parser they are meant for.  Only append new entries,
// so that existing seeds and corpus files keep selecting the same parser.
static const FuzzTarget fuzzTargets[] =
{
  FUZZ_HFV(fuzzAuth),                        //  0
  FUZZ_HFV(fuzzCSeqCategory),                //  1
  FUZZ_HFV(fuzzCallId),                      //  2
  FUZZ_HFV(fuzzCpimContents),                //  3
  FUZZ_HFV(fuzzDtmfPayloadContents),         //  4
  FUZZ_HFV(fuzzDateCategory),                //  5
  FUZZ_HFV(fuzzExpiresCategory),             //  6
  FUZZ_HFV(fuzzGenericUri),                  //  7
  FUZZ_HFV(fuzzIntegerCategory),             //  8
  FUZZ_HFV(fuzzInvalidContents),             //  9
  FUZZ_HFV(fuzzMessageWaitingContents),      // 10
  FUZZ_HFV(fuzzMime),                        // 11
  FUZZ_HFV(fuzzMultipartMixedContents),      // 12
  FUZZ_BUF(fuzzNameAddr),                    // 13
  FUZZ_HFV(fuzzOctetContents),               // 14
  FUZZ_HFV(fuzzPkcs7Contents),               // 15
  FUZZ_HFV(fuzzPkcs8Contents),               // 16
  FUZZ_HFV(fuzzPlainContents),               // 17
  FUZZ_HFV(fuzzPrivacyCategory),             // 18
  FUZZ_HFV(fuzzRAckCategory),                // 19
  FUZZ_HFV(fuzzRequestLine),                 // 20
  FUZZ_HFV(fuzzRlmi),                        // 21
  FUZZ_HFV(fuzzSdp),                         // 22
  FUZZ_BUF(fuzzSip),                         // 23
  FUZZ_HFV(fuzzUri),                         // 24
  FUZZ_HFV(fuzzSipFrag),                     // 25
  FUZZ_HFV(fuzzStatusLine),                  // 26
  FUZZ_HFV(fuzzStringCategory),              // 27
  FUZZ_HFV(fuzzToken),                       // 28
  FUZZ_HFV(fuzzTokenOrQuotedStringCategory), // 29
  FUZZ_HFV(fuzzUInt32Category),              // 30
  FUZZ_HFV(fuzzVia),                         // 31
  FUZZ_HFV(fuzzWarningCategory),             // 32
  FUZZ_HFV(fuzzX509Contents),                // 33
};

#undef FUZZ_HFV
#undef FUZZ_BUF

// Entrypoint for Clang's libfuzzer
extern "C" int LLVMFuzzerTestOneInput(const unsigned char *data,
                                      size_t size) {

  if (size < 1)
  {
    return 0;
  }

  const FuzzTarget target = fuzzTargets[data[0] % (sizeof(fuzzTargets) / sizeof(fuzzTargets[0]))];
  const char* payload = reinterpret_cast<const char*>(data + 1);
  const size_t payloadSize = size - 1;

  const resip::Data buffer(resip::Data::Share, payload, payloadSize);
  const resip::HeaderFieldValue hfv(payload, payloadSize);

  target(buffer, hfv);

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
