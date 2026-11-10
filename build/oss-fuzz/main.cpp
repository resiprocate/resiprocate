// Stand-in for a fuzzing engine's main(), linked into fuzz targets when
// LIB_FUZZING_ENGINE isn't set (see add_fuzzing_executable).
//
//   fuzzTarget file1 [file2 ...]  - run each file through the target once,
//                                   e.g. a crash file from libFuzzer/OSS-Fuzz
//   fuzzTarget                    - smoke test with a fixed dummy input
//
// LLVMFuzzerInitialize isn't called, since not every target defines one.

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);

int main(int argc, char* argv[])
{
   if (argc < 2)
   {
      const uint8_t data[42] = {};
      LLVMFuzzerTestOneInput(data, sizeof(data));
      return 0;
   }

   for (int i = 1; i < argc; ++i)
   {
      std::ifstream f(argv[i], std::ios::binary);
      if (!f)
      {
         std::fprintf(stderr, "cannot open %s\n", argv[i]);
         return 1;
      }
      const std::vector<char> contents((std::istreambuf_iterator<char>(f)),
                                       std::istreambuf_iterator<char>());

      // Copy into a buffer of exactly the input's size, so that reading even
      // one byte past the end is caught by AddressSanitizer.
      uint8_t* buf = new uint8_t[contents.empty() ? 1 : contents.size()];
      if (!contents.empty())
      {
         std::memcpy(buf, contents.data(), contents.size());
      }
      std::fprintf(stderr, "Running %s (%zu bytes)\n", argv[i], contents.size());
      LLVMFuzzerTestOneInput(buf, contents.size());
      delete[] buf;
      std::fprintf(stderr, "Done %s\n", argv[i]);
   }
   return 0;
}


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