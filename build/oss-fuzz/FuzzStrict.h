/* Strict mode for the fuzz targets' seed-replay tests.
 *
 * A fuzz target normally ignores input its parser rejects, since most
 * fuzzer-generated input is invalid.  The seed-replay ctests set
 * RESIP_FUZZ_STRICT=1, and then fuzzRejected() ends the run with an error,
 * so a seed that no longer parses (or that a harness change has stopped
 * reaching the parser) fails the test instead of passing silently.
 *
 * Usable from C and C++.  Fuzzing runs leave the variable unset, so this
 * costs one getenv() per process.
 */
#ifndef RESIP_FUZZ_STRICT_H
#define RESIP_FUZZ_STRICT_H

#include <stdio.h>
#include <stdlib.h>

static int fuzzStrict(void)
{
   static int strict = -1;
   if (strict < 0)
   {
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996) /* getenv: only read once, from one thread */
#endif
      const char* v = getenv("RESIP_FUZZ_STRICT");
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
      strict = (v != NULL && v[0] != '\0' && v[0] != '0') ? 1 : 0;
   }
   return strict;
}

static void fuzzRejected(const char* why)
{
   if (fuzzStrict())
   {
      fprintf(stderr, "RESIP_FUZZ_STRICT: input was rejected: %s\n",
              why != NULL ? why : "");
      exit(1);
   }
}

#endif

/* ====================================================================

 Copyright (c) 2026, SIP Spectrum, Inc. http://www.sipspectrum.com
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
