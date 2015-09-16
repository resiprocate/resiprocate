#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_SSL

#include "rutil/ResipAssert.h"
#include <iostream>
#include <rutil/ssl/OpenSSLInit.hxx>

#include <openssl/e_os2.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>

#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"

using namespace dtls;
const char* DtlsFactory::DefaultSrtpProfile = "SRTP_AES128_CM_SHA1_80:SRTP_AES128_CM_SHA1_32";

DtlsFactory::DtlsFactory(std::auto_ptr<DtlsTimerContext> tc,X509 *cert, EVP_PKEY *privkey):
   mTimerContext(tc),
   mCert(cert)
{
   int r;

   mContext=SSL_CTX_new(DTLSv1_method());
   resip_assert(mContext);

   r=SSL_CTX_use_certificate(mContext, cert);
   resip_assert(r==1);

   r=SSL_CTX_use_PrivateKey(mContext, privkey);
   resip_assert(r==1);

   // Set SRTP profiles
   r=SSL_CTX_set_tlsext_use_srtp(mContext, DefaultSrtpProfile);
   resip_assert(r==0);
}

DtlsFactory::~DtlsFactory()
{
   SSL_CTX_free(mContext);
}


DtlsSocket*
DtlsFactory::createClient(std::auto_ptr<DtlsSocketContext> context)
{
   return new DtlsSocket(context,this,DtlsSocket::Client);
}

DtlsSocket*
DtlsFactory::createServer(std::auto_ptr<DtlsSocketContext> context)
{
   return new DtlsSocket(context,this,DtlsSocket::Server);  
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
DtlsFactory::demuxPacket(const unsigned char *data, unsigned int len) 
{
   resip_assert(len>=1);

   if((data[0]==0)   || (data[0]==1))
      return stun;
   if((data[0]>=128) && (data[0]<=191))
      return rtp;
   if((data[0]>=20)  && (data[0]<=64))
      return dtls;

   return unknown;
}

#endif //USE_SSL


/* ====================================================================

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
