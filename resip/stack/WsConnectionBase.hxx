#ifndef RESIP_WsConnectionBase_hxx
#define RESIP_WsConnectionBase_hxx

#include <resip/stack/WsConnectionValidator.hxx>
#include <resip/stack/Cookie.hxx>
#include <resip/stack/WsCookieContext.hxx>

#include <memory>
#include <vector>

namespace resip
{

class WsConnectionBase
{
   public:
      WsConnectionBase() = default;
      explicit WsConnectionBase(std::shared_ptr<WsConnectionValidator> mWsConnectionValidator);
      WsConnectionBase(const WsConnectionBase&) = delete;
      WsConnectionBase(WsConnectionBase&&) = delete;
      virtual ~WsConnectionBase() = default;

      WsConnectionBase& operator=(const WsConnectionBase&) = delete;
      WsConnectionBase& operator=(WsConnectionBase&&) = delete;

      void setCookies(const CookieList& cookies);
      const CookieList& getCookies() const noexcept;
      std::shared_ptr<WsCookieContext> getWsCookieContext() const noexcept;
      void setWsCookieContext(std::shared_ptr<WsCookieContext> wsCookieContext) noexcept;
      std::shared_ptr<WsConnectionValidator> connectionValidator() const noexcept;

   private:
      CookieList mCookies;
      std::shared_ptr<WsCookieContext> mWsCookieContext;
      std::shared_ptr<WsConnectionValidator> mWsConnectionValidator;
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

