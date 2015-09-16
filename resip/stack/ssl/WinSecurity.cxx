#include <sys/types.h>

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif


#ifdef USE_SSL
#include "resip/stack/ssl/WinSecurity.hxx"
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
#include <openssl/pkcs12.h>

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
   getCerts(WinSecurity::ROOT_CA_STORE);
   //getCerts(WinSecurity::CA_STORE);
   //getCredentials(WinSecurity::PRIVATE_STORE);
   //getCerts(WinSecurity::USERS_STORE);   
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

static const Data storeRootCA("Root");
static const Data storeCA("CA");
static const Data storePrivate("My");
static const Data storeUsers("DOMAIN_USERS");
static const Data storeUnknown("UNKNOWN_STORE");
static const Data 
certStoreTypes(  WinSecurity::MsCertStoreType pType )
{
   switch (pType)
   {
      case  WinSecurity::ROOT_CA_STORE:         return storeRootCA;
      case  WinSecurity::CA_STORE:              return storeCA;
      case  WinSecurity::PRIVATE_STORE:         return storePrivate;
      case  WinSecurity::USERS_STORE:           return storeUsers;
      default:
      {
         ErrLog( << "Some unknown certificate store type requested" << (int)(pType) );
         resip_assert(0);
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
      resip_assert(0);
      return NULL;
   }
   //mStoreHandle = ::CertOpenStore(
   //                    CERT_STORE_PROV_SYSTEM, 
   //                    0, 
   //                    0,
   //                    dwFlags, 
   //                    storeName
   //                );
   mStoreHandle = ::CertOpenSystemStore(0, name.c_str());
#ifdef UNICODE
   LocalFree((HLOCAL)storeName);
#endif

   if(NULL == mStoreHandle)
   {
      ErrLog( << name.c_str() << " system certificate store cannot be openned");
      resip_assert(0);
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
      InfoLog(<< "WinSecurity::getCerts: reading certs from store type=" << certStoreTypes(eType));

      PCCERT_CONTEXT   pCertContext = NULL;  
      while((pCertContext = ::CertEnumCertificatesInStore(storeHandle, pCertContext)) != NULL)
      {
         Data certDER(Data::Borrow, (const char*)pCertContext->pbCertEncoded, pCertContext->cbCertEncoded);
         addCertDER (BaseSecurity::RootCert, NULL, certDER, false);

         /*
         char pszNameString[256] = {0};
         CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, pszNameString, sizeof(pszNameString));
         char pszIssuerString[256] = {0};
         CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL, pszIssuerString, sizeof(pszIssuerString));
         char pszFriendlyNameString[256] = {0};
         CertGetNameString(pCertContext, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL, pszFriendlyNameString, sizeof(pszFriendlyNameString));
         char pszRDNString[256] = {0};
         DWORD dwStrType = CERT_X500_NAME_STR;
         //DWORD dwStrType =  CERT_SIMPLE_NAME_STR;
         CertGetNameString(pCertContext, CERT_NAME_RDN_TYPE, 0, &dwStrType, pszRDNString, sizeof(pszRDNString));

         DebugLog(<< "[" << i+1 << "] WinSecurity::getCerts: CertificateInfo: Version=" << pCertContext->pCertInfo->dwVersion);
         DebugLog(<< "     Name=" << pszNameString);
         DebugLog(<< "     Issuer=" << pszIssuerString);
         DebugLog(<< "     FriendlyName=" << pszFriendlyNameString);
         DebugLog(<< "     Subject=" << pszRDNString); */
         i++;
      }
      CertFreeCertificateContext(pCertContext);
   }
   InfoLog( << i << " certs loaded of type " << eType );
   closeCertifStore(storeHandle);
}

void 
WinSecurity::getCredentials(MsCertStoreType eType)
{
    //retrieves both certificates and assocaited private keys
    //retrive only certificates
    HCERTSTORE storeHandle = NULL;
    storeHandle = openSystemCertStore(certStoreTypes(eType));
    int i = 0;
    int ip = 0;
    if(NULL != storeHandle)
    {
        PCCERT_CONTEXT   pCertContext = NULL;  
        while((pCertContext = ::CertEnumCertificatesInStore(storeHandle, pCertContext)) != NULL)
        {
            Data certDER(Data::Borrow, (const char*)pCertContext->pbCertEncoded, pCertContext->cbCertEncoded);
            addCertDER (BaseSecurity::DomainCert, NULL, certDER, true);

            char pszNameString[256] = {0};
            CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, pszNameString, sizeof(pszNameString));
            char pszIssuerString[256] = {0};
            CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL, pszIssuerString, sizeof(pszIssuerString));
            char pszFriendlyNameString[256] = {0};
            CertGetNameString(pCertContext, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL, pszFriendlyNameString, sizeof(pszFriendlyNameString));
            char pszRDNString[256] = {0};
            DWORD dwStrType = CERT_X500_NAME_STR;
            //DWORD dwStrType =  CERT_SIMPLE_NAME_STR;
            CertGetNameString(pCertContext, CERT_NAME_RDN_TYPE, 0, &dwStrType, pszRDNString, sizeof(pszRDNString));

            DebugLog(<< "[" << i+1 << "] WinSecurity::getCerts: CertificateInfo: Version=" << pCertContext->pCertInfo->dwVersion);
            DebugLog(<< "     Name=" << pszNameString);
            DebugLog(<< "     Issuer=" << pszIssuerString);
            DebugLog(<< "     FriendlyName=" << pszFriendlyNameString);
            DebugLog(<< "     Subject=" << pszRDNString);
            i++;

            DWORD dwKeySpec;
            HCRYPTPROV hCryptProv;
            BOOL bCallerFreeProvOrNCryptKey;
            //get private key
            BOOL bRet = CryptAcquireCertificatePrivateKey(
                pCertContext,
                0,
                NULL,
                &hCryptProv,
                &dwKeySpec,
                &bCallerFreeProvOrNCryptKey);
            if (!bRet)
            {
                DWORD dwRet = GetLastError();
                if(dwRet == CRYPT_E_NO_KEY_PROPERTY)
                {
                    ErrLog( << " Cannot retrieve private key - non present!");
                }
                else
                {
                    ErrLog( << " Cannot retrieve private key, dwRet=" << dwRet);
                }
            }
            else
            {
                DebugLog(<< "     PrivateKey=Loaded!");
                ip++;

                HCERTSTORE  hMemoryStore;
                if(hMemoryStore = CertOpenStore(
                        CERT_STORE_PROV_MEMORY,    // Memory store
                        0,                         // Encoding type
                                                   // not used with a memory store
                        NULL,                      // Use the default provider
                        0,                         // No flags
                        NULL))                     // Not needed
                {
                    printf("Opened a memory store for private key export. \n");

                    //-------------------------------------------------------------------
                    // Add the certificate from the My store to the new memory store.
                    if(CertAddCertificateContextToStore(
                        hMemoryStore,                // Store handle
                        pCertContext,                // Pointer to a certificate
                        CERT_STORE_ADD_USE_EXISTING,
                        NULL))
                    {
                        printf("Certificate added to the memory store. \n");

                        CRYPT_DATA_BLOB dataBlob = {0};
                        LPCWSTR password = L""; // your password for the cretificate and private key goes here

                        if(PFXExportCertStoreEx(hMemoryStore, &dataBlob, password, NULL,
                            EXPORT_PRIVATE_KEYS | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY | REPORT_NO_PRIVATE_KEY))
                        {
                            if (dataBlob.cbData > 0)
                            {
                                dataBlob.pbData = (BYTE*)malloc(dataBlob.cbData);
                                if (PFXExportCertStoreEx(hMemoryStore, &dataBlob, password, NULL,
                                    EXPORT_PRIVATE_KEYS | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY | REPORT_NO_PRIVATE_KEY))
                                {
                                    EVP_PKEY *pkey;
                                    X509 *cert;
                                    STACK_OF(X509) *ca = NULL;
                                    PKCS12 *p12;
                                    //int i;
                                    //CRYPTO_malloc_init();
                                    //OpenSSL_add_all_algorithms();
                                    //SSLeay_add_all_algorithms();
                                    //ERR_load_crypto_strings();

                                    BIO* input = BIO_new_mem_buf((void*)dataBlob.pbData, dataBlob.cbData);
                                    p12 = d2i_PKCS12_bio(input, NULL);

                                    PKCS12_parse(p12, "" /* password */, &pkey, &cert, &ca);
                                    PKCS12_free(p12);

                                    if (pkey)
                                    {
                                        BIO *bo = BIO_new( BIO_s_mem() );
                                        PEM_write_bio_PrivateKey(bo, pkey, NULL, (unsigned char *)"" /* password */, 0, NULL, (char*)"" /* password */);

                                        char *p;
                                        long len = BIO_get_mem_data(bo, &p);

                                        Data keyPEM(Data::Borrow, (const char*)p, len);
                                        addDomainPrivateKeyPEM("test", keyPEM);

                                        BIO_free_all(bo);
                                    }
                                    free(dataBlob.pbData);
                                }
                            }
                        }
                        else
                        {
                            DWORD dwRet = GetLastError();
                            ErrLog( << " Could not PFXExportCertStoreEx, dwRet=" << dwRet);
                        }
                    }
                    else
                    {
                        DWORD dwRet = GetLastError();
                        ErrLog( << " Could not add the certificate to the memory store, dwRet=" << dwRet);
                    }

                    CertCloseStore(hMemoryStore, CERT_CLOSE_STORE_CHECK_FLAG);
                }
                else
                {
                    DWORD dwRet = GetLastError();
                    ErrLog( << " Cannot open cert store, dwRet=" << dwRet);
                }

                if(bCallerFreeProvOrNCryptKey)
                {
#if(_WIN32_WINNT >= 0x0600)
                    if(dwKeySpec == CERT_NCRYPT_KEY_SPEC)
                    {
                        NCryptFreeObject(hCryptProv);
                    }
                    else
                    {
                        CryptReleaseContext(hCryptProv, 0);
                    }
#else
                    CryptReleaseContext(hCryptProv, 0);
#endif // if(_WIN32_WINNT >= 0x0600)
                }
            }
        }
    }
    InfoLog( << i << " certs loaded of type " << eType << " (" << ip << " private keys loaded)");
    closeCertifStore(storeHandle);
}

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
