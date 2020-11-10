#ifndef RESIP_WsCookieContextFactory_hxx
#define RESIP_WsCookieContextFactory_hxx

#include "Cookie.hxx"
#include "rutil/Data.hxx"
#include "Uri.hxx"
#include "WsCookieContext.hxx"

#include <memory>

namespace resip
{

class WsCookieContextFactory
{
   public:
      WsCookieContextFactory() = default;
      WsCookieContextFactory(const WsCookieContextFactory&) = delete;
      WsCookieContextFactory(WsCookieContextFactory&&) = delete;
      virtual ~WsCookieContextFactory() = default;

      WsCookieContextFactory& operator=(const WsCookieContextFactory&) = delete;
      WsCookieContextFactory& operator=(WsCookieContextFactory&&) = delete;

      virtual std::shared_ptr<WsCookieContext> makeCookieContext(const CookieList& cookieList, const Uri& requestUri) = 0;
};

class BasicWsCookieContextFactory final
   : public WsCookieContextFactory
{
   public:
      explicit BasicWsCookieContextFactory(const Data& infoCookieName = "", const Data& extraCookieName = "", const Data& macCookieName = "")
       : mInfoCookieName(infoCookieName.empty() ? "WSSessionInfo" : infoCookieName),
         mExtraCookieName(extraCookieName.empty() ? "WSSessionExtra": extraCookieName),
         macCookieName(macCookieName.empty() ? "WSSessionMAC" : macCookieName)
      {
      }

      std::shared_ptr<WsCookieContext> makeCookieContext(const CookieList& cookieList, const Uri& requestUri) override
      {
         return std::make_shared<WsCookieContext>(cookieList, mInfoCookieName, mExtraCookieName, macCookieName, requestUri);
      }

   private:
      Data mInfoCookieName;
      Data mExtraCookieName;
      Data macCookieName;
};

}

#endif

/* ====================================================================
 * BSD License
 *
 * Copyright (c) 2013 Daniel Pocock http://danielpocock.com All rights reserved.
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
