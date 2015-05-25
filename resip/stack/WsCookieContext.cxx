#include "resip/stack/WsCookieContext.hxx"
#include "resip/stack/Transport.hxx"
#include "resip/stack/UnknownParameterType.hxx"
#include "rutil/Data.hxx"
#include "rutil/Logger.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

WsCookieContext::WsCookieContext()
{
}

WsCookieContext::WsCookieContext(const WsCookieContext& rhs)
{
   mWsSessionInfo = rhs.getWsSessionInfo();
   mWsSessionExtra = rhs.getWsSessionExtra();
   mWsSessionMAC = rhs.getWsSessionMAC();
   mWsFromUri = rhs.getWsFromUri();
   mWsDestUri = rhs.getWsDestUri();
   mExpiresTime = rhs.getExpiresTime();
}


WsCookieContext& WsCookieContext::operator=(const WsCookieContext& rhs)
{
   if(this != &rhs)
   {
      mWsSessionInfo = rhs.getWsSessionInfo();
      mWsSessionExtra = rhs.getWsSessionExtra();
      mWsSessionMAC = rhs.getWsSessionMAC();
      mWsFromUri = rhs.getWsFromUri();
      mWsDestUri = rhs.getWsDestUri();
      mExpiresTime = rhs.getExpiresTime();
   }
   return *this;
}

WsCookieContext::WsCookieContext(const CookieList& cookieList, const Data& infoCookieName, const Data& extraCookieName, const Data& macCookieName, const Uri& requestUri)
{
   for (CookieList::const_iterator it = cookieList.begin(); it != cookieList.end(); ++it)
   {
      if ((*it).name() == infoCookieName)
      {
         mWsSessionInfo = (*it).value();
      }
      else if ((*it).name() == extraCookieName)
      {
         mWsSessionExtra = (*it).value();
      }
      else if ((*it).name() == macCookieName)
      {
         mWsSessionMAC = (*it).value();
      }
   }

   // If present, parameters in the request URI override those in the cookies
   UnknownParameterType pInfo(infoCookieName);
   if(requestUri.exists(pInfo))
   {
      mWsSessionInfo = requestUri.param(pInfo).urlDecoded();
   }
   UnknownParameterType pExtra(extraCookieName);
   if(requestUri.exists(pExtra))
   {
      mWsSessionExtra = requestUri.param(pExtra).urlDecoded();
   }
   UnknownParameterType pMac(macCookieName);
   if(requestUri.exists(pMac))
   {
      mWsSessionMAC = requestUri.param(pMac).urlDecoded();
   }

   if(mWsSessionInfo.empty())
   {
      ErrLog(<<"Cookie " << infoCookieName << " missing or empty");
      throw Transport::Exception("Required cookie missing", __FILE__, __LINE__);
   }
   if(mWsSessionMAC.empty())
   {
      ErrLog(<<"Cookie " << macCookieName << " missing or empty");
      throw Transport::Exception("Required cookie missing", __FILE__, __LINE__);
   }

   ParseBuffer pb(mWsSessionInfo);
   StackLog(<<"Checking Cookie scheme version");
   int contextVersion = pb.uInt32();
   if(contextVersion != RESIP_WS_COOKIE_CONTEXT_VERSION)
   {
      // Cookie created for another version of the code
      ErrLog(<<"Expecting cookie version " << RESIP_WS_COOKIE_CONTEXT_VERSION << " but found " << contextVersion);
      throw ParseException("Cookie version mismatch", pb.getContext(), __FILE__, __LINE__);
   }
   pb.skipToChar(':');
   pb.skipChar(':');

   pb.skipToChar(':');  // skip the creation time
   pb.skipChar(':');
   mExpiresTime = (time_t) pb.uInt64();

   const char* anchor;
   Data uriString;

   pb.skipToChar(':');
   pb.skipChar(':');
   anchor = pb.position();
   pb.skipToChar(':');
   pb.data(uriString, anchor);
   mWsFromUri = Uri("sip:" + uriString);

   pb.skipChar(':');
   anchor = pb.position();
   pb.skipToChar(':');
   pb.data(uriString, anchor);
   mWsDestUri = Uri("sip:" + uriString);
}

WsCookieContext::~WsCookieContext()
{
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
