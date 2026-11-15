// Tests for BasicWsConnectionValidator: a WebSocket cookie is accepted only
// when its MAC is exactly the hex HMAC (in either case) of its session info
// and extra under the shared secret, and it hasn't expired.

#include "repro/BasicWsConnectionValidator.hxx"
#include "resip/stack/Cookie.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/WsCookieContext.hxx"
#include "rutil/Data.hxx"
#include "rutil/stun/Stun.hxx"

#include <iostream>
#include <time.h>

using namespace resip;
using namespace repro;

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

const Data kSecret("my-shared-secret");
const Data kExtra("extra-data");

// Session info in the format WsCookieContext parses:
// version:created:expires:from:dest
Data
sessionInfo(time_t expires)
{
   const time_t now = time(NULL);
   return Data(RESIP_WS_COOKIE_CONTEXT_VERSION) + ":" +
          Data(static_cast<uint64_t>(now)) + ":" +
          Data(static_cast<uint64_t>(expires)) + ":" +
          "alice@example.com:bob@example.com";
}

// The MAC the web server would put in the cookie (lower case hex HMAC-SHA1)
Data
macFor(const Data& info, const Data& extra, const Data& secret)
{
   const Data message = info + ':' + extra;
   char hmac[20];
   computeHmac(hmac, message.data(), (int)message.size(), secret.data(), (int)secret.size());
   return Data(hmac, sizeof(hmac)).hex();
}

bool
validate(const Data& info, const Data& extra, const Data& mac, const Data& secret = kSecret)
{
   CookieList cookies;
   cookies.push_back(Cookie("WSSessionInfo", info));
   cookies.push_back(Cookie("WSSessionExtra", extra));
   cookies.push_back(Cookie("WSSessionMAC", mac));
   WsCookieContext context(cookies, "WSSessionInfo", "WSSessionExtra", "WSSessionMAC",
                           Uri("sip:proxy.example.com;transport=ws"));
   BasicWsConnectionValidator validator(secret);
   return validator.validateConnection(context);
}

}

int main()
{
   const Data info = sessionInfo(time(NULL) + 600);
   const Data mac = macFor(info, kExtra, kSecret);
   CHECK(mac.size() == 40);

   // The MAC as generated, and in upper case, is accepted
   CHECK(validate(info, kExtra, mac));
   Data upper(mac);
   upper.uppercase();
   CHECK(upper != mac);
   CHECK(validate(info, kExtra, upper));

   // Any single wrong digit is rejected, first, middle or last
   const Data::size_type positions[] = { 0, 19, 39 };
   for(Data::size_type p : positions)
   {
      Data wrong(mac);
      wrong[p] = (wrong[p] == '0') ? '1' : '0';
      CHECK(!validate(info, kExtra, wrong));
   }

   // The MAC must be exactly 40 digits
   CHECK(!validate(info, kExtra, mac.substr(0, 39)));
   CHECK(!validate(info, kExtra, mac + "0"));
   CHECK(!validate(info, kExtra, mac + mac));

   // Non-hex characters are rejected
   CHECK(!validate(info, kExtra, "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"));
   Data notHex(mac);
   notHex[39] = 'g';
   CHECK(!validate(info, kExtra, notHex));

   // The MAC covers the session info and the extra data, under the secret
   const Data otherInfo = sessionInfo(time(NULL) + 601);
   CHECK(!validate(otherInfo, kExtra, mac));
   CHECK(!validate(info, "other-extra", mac));
   CHECK(!validate(info, kExtra, mac, "another-secret"));

   // A correctly signed but expired cookie is rejected
   const Data expiredInfo = sessionInfo(time(NULL) - 10);
   CHECK(!validate(expiredInfo, kExtra, macFor(expiredInfo, kExtra, kSecret)));

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
