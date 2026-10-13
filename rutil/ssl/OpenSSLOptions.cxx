#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdexcept>

#include <openssl/ssl.h>

#include "rutil/ssl/OpenSSLOptions.hxx"

using namespace std;

namespace resip
{

long
parseOpenSSLCTXOption(const Data& optionName)
{
   if(optionName == "SSL_OP_ALL")
   {
      return SSL_OP_ALL;
   }
   if(optionName == "SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION")
   {
      return SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
   }
   if(optionName == "SSL_OP_CIPHER_SERVER_PREFERENCE")
   {
      return SSL_OP_CIPHER_SERVER_PREFERENCE;
   }
#if defined SSL_OP_CISCO_ANYCONNECT
   if(optionName == "SSL_OP_CISCO_ANYCONNECT")
   {
      return SSL_OP_CISCO_ANYCONNECT;
   }
#endif
   if(optionName == "SSL_OP_COOKIE_EXCHANGE")
   {
      return SSL_OP_COOKIE_EXCHANGE;
   }
#if defined SSL_OP_CRYPTOPRO_TLSEXT_BUG
   if(optionName == "SSL_OP_CRYPTOPRO_TLSEXT_BUG")
   {
      return SSL_OP_CRYPTOPRO_TLSEXT_BUG;
   }
#endif
   if(optionName == "SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS")
   {
      return SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
   }
   if(optionName == "SSL_OP_EPHEMERAL_RSA")
   {
      return SSL_OP_EPHEMERAL_RSA;
   }
   if(optionName == "SSL_OP_LEGACY_SERVER_CONNECT")
   {
      return SSL_OP_LEGACY_SERVER_CONNECT;
   }
   if(optionName == "SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER")
   {
      return SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER;
   }
   if(optionName == "SSL_OP_MICROSOFT_SESS_ID_BUG")
   {
      return SSL_OP_MICROSOFT_SESS_ID_BUG;
   }
   if(optionName == "SSL_OP_MSIE_SSLV2_RSA_PADDING")
   {
      return SSL_OP_MSIE_SSLV2_RSA_PADDING;
   }
   if(optionName == "SSL_OP_NETSCAPE_CA_DN_BUG")
   {
      return SSL_OP_NETSCAPE_CA_DN_BUG;
   }
   if(optionName == "SSL_OP_NETSCAPE_CHALLENGE_BUG")
   {
      return SSL_OP_NETSCAPE_CHALLENGE_BUG;
   }
   if(optionName == "SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG")
   {
      return SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG;
   }
   if(optionName == "SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG")
   {
      return SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG;
   }
#if defined SSL_OP_NO_COMPRESSION
   if(optionName == "SSL_OP_NO_COMPRESSION")
   {
      return SSL_OP_NO_COMPRESSION;
   }
#endif
   if(optionName == "SSL_OP_NO_QUERY_MTU")
   {
      return SSL_OP_NO_QUERY_MTU;
   }
   if(optionName == "SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION")
   {
      return SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION;
   }
   if(optionName == "SSL_OP_NO_SSLv2")
   {
      return SSL_OP_NO_SSLv2;
   }
   if(optionName == "SSL_OP_NO_SSLv3")
   {
      return SSL_OP_NO_SSLv3;
   }
   if(optionName == "SSL_OP_NO_TICKET")
   {
      return SSL_OP_NO_TICKET;
   }
   if(optionName == "SSL_OP_NO_TLSv1")
   {
      return SSL_OP_NO_TLSv1;
   }
#ifdef SSL_OP_NO_TLSv1_1
   if(optionName == "SSL_OP_NO_TLSv1_1")
   {
      return SSL_OP_NO_TLSv1_1;
   }
#endif
#ifdef SSL_OP_NO_TLSv1_2
   if(optionName == "SSL_OP_NO_TLSv1_2")
   {
      return SSL_OP_NO_TLSv1_2;
   }
#endif
   if(optionName == "SSL_OP_PKCS1_CHECK_1")
   {
      return SSL_OP_PKCS1_CHECK_1;
   }
   if(optionName == "SSL_OP_PKCS1_CHECK_2")
   {
      return SSL_OP_PKCS1_CHECK_2;
   }
#ifdef SSL_OP_SAFARI_ECDHE_ECDSA_BUG
   if(optionName == "SSL_OP_SAFARI_ECDHE_ECDSA_BUG")
   {
      return SSL_OP_SAFARI_ECDHE_ECDSA_BUG;
   }
#endif
   if(optionName == "SSL_OP_SINGLE_DH_USE")
   {
      return SSL_OP_SINGLE_DH_USE;
   }
   if(optionName == "SSL_OP_SINGLE_ECDH_USE")
   {
      return SSL_OP_SINGLE_ECDH_USE;
   }
   if(optionName == "SSL_OP_SSLEAY_080_CLIENT_DH_BUG")
   {
      return SSL_OP_SSLEAY_080_CLIENT_DH_BUG;
   }
   if(optionName == "SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG")
   {
      return SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG;
   }
   if(optionName == "SSL_OP_TLS_BLOCK_PADDING_BUG")
   {
      return SSL_OP_TLS_BLOCK_PADDING_BUG;
   }
   if(optionName == "SSL_OP_TLS_D5_BUG")
   {
      return SSL_OP_TLS_D5_BUG;
   }
   if(optionName == "SSL_OP_TLS_ROLLBACK_BUG")
   {
      return SSL_OP_TLS_ROLLBACK_BUG;
   }
   Data error = "Not a recognized OpenSSL option name: " + optionName;
   throw invalid_argument(error.c_str());
}

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
