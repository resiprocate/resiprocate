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
      // Various bug workarounds that should be rather harmless.
      return SSL_OP_ALL;
   }
   if(optionName == "SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION")
   {
      return SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
   }
   if(optionName == "SSL_OP_CIPHER_SERVER_PREFERENCE")
   {
      // Set on servers to choose the cipher according to server's preferences.
      return SSL_OP_CIPHER_SERVER_PREFERENCE;
   }
#if defined SSL_OP_CISCO_ANYCONNECT
   if(optionName == "SSL_OP_CISCO_ANYCONNECT")
   {
      // Use Cisco's version identifier of DTLS_BAD_VER (only with deprecated DTLSv1_client_method())
      return SSL_OP_CISCO_ANYCONNECT;
   }
#endif
   if(optionName == "SSL_OP_COOKIE_EXCHANGE")
   {
      // Turn on Cookie Exchange (on relevant for servers)
      return SSL_OP_COOKIE_EXCHANGE;
   }
#if defined SSL_OP_CRYPTOPRO_TLSEXT_BUG
   if(optionName == "SSL_OP_CRYPTOPRO_TLSEXT_BUG")
   {
      // Make server add server - hello extension from early version of
      // cryptopro draft, when GOST ciphersuite is negotiated.Required for
      // interoperability with CryptoPro CSP 3.x
      return SSL_OP_CRYPTOPRO_TLSEXT_BUG;
   }
#endif
   if(optionName == "SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS")
   {
      // Disable SSL 3.0/TLS 1.0 CBC vulnerability workaround that was added
      // in OpenSSL 0.9.6d.Usually(depending on the application protocol)
      // the workaround is not needed.Unfortunately some broken SSL / TLS
      // implementations cannot handle it at all, which is why we include it
      // in SSL_OP_ALL.Added in 0.9.6e
      return SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
   }
   if(optionName == "SSL_OP_EPHEMERAL_RSA")
   {
      // Obsolete in newer OpenSSL versions
      return SSL_OP_EPHEMERAL_RSA;
   }
   if(optionName == "SSL_OP_LEGACY_SERVER_CONNECT")
   {
      // Allow initial connection to servers that don't support RI
      return SSL_OP_LEGACY_SERVER_CONNECT;
   }
   if(optionName == "SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER")
   {
      // Obsolete in newer OpenSSL versions
      return SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER;
   }
   if(optionName == "SSL_OP_MICROSOFT_SESS_ID_BUG")
   {
      // Obsolete in newer OpenSSL versions
      return SSL_OP_MICROSOFT_SESS_ID_BUG;
   }
   if(optionName == "SSL_OP_MSIE_SSLV2_RSA_PADDING")
   {
      // Obsolete in newer OpenSSL versions
      return SSL_OP_MSIE_SSLV2_RSA_PADDING;
   }
   if(optionName == "SSL_OP_NETSCAPE_CA_DN_BUG")
   {
      // Obsolete in newer OpenSSL versions
      return SSL_OP_NETSCAPE_CA_DN_BUG;
   }
   if(optionName == "SSL_OP_NETSCAPE_CHALLENGE_BUG")
   {
      // Obsolete in newer OpenSSL versions
      return SSL_OP_NETSCAPE_CHALLENGE_BUG;
   }
   if(optionName == "SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG")
   {
      // Obsolete in newer OpenSSL versions
      return SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG;
   }
   if(optionName == "SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG")
   {
      // Obsolete in newer OpenSSL versions
      return SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG;
   }
#if defined SSL_OP_NO_COMPRESSION
   if(optionName == "SSL_OP_NO_COMPRESSION")
   {
      // Don't use compression even if supported
      return SSL_OP_NO_COMPRESSION;
   }
