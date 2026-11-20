#include <vector>

#include "rutil/Logger.hxx"
#include "rutil/stun/Stun.hxx"

// rutil/stun/Stun.cxx implements the STUN / TURN wire-format parser (RFC 3489
// and RFC 5389 elements, plus turn-08 attributes) that backs reTurn and the
// ICE / TURN media paths.  Although the file is compiled into librutil, no
// fuzz target references its symbols, so the linker drops the object and the
// whole parser has zero fuzz coverage today.  This harness makes the parser
// reachable by calling the public entry point stunParseMessage() on bytes
// from the fuzz buffer.  Fuzzing rationale: STUN packets arrive unchanged
// from the network; the parser walks attacker-controlled TLV attributes
// (including heap-allocated Realm/Nonce/TurnData strings), so memory-safety
// violations here are directly reachable from the wire.

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv) {
   resip::Log::initialize(resip::Log::Cout, resip::Log::None, *argv[0]);
   return 0;
}

// Entrypoint for Clang's libfuzzer
extern "C" int LLVMFuzzerTestOneInput(const unsigned char *data,
                                      unsigned long size) {

   // stunParseMessage takes a mutable char* (historic API); it only reads the
   // buffer, but work on a private copy so the parser can never observe (or
   // be observed writing into) memory outside its own allocation.
   std::vector<char> buf(data, data + size);

   resip::StunMessage msg;
   (void)resip::stunParseMessage(buf.data(),
                                 static_cast<unsigned int>(buf.size()),
                                 msg,
                                 false);

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
