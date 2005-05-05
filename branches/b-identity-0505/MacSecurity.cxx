#include "resiprocate/MacSecurity.hxx"
#include "resiprocate/os/Logger.hxx"

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

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

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

int verifyCallback(int iInCode, X509_STORE_CTX *pInStore);

void
MacSecurity::preload()
{
   // TODO: this needs to be refactored with WinSecurity.cxx
   X509_STORE_set_verify_cb_func(mRootCerts, verifyCallback);
   
   // load the root certificates
   getCerts();
}

KeychainHandle
MacSecurity::openSystemCertStore(const Data& name)
{
   // Currently we only search for certificates in the
   // default key chain. Later when we support more we can
   // enable it here.
   if (!name.empty())
   {
      ErrLog( << "Certificate store " << name << " unsupported");
      assert(0);
      return NULL;
   }

   OSStatus status;
   SecKeychainSearchRef searchReference = nil;
   status = ::SecKeychainSearchCreateFromAttributes(
      NULL,
      kSecCertificateItemClass,
      NULL,
      &searchReference
   );
   
   if (status != noErr)
   {
      ErrLog( << "System certificate store cannot be openned");
      assert(0);
      return NULL;
   }

   InfoLog( << "System certificate store opened");
   return searchReference;
}

void 
MacSecurity::closeCertifStore(KeychainHandle searchReference)
{
   if (NULL == searchReference)
      return;
    
   ::CFRelease(searchReference);
}

void
MacSecurity::getCerts()
{
   SecKeychainSearchRef searchReference = NULL;
   searchReference = (SecKeychainSearchRef) openSystemCertStore(Data());

   // nothing to do, error already reported
   if (searchReference == NULL)
      return;
   
   // itterate over each certificate
   for (;;)
   {
      OSStatus status = noErr;
      SecKeychainItemRef itemRef = nil;
      
      // get the next cerfiticate in the search
      status = ::SecKeychainSearchCopyNext(
         searchReference,
         &itemRef
      );

      // get data from the certificate
      if (status == noErr)
      {        
         void *data;
         UInt32 dataSize;
         status = ::SecKeychainItemCopyAttributesAndData(
            itemRef,
            NULL,
            NULL,
            NULL,
            &dataSize,
            &data
         );
         
         if (status == noErr && data != NULL)
         {
            Data certDER(Data::Borrow, (const char*)data, dataSize);
            addCertDER (BaseSecurity::RootCert, NULL, certDER, false);
         }
         
         // TODO: need to free data using SecKeychainItemFreeAttributesAndData ??  
      }
      
      // free the certificate handle
      if (itemRef)
         CFRelease(itemRef);
      
      if (status == errSecItemNotFound)
      {
         // no more certificates left
         break;
      }
      else if (status != noErr)
      {
         // there was an error loading the certificate
         ErrLog( << "Couldn't load certificate, error code: " << status);
         assert(0);
      }
   }
   
   closeCertifStore(searchReference);
}

// TODO: this needs to be refactored with WinSecurity.cxx
int 
verifyCallback(int iInCode, X509_STORE_CTX *pInStore)
{
   char cBuf1[500];
   char cBuf2[500];
   X509 *pErrCert;
   int iErr = 0;
   int iDepth = 0;
   pErrCert = X509_STORE_CTX_get_current_cert(pInStore);
   iErr = X509_STORE_CTX_get_error(pInStore);
   iDepth =	X509_STORE_CTX_get_error_depth(pInStore);

   if (NULL != pErrCert)
      X509_NAME_oneline(X509_get_subject_name(pErrCert),cBuf1,256);

   sprintf(cBuf2,"depth=%d %s\n",iDepth,cBuf1);
   if(!iInCode)
   {
      memset(cBuf2, 0, sizeof(cBuf2) ); 
      sprintf(cBuf2, "\n Error %s", X509_verify_cert_error_string(pInStore->error) );
   }
 
   return iInCode;
}