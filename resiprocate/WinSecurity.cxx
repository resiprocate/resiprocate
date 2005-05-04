#include "resiprocate/WinSecurity.hxx"
#include <sys/types.h>
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
#include <Wincrypt.h>
#include "resiprocate/os/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

#include <windows.h>
#include <wincrypt.h>

void 
WinSecurity::preload()
{
   HCERTSTORE storeHandle = NULL;

   getCerts(WinSecurity::ROOT_CA_STORE);
   //getCerts(WinSecurity::CA_STORE);
   //getCredentials(WinSecurity::PRIVATE_STORE);
   //getCerts(WinSecurity::USERS_STORE);   
}

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
   if(NULL != storeHandle)
   {
      PCCERT_CONTEXT   pCertContext = NULL;  
      while((pCertContext = ::CertEnumCertificatesInStore(storeHandle, pCertContext)) != NULL)
      {
         Data certDER(Data::Borrow, (const char*)pCertContext->pbCertEncoded, pCertContext->cbCertEncoded);
         addCertDER (BaseSecurity::RootCert, NULL, certDER, false);
      }
      CertFreeCertificateContext(pCertContext);
   }
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
