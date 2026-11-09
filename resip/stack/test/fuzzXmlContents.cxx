#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"

#include "resip/stack/DialogInfoContents.hxx"
#include "resip/stack/GenericPidfContents.hxx"
#include "resip/stack/Mime.hxx"

// dialog-info (RFC 4237) and presence PIDF (RFC 3863) documents arrive as
// attacker-controlled SIP message bodies and are parsed by reSIProcate's own
// XML pull parser (rutil/XMLCursor).  fuzzStack.cxx deliberately excludes
// these two Contents classes ("leaks or hogs memory"), so up to this harness
// existed no fuzz coverage at all for DialogInfoContents.cxx,
// GenericPidfContents.cxx or XMLCursor.cxx, which all sat at or near 0%.
// This target feeds the fuzz buffer to each XML-based Contents class, checks
// the parse, round-trips it through encode() and exercises the public
// accessors, the same way the test suite does.  Fuzzing rationale: XML
// documents on the SIP wire are untrusted input; memory-safety violations
// and leaks on these paths are genuine findings and will be reported as
// such, never muted.

static void fuzzDialogInfoContents(const resip::HeaderFieldValue& hfv)
{
    static const resip::Mime type("application", "dialog-info+xml");
    resip::DialogInfoContents payload(hfv, type);
    try
    {
        payload.checkParsed();
        (void)payload.getEntity();
        (void)payload.getVersion();
        (void)payload.getDialogInfoState();
        (void)payload.getDialogs();
        resip::Data encoded;
        resip::DataStream ds(encoded);
        payload.encode(ds);
        ds.flush();
    }
    catch (const resip::ParseException&)
    {
    }
}

static void fuzzGenericPidfContents(const resip::HeaderFieldValue& hfv)
{
    static const resip::Mime type("application", "pidf+xml");
    resip::GenericPidfContents payload(hfv, type);
    try
    {
        payload.checkParsed();
        (void)payload.getEntity();
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

// Entrypoint for Clang's libfuzzer
extern "C" int LLVMFuzzerTestOneInput(const unsigned char *data,
                                      unsigned long size) {

  const resip::HeaderFieldValue hfv(reinterpret_cast<const char*>(data), size);

  fuzzDialogInfoContents(hfv);
  fuzzGenericPidfContents(hfv);

  return 0;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
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
 *    and "VOCAL" must not be used to endorse or promote products derived
 *    from this software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this Software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written permission
 *    of Vovida Networks, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ==================================================================== */
