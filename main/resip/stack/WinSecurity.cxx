#include "resip/stack/WinSecurity.hxx"
#include <sys/types.h>

#ifdef USE_SSL
#include <openssl/e_os2.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/pkcs7.h>
#include <openssl/ossl_typ.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>
#endif

#include <Wincrypt.h>
#include "rutil/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

#include <windows.h>
#include <wincrypt.h>

void 
WinSecurity::preload()
{
   HCERTSTORE storeHandle = NULL;

#ifdef USE_SSL
   getCerts(WinSecurity::ROOT_CA_STORE);
   //getCerts(WinSecurity::CA_STORE);
   //getCredentials(WinSecurity::PRIVATE_STORE);
   //getCerts(WinSecurity::USERS_STORE);   
#endif
}

void
WinSecurity::onReadPEM(const Data& name, PEMType type, Data& buffer) const
{
   return;
}

void
WinSecurity::onWritePEM(const Data& name, PEMType type, const Data& buffer) const
{
   return;
}

void
WinSecurity::onRemovePEM(const Data& name, PEMType type) const
{
   return;
}

#ifdef USE_SSL

static const Data 
certStoreTypes(  WinSecurity::MsCertStoreType pType )
{
   static const Data storeRootCA("Root");
   static const Data storeCA("CA");
   static const Data storePrivate("My");
   static const Data storeUsers("DOMAIN_USERS");
   static const Data storeUnknown("UNKNOWN_STORE");

   switch (pType)
   {
      case  WinSecurity::ROOT_CA_STORE:         return storeRootCA;
      case  WinSecurity::CA_STORE:              return storeCA;
      case  WinSecurity::PRIVATE_STORE:         return storePrivate;
      case  WinSecurity::USERS_STORE:           return storeUsers;
      default:
      {
         ErrLog( << "Some unkown certificate store type requested" << (int)(pType) );
         assert(0);
      }
   }
   return storeUnknown;
}

#ifdef UNICODE
static LPWSTR AnsiToUnicode(LPCSTR szInString)
{
   LPWSTR pwszString = NULL;
   if(NULL == szInString))
      return 0;

int iLen = 0;
iLen = MultiByteToWideChar( CP_UTF8, 0, szInString, -1, 0, 0 );
if (0 == iLen)
   return pwszString;

      pwszString = (LPWSTR)LocalAlloc(
         LMEM_FIXED, 
         iLen * sizeof(WCHAR)
         );
if (NULL == pwszString)
   return pwszString;

int iRet = MultiByteToWideChar( CP_UTF8, 0, szInString, -1, pwszString, iLen );

if (0 == iRet)
{
   LocalFree(pwszString);
}
return pwszString;
}
#endif

HCERTSTORE 
WinSecurity::openSystemCertStore(const Data& name)
{
   HCERTSTORE mStoreHandle = NULL;
   LPCTSTR storeName = NULL;
   DWORD dwFlags;

   dwFlags = CERT_STORE_OPEN_EXISTING_FLAG | CERT_SYSTEM_STORE_LOCAL_MACHINE;

#ifndef UNICODE
   storeName = name.c_str();
#else
   storeName = AnsiToUnicode(name.c_str());
#endif

   if (NULL == storeName)
   {
      ErrLog( << " Invalid store name");
      assert(0);
      return NULL;
   }
   //mStoreHandle = ::CertOpenStore(
   //                    CERT_STORE_PROV_SYSTEM, 
   //                    0, 
   //                    0,
   //                    dwFlags, 
   //                    storeName
   //                );
   mStoreHandle = ::CertOpenSystemStore(0, "Root");
#ifdef UNICODE
   LocalFree((HLOCAL)storeName);
#endif

   if(NULL == mStoreHandle)
   {
      ErrLog( << name.c_str() << " system certificate store cannot be openned");
      assert(0);
      return NULL;
   }
   InfoLog( << name.c_str() << " System certificate store opened");
   return mStoreHandle;
}

void 
WinSecurity::closeCertifStore(HCERTSTORE storeHandle)
{
   if (NULL == storeHandle)
      return;
    
   ::CertCloseStore(storeHandle ,0);
}
void 
WinSecurity::getCerts(MsCertStoreType eType)
{
   //retrive only certificates
   HCERTSTORE storeHandle = NULL;
   storeHandle = openSystemCertStore(certStoreTypes(eType));
   int i = 0;
   if(NULL != storeHandle)
   {
      PCCERT_CONTEXT   pCertContext = NULL;  
      while((pCertContext = ::CertEnumCertificatesInStore(storeHandle, pCertContext)) != NULL)
      {
         Data certDER(Data::Borrow, (const char*)pCertContext->pbCertEncoded, pCertContext->cbCertEncoded);
         addCertDER (BaseSecurity::RootCert, NULL, certDER, false);
         i++;
      }
      CertFreeCertificateContext(pCertContext);
   }
   InfoLog( << i << " certs loaded of type " << eType );
   closeCertifStore(storeHandle);
}

/*
  void 
  WinSecurity::getCredentials(MsCertStoreType eType)
  {
  //retrieves both certificates and assocaited private keys
  //retrive only certificates
  HCERTSTORE storeHandle = NULL;
  storeHandle = openCertifStore(certStoreTypes(eType));
  if(NULL != storeHandle)
  {
  PCCERT_CONTEXT   pCertContext = NULL;  
  while(pCertContext = ::CertEnumCertificatesInStore(mStoreHandle, pCertContext) != NULL)
  {
  Data certDER(Data::Take, pCertContext->pbCertEncoded, pCertContext->cbCertEncoded);
  addCertDER (BaseSecurity::RootCert, NULL, certDER, true);
  DWORD dwKeySpec;
  HCRYPTPROV hCryptProv;
  //get private key
  BOOL bRet = CryptAcquireCertificatePrivateKey(
  pCertContext,
  0,
  NULL,
  &hCryptProv,
  &dwKeySpec,
  NULL
  );
  if (!bRet)
  {
  ErrLog( << " Cannot retrieve private key");
  }
  }
  }
  closeCertifStore(storeHandle);
  }*/

#endif // ifdef USE_SSL

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
