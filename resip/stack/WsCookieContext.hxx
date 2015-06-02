#ifndef RESIP_WsCookieContext_hxx
#define RESIP_WsCookieContext_hxx

#include "Cookie.hxx"
#include "rutil/Data.hxx"
#include "Uri.hxx"

#define RESIP_WS_COOKIE_CONTEXT_VERSION 1

namespace resip
{

class WsCookieContext
{
   public:
      WsCookieContext();
      WsCookieContext(const CookieList& cookieList, const Data& infoCookieName, const Data& extraCookieName, const Data& macCookieName, const Uri& requestUri);
      WsCookieContext(const WsCookieContext& rhs);
      ~WsCookieContext();

      WsCookieContext& operator=(const WsCookieContext& rhs);

      Data getWsSessionInfo() const { return mWsSessionInfo; };
      Data getWsSessionExtra() const { return mWsSessionExtra; };
      Data getWsSessionMAC() const { return mWsSessionMAC; };
      Uri getWsFromUri() const { return mWsFromUri; };
      Uri getWsDestUri() const { return mWsDestUri; };
      time_t getExpiresTime() const { return mExpiresTime; };

   private:
      Data mWsSessionInfo;
      Data mWsSessionExtra;
      Data mWsSessionMAC;
      Uri mWsFromUri;
      Uri mWsDestUri;
      time_t mExpiresTime;
};

}

#endif

/* ====================================================================
 * BSD License
 *
 * Copyright (c) 2013 Catalin Constantin Usurelu  All rights reserved.
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
 */
