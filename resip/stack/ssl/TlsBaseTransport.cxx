#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifdef USE_SSL

#include <memory>
#include <stdexcept>

#include "rutil/compat.hxx"
#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/ssl/TlsBaseTransport.hxx"
#include "resip/stack/ssl/TlsConnection.hxx"
#include "resip/stack/ssl/Security.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

using namespace std;
using namespace resip;

TlsBaseTransport::TlsBaseTransport(Fifo<TransactionMessage>& fifo, 
                           int portNum, 
                           IpVersion version,
                           const Data& interfaceObj,
                           Security& security,
                           const Data& sipDomain, 
                           SecurityTypes::SSLType sslType,
                           TransportType transportType,
                           AfterSocketCreationFuncPtr socketFunc,
                           Compression &compression,
                           unsigned transportFlags,
                           SecurityTypes::TlsClientVerificationMode cvm,
                           bool useEmailAsSIP,
                           const Data& certificateFilename, 
                           const Data& privateKeyFilename,
                           const Data& privateKeyPassPhrase) :
   TcpBaseTransport(fifo, portNum, version, interfaceObj, socketFunc, compression, transportFlags),
   mSecurity(&security),
   mSslType(sslType),
   mDomainCtx(0),
   mClientVerificationMode(cvm),
   mUseEmailAsSIP(useEmailAsSIP)
{
   setTlsDomain(sipDomain);   
   mTuple.setType(transportType);

   init();

   // If we have specified a sipDomain, then we need to create a new context for this domain,
   // otherwise we will use the SSL Ctx or TLS Ctx created in the Security class
   if(!sipDomain.empty())
   {
      switch(sslType)
      {
      case SecurityTypes::SSLv23:
         DebugLog(<<"Using SSLv23_method");
         mDomainCtx = mSecurity->createDomainCtx(SSLv23_method(), sipDomain, certificateFilename, privateKeyFilename, privateKeyPassPhrase);
         break;
      case SecurityTypes::TLSv1:
         DebugLog(<<"Using TLSv1_method");
         mDomainCtx = mSecurity->createDomainCtx(TLSv1_method(), sipDomain, certificateFilename, privateKeyFilename, privateKeyPassPhrase);
         break;
      default:
         throw invalid_argument("Unrecognised SecurityTypes::SSLType value");
      }
   }
}


TlsBaseTransport::~TlsBaseTransport()
{
   if (mDomainCtx)
   {
      SSL_CTX_free(mDomainCtx);mDomainCtx=0;
   }
}

SSL_CTX* 
TlsBaseTransport::getCtx() const 
{ 
   if(mDomainCtx)
   {
      DebugLog(<<"Using TlsDomain-transport SSL_CTX");
      return mDomainCtx;
   }
   else if(mSslType == SecurityTypes::SSLv23)
   {
      DebugLog(<<"Using SSLv23_method");
      return mSecurity->getSslCtx();
   }
   DebugLog(<<"Using TLSv1_method");
   return mSecurity->getTlsCtx();
}

bool
TlsBaseTransport::setPeerCertificateVerificationCallback(
   SecurityTypes::SSLVendor vendor, void *func, void *arg)
{
   // Only OpenSSL is supported at present
   if(vendor != SecurityTypes::OpenSSL)
   {
      ErrLog(<<"refusing to set SSL callback for unknown SSL stack vendor");
      return false;
   }

   // For full details of this callback see:
   // https://www.openssl.org/docs/ssl/SSL_CTX_set_cert_verify_callback.html
   SSL_CTX_set_cert_verify_callback(getCtx(),
      (int (*)(X509_STORE_CTX *,void *))func, arg);

   return true;
}

Connection* 
TlsBaseTransport::createConnection(const Tuple& who, Socket fd, bool server)
{
   resip_assert(this);
   Connection* conn = new TlsConnection(this,who, fd, mSecurity, server,
                                        tlsDomain(), mSslType, mCompression );
   return conn;
}

#endif /* USE_SSL */

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