#endif
   if(optionName == "SSL_OP_NO_QUERY_MTU")
   {
      // DTLS option
      return SSL_OP_NO_QUERY_MTU;
   }
   if(optionName == "SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION")
   {
      // As server, disallow session resumption on renegotiation
      return SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION;
   }
   if(optionName == "SSL_OP_NO_SSLv2")
   {
      // Obsolete in newer OpenSSL versions
      return SSL_OP_NO_SSLv2;
   }
   if(optionName == "SSL_OP_NO_SSLv3")
   {
      return SSL_OP_NO_SSLv3;
   }
   if(optionName == "SSL_OP_NO_TICKET")
   {
      // DTLS option: Don't use RFC4507 ticket extension
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
      // Obsolete in newer OpenSSL versions
      return SSL_OP_PKCS1_CHECK_1;
   }
   if(optionName == "SSL_OP_PKCS1_CHECK_2")
   {
      // Obsolete in newer OpenSSL versions
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
      // Obsolete in newer OpenSSL versions
      return SSL_OP_SINGLE_DH_USE;
   }
   if(optionName == "SSL_OP_SINGLE_ECDH_USE")
   {
      // Obsolete in newer OpenSSL versions
      return SSL_OP_SINGLE_ECDH_USE;
   }
   if(optionName == "SSL_OP_SSLEAY_080_CLIENT_DH_BUG")
   {
      // Obsolete in newer OpenSSL versions
      return SSL_OP_SSLEAY_080_CLIENT_DH_BUG;
   }
   if(optionName == "SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG")
   {
      // Obsolete in newer OpenSSL versions
      return SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG;
   }
   if(optionName == "SSL_OP_TLS_BLOCK_PADDING_BUG")
   {
      // Obsolete in newer OpenSSL versions
      return SSL_OP_TLS_BLOCK_PADDING_BUG;
   }
   if(optionName == "SSL_OP_TLS_D5_BUG")
   {
      // Obsolete in newer OpenSSL versions
      return SSL_OP_TLS_D5_BUG;
   }
   if(optionName == "SSL_OP_TLS_ROLLBACK_BUG")
   {
      // If set, a server will allow a client to issue an SSLv3.0 version
      // number as latest version supported in the premaster secret, even when
      // TLSv1.0 (version 3.1) was announced in the client hello.Normally
      // this is forbidden to prevent version rollback attacks.
      return SSL_OP_TLS_ROLLBACK_BUG;
   }
#ifdef SSL_OP_NO_EXTENDED_MASTER_SECRET
   if (optionName == "SSL_OP_NO_EXTENDED_MASTER_SECRET")
   {
      return SSL_OP_NO_EXTENDED_MASTER_SECRET;
   }
#endif
#ifdef SSL_OP_CLEANSE_PLAINTEXT
   if (optionName == "SSL_OP_CLEANSE_PLAINTEXT")
   {
      // Cleanse plaintext copies of data delivered to the application
      return SSL_OP_CLEANSE_PLAINTEXT;
   }
#endif
#ifdef SSL_OP_ENABLE_KTLS
   if (optionName == "SSL_OP_ENABLE_KTLS")
   {
      return SSL_OP_ENABLE_KTLS;
   }
#endif
#ifdef SSL_OP_TLSEXT_PADDING
   if (optionName == "SSL_OP_TLSEXT_PADDING")
   {
      return SSL_OP_TLSEXT_PADDING;
   }
#endif
#ifdef SSL_OP_IGNORE_UNEXPECTED_EOF
   if (optionName == "SSL_OP_IGNORE_UNEXPECTED_EOF")
   {
      return SSL_OP_IGNORE_UNEXPECTED_EOF;
   }
#endif
#ifdef SSL_OP_ALLOW_CLIENT_RENEGOTIATION
   if (optionName == "SSL_OP_ALLOW_CLIENT_RENEGOTIATION")
   {
      return SSL_OP_ALLOW_CLIENT_RENEGOTIATION;
   }
#endif
#ifdef SSL_OP_DISABLE_TLSEXT_CA_NAMES
   if (optionName == "SSL_OP_DISABLE_TLSEXT_CA_NAMES")
   {
      return SSL_OP_DISABLE_TLSEXT_CA_NAMES;
   }
#endif
#ifdef SSL_OP_ALLOW_NO_DHE_KEX
   if (optionName == "SSL_OP_ALLOW_NO_DHE_KEX")
   {
      // In TLSv1.3 allow a non-(ec)dhe based kex_mode
      return SSL_OP_ALLOW_NO_DHE_KEX;
   }
