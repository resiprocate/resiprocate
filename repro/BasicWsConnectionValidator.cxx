
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "repro/BasicWsConnectionValidator.hxx"
#include "resip/stack/Cookie.hxx"
#include "resip/stack/WsCookieContext.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/stun/Stun.hxx"
#include "rutil/Logger.hxx"

#include <time.h>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;


BasicWsConnectionValidator::BasicWsConnectionValidator(const Data& wsCookieAuthSharedSecret)
   : mWsCookieAuthSharedSecret(wsCookieAuthSharedSecret)
{
}

BasicWsConnectionValidator::~BasicWsConnectionValidator()
{
}

bool BasicWsConnectionValidator::validateConnection(const resip::WsCookieContext& wsCookieContext)
{
   Data message = wsCookieContext.getWsSessionInfo() + ':' + wsCookieContext.getWsSessionExtra();
   unsigned char hmac[20];
   computeHmac((char*)hmac, message.data(), message.size(), mWsCookieAuthSharedSecret.data(), mWsCookieAuthSharedSecret.size());

   if(strncasecmp(wsCookieContext.getWsSessionMAC().data(), Data(hmac, 20).hex().data(), 40) != 0)
   {
      WarningLog(<< "Cookie MAC validation failed");
      return false;
   }

   if(difftime(wsCookieContext.getExpiresTime(), time(NULL)) < 0)
   {
      WarningLog(<< "Received expired cookie");
      return false;
   }

   return true;
}

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
