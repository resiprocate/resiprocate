#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <sys/types.h>
#include <iostream>
#include <memory>

#include "rutil/Logger.hxx"
#include "resip/stack/Cookie.hxx"
#include "resip/stack/WsCookieContext.hxx"
#include "resip/stack/WsCookieContextFactory.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

int main()
{
   Cookie info("WSSessionInfo", "1%3A1387814798%3A1987814798%3Abob@example.org%3Aalice@example.org");
   Cookie extra("WSSessionExtra", "custom");
   Cookie mac("WSSessionMAC", "abcdabcdabcdabcdabcdabcdabcdabcd");

   CookieList cookies;
   cookies.push_back(info);
   cookies.push_back(extra);
   cookies.push_back(mac);

   BasicWsCookieContextFactory factory;
   Uri uri("/");
   SharedPtr<WsCookieContext> ctx = factory.makeCookieContext(cookies, uri);

   assert(ctx->getExpiresTime() == 1987814798);

   uri = Uri("/;WSSessionInfo=1%3A1387814798%3A1987834798%3Abob@example.org%3Aalice@example.org;WSSessionExtra=crazy%3Akangaroo");
   ctx = factory.makeCookieContext(cookies, uri);
   assert(ctx->getExpiresTime() == 1987834798);
   assert(ctx->getWsSessionExtra() == "crazy:kangaroo");

   return 0;
}


/* ====================================================================
 *
 * Copyright (c) 2013 Daniel Pocock  All rights reserved.
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

