// Tests for resip::constantTimeEquals: it must say equal only when every byte
// matches, wherever a difference is and whatever bit it is in.

#include "rutil/ConstantTimeEquals.hxx"

#include <cstring>
#include <iostream>

using namespace resip;

namespace
{

int gFailures = 0;

#define CHECK(cond) \
   do { \
      if(!(cond)) \
      { \
         gFailures++; \
         std::cerr << __FILE__ << ":" << __LINE__ << ": check failed: " #cond << std::endl; \
      } \
   } while(0)

}

int main()
{
   unsigned char a[32];
   unsigned char b[32];
   for(size_t i = 0; i < sizeof(a); ++i)
   {
      a[i] = static_cast<unsigned char>(i * 37 + 11);
   }
   memcpy(b, a, sizeof(a));

   CHECK(constantTimeEquals(a, b, sizeof(a)));
   CHECK(constantTimeEquals(a, a, sizeof(a)));
   CHECK(constantTimeEquals(a, b, 0));

   // A difference in any byte, in any bit, is caught
   for(size_t i = 0; i < sizeof(a); ++i)
   {
      for(int bit = 0; bit < 8; ++bit)
      {
         b[i] = static_cast<unsigned char>(a[i] ^ (1 << bit));
         CHECK(!constantTimeEquals(a, b, sizeof(a)));
         CHECK(!constantTimeEquals(b, a, sizeof(a)));
         b[i] = a[i];
      }
   }

   // Bytes past len are not looked at
   b[sizeof(b) - 1] ^= 0xff;
   CHECK(constantTimeEquals(a, b, sizeof(a) - 1));
   CHECK(!constantTimeEquals(a, b, sizeof(a)));
   b[sizeof(b) - 1] = a[sizeof(a) - 1];

   // Signed char buffers (as the STUN hash attributes are) compare the same way
   const char x[] = "\x80\xff\x01";
   const char y[] = "\x80\xff\x01";
   const char z[] = "\x80\x7f\x01";
   CHECK(constantTimeEquals(x, y, 3));
   CHECK(!constantTimeEquals(x, z, 3));

   if(gFailures)
   {
      std::cerr << gFailures << " check(s) failed" << std::endl;
      return 1;
   }
   std::cerr << "All OK" << std::endl;
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