#endif
#ifdef SSL_OP_NO_ENCRYPT_THEN_MAC
   if (optionName == "SSL_OP_NO_ENCRYPT_THEN_MAC")
   {
      return SSL_OP_NO_ENCRYPT_THEN_MAC;
   }
#endif
#ifdef SSL_OP_ENABLE_MIDDLEBOX_COMPAT
   if (optionName == "SSL_OP_ENABLE_MIDDLEBOX_COMPAT")
   {
      // Enable TLSv1.3 Compatibility mode. This is on by default. A future
      // version of OpenSSL may have this disabled by default.
      return SSL_OP_ENABLE_MIDDLEBOX_COMPAT;
   }
#endif
#ifdef SSL_OP_PRIORITIZE_CHACHA
   if (optionName == "SSL_OP_PRIORITIZE_CHACHA")
   {
      // Prioritize Chacha20Poly1305 when client does. Modifies SSL_OP_CIPHER_SERVER_PREFERENCE.
      return SSL_OP_PRIORITIZE_CHACHA;
   }
#endif
#ifdef SSL_OP_NO_ANTI_REPLAY
   if (optionName == "SSL_OP_NO_ANTI_REPLAY")
   {
      // Switches off automatic TLSv1.3 anti-replay protection for early data.
      // This is a server - side option only(no effect on the client).
      return SSL_OP_NO_ANTI_REPLAY;
   }
#endif
#ifdef SSL_OP_NO_TLSv1_3
   if (optionName == "SSL_OP_NO_TLSv1_3")
   {
      return SSL_OP_NO_TLSv1_3;
   }
#endif
#ifdef SSL_OP_NO_DTLSv1
   if (optionName == "SSL_OP_NO_DTLSv1")
   {
      return SSL_OP_NO_DTLSv1;
   }
#endif
#ifdef SSL_OP_NO_DTLSv1_2
   if (optionName == "SSL_OP_NO_DTLSv1_2")
   {
      return SSL_OP_NO_DTLSv1_2;
   }
#endif
#ifdef SSL_OP_NO_RENEGOTIATION
   if (optionName == "SSL_OP_NO_RENEGOTIATION")
   {
      return SSL_OP_NO_RENEGOTIATION;
   }
#endif
#ifdef SSL_OP_NO_TX_CERTIFICATE_COMPRESSION
   if (optionName == "SSL_OP_NO_TX_CERTIFICATE_COMPRESSION")
   {
      // Disable RFC8879 certificate compression: don't send compressed certificates,
      // and ignore the extension when received.
      return SSL_OP_NO_TX_CERTIFICATE_COMPRESSION;
   }
#endif
#ifdef SSL_OP_NO_RX_CERTIFICATE_COMPRESSION
   if (optionName == "SSL_OP_NO_RX_CERTIFICATE_COMPRESSION")
   {
      // Disable RFC8879 certificate compression: don't send the extension, and
      // subsequently indicating that receiving is not supported
      return SSL_OP_NO_RX_CERTIFICATE_COMPRESSION;
   }
#endif
#ifdef SSL_OP_ENABLE_KTLS_TX_ZEROCOPY_SENDFILE
   if (optionName == "SSL_OP_ENABLE_KTLS_TX_ZEROCOPY_SENDFILE")
   {
      return SSL_OP_ENABLE_KTLS_TX_ZEROCOPY_SENDFILE;
   }
#endif
#ifdef SSL_OP_PREFER_NO_DHE_KEX
   if (optionName == "SSL_OP_PREFER_NO_DHE_KEX")
   {
      return SSL_OP_PREFER_NO_DHE_KEX;
   }
#endif
#ifdef SSL_OP_NO_SSL_MASK
   if (optionName == "SSL_OP_NO_SSL_MASK")
   {
      // (SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2 | SSL_OP_NO_TLSv1_3)
      return SSL_OP_NO_SSL_MASK;
   }
#endif
#ifdef SSL_OP_NO_DTLS_MASK
   if (optionName == "SSL_OP_NO_DTLS_MASK")
   {
      // (SSL_OP_NO_DTLSv1 | SSL_OP_NO_DTLSv1_2)
      return SSL_OP_NO_DTLS_MASK;
   }
#endif
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
