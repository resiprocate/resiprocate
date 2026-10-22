#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_SSL

#include "rutil/ResipAssert.h"
#include <iostream>
#include <rutil/ssl/OpenSSLInit.hxx>

#include <openssl/e_os2.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>

#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"

#include <string>
#include <utility>

using namespace dtls;
const char* DtlsFactory::DefaultSrtpProfile = "SRTP_AES128_CM_SHA1_80:SRTP_AES128_CM_SHA1_32";

DtlsFactory::DtlsFactory(std::unique_ptr<DtlsTimerContext> tc,X509 *cert, EVP_PKEY *privkey):
   mTimerContext(std::move(tc)),
   mCert(cert)
{
   int r;

   mContext=SSL_CTX_new(DTLS_method());
   resip_assert(mContext);

   r=SSL_CTX_use_certificate(mContext, cert);
   resip_assert(r==1);

   r=SSL_CTX_use_PrivateKey(mContext, privkey);
   resip_assert(r==1);

   // Set SRTP profiles
   r=SSL_CTX_set_tlsext_use_srtp(mContext, DefaultSrtpProfile);
   resip_assert(r==0);

   // DTLS cookies, for DtlsListener
   r=RAND_bytes(mCookieSecret, sizeof(mCookieSecret));
   resip_assert(r==1);
   SSL_CTX_set_app_data(mContext, this);
   SSL_CTX_set_cookie_generate_cb(mContext, generateCookie);
   SSL_CTX_set_cookie_verify_cb(mContext, verifyCookie);
}

DtlsFactory::~DtlsFactory()
{
   SSL_CTX_free(mContext);
}


DtlsSocket*
DtlsFactory::createClient(std::unique_ptr<DtlsSocketContext> context)
{
   return new DtlsSocket(std::move(context), this, DtlsSocket::Client);
}

DtlsSocket*
DtlsFactory::createServer(std::unique_ptr<DtlsSocketContext> context)
{
   return new DtlsSocket(std::move(context), this, DtlsSocket::Server);
}

std::unique_ptr<DtlsListener>
DtlsFactory::createListener()
{
   return std::unique_ptr<DtlsListener>(new DtlsListener(this));
}

int
DtlsFactory::cookiePeerIndex()
{
   static const int index = SSL_get_ex_new_index(0, (void*)"dtls cookie peer", nullptr, nullptr, nullptr);
   return index;
}

bool
DtlsFactory::computeCookie(SSL* ssl, unsigned char* cookie, unsigned int* cookieLen)
{
   DtlsFactory* factory = static_cast<DtlsFactory*>(SSL_CTX_get_app_data(SSL_get_SSL_CTX(ssl)));
   const std::string* peer = static_cast<const std::string*>(SSL_get_ex_data(ssl, cookiePeerIndex()));
   if(!factory || !peer || peer->empty())
   {
      return false;
   }
   return HMAC(EVP_sha256(), factory->mCookieSecret, sizeof(factory->mCookieSecret),
               (const unsigned char*)peer->data(), peer->size(), cookie, cookieLen) != nullptr;
}

int
DtlsFactory::generateCookie(SSL* ssl, unsigned char* cookie, unsigned int* cookieLen)
{
   // cookie has room for DTLS1_COOKIE_LENGTH (255) bytes, the HMAC takes 32
   return computeCookie(ssl, cookie, cookieLen) ? 1 : 0;
}

int
DtlsFactory::verifyCookie(SSL* ssl, const unsigned char* cookie, unsigned int cookieLen)
{
   unsigned char expected[EVP_MAX_MD_SIZE];
   unsigned int expectedLen = 0;
   return computeCookie(ssl, expected, &expectedLen) &&
          cookieLen == expectedLen && CRYPTO_memcmp(cookie, expected, expectedLen) == 0 ? 1 : 0;
}

void
DtlsFactory::getMyCertFingerprint(char *fingerprint)
{
   DtlsSocket::computeFingerprint(mCert,fingerprint);
}

void
DtlsFactory::setSrtpProfiles(const char *str)
{
   int r;

   r=SSL_CTX_set_tlsext_use_srtp(mContext,str);

   resip_assert(r==0);
}

void
DtlsFactory::setCipherSuites(const char *str)
{
   int r;

   r=SSL_CTX_set_cipher_list(mContext,str);
   resip_assert(r==1);
}

DtlsFactory::PacketType
DtlsFactory::demuxPacket(const unsigned char *data, size_t len) 
{
   // By the first byte, as RFC 7983 section 7 sets out: 0-3 STUN, 20-63 DTLS, 128-191 RTP/RTCP.
   // Anything else (eg. 16-19 ZRTP, 64-79 TURN ChannelData) is unknown, as is an empty datagram,
   // which a peer can send.
   if(len == 0)
      return unknown;

   if(data[0]<=3)
      return stun;
   if((data[0]>=128) && (data[0]<=191))
      return rtp;
   if((data[0]>=20)  && (data[0]<=63))
      return dtls;

   return unknown;
}

#endif //USE_SSL


/* ====================================================================

 Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
 Copyright (c) 2007-2008, Eric Rescorla and Derek MacDonald
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:
 
 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 
 
 3. None of the contributors names may be used to endorse or promote 
    products derived from this software without specific prior written 
    permission. 
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
