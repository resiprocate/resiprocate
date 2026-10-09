#if !defined(RESIP_CONNECTIONINFO_HXX)
#define RESIP_CONNECTIONINFO_HXX

#include <list>
#include <memory>

#include "rutil/Data.hxx"
#include "resip/stack/Cookie.hxx"

namespace resip
{

class WsCookieContext;

/**
   @brief Facts about a TLS or WebSocket connection that do not change once it
   is set up.

   The connection builds one instance when it receives its first SIP message
   and every message it receives after that shares it (see
   SipMessage::getConnectionInfo()), so each message carries one pointer instead
   of its own copy of these values.  It is immutable, which makes it safe to read
   from any thread for as long as a message holds it, including after the
   connection itself has gone away.
*/
class ConnectionInfo
{
   public:
      /// Names from the peer's verified TLS certificate (subjectAltName, or
      /// commonName if it has none).  Empty for non-TLS connections, and when
      /// the peer did not present a certificate.
      std::list<Data> mTlsPeerNames;

      /// SNI (TLS server_name) the peer sent in its ClientHello.  Only set for
      /// TLS connections we accepted (server mode); empty if the peer sent none.
      Data mTlsSni;

      /// Cookies from the WebSocket Upgrade request.
      CookieList mWsCookies;

      /// Parsed cookie authentication elements from the WebSocket Upgrade
      /// request, if the transport has a WsCookieContextFactory.
      std::shared_ptr<WsCookieContext> mWsCookieContext;
};

}

#endif

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
 *
 */
