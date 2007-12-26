#include "resip/stack/MacSecurity.hxx"
#include "rutil/Logger.hxx"

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
   // load the root certificates
   getCerts();
}

// Opens a search handle to certificates store in
// the X509Anchors keychain
KeychainHandle
MacSecurity::openSystemCertStore()
{
   OSStatus status = noErr;

   // The ROOT certificates we're interested in are stored
   // in the X509Anchors keychain

   // NOTE: instead of hardcoding the "/System" portion of the path
   // we could retrieve it using ::FSFindFolder instead. But it
   // doesn't seem useful right now.
   SecKeychainRef systemCertsKeyChain;
   status = ::SecKeychainOpen(
      "/System/Library/Keychains/X509Anchors",
      &systemCertsKeyChain
   );

   if (status != noErr)
   {
      ErrLog( << "X509Anchors keychain could not be opened");
      assert(0);
      return NULL;
   }

   // Create a handle to search that iterates over root certificates
   // in the X509Anchors keychain

   SecKeychainSearchRef searchReference = nil;
   status = ::SecKeychainSearchCreateFromAttributes(
      systemCertsKeyChain,
      kSecCertificateItemClass,
      NULL,
      &searchReference
   );
   
   // Now that we have the search handle we don't need an explicit
   // reference to the keychain
   
   ::CFRelease(systemCertsKeyChain);
   
   if (status != noErr)
   {      
      ErrLog( << "System certificate store cannot be opened");
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
   searchReference = (SecKeychainSearchRef) openSystemCertStore();

   // nothing to do, error already reported
   if (searchReference == NULL)
      return;
   
   // iterate over each certificate
   for (;;)
   {
      OSStatus status = noErr;
      SecKeychainItemRef itemRef = nil;
      
      // get the next certificate in the search
      status = ::SecKeychainSearchCopyNext(
         searchReference,
         &itemRef
      );
      if (status == errSecItemNotFound)
      {
         // no more certificates left
         break;
      }      

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
            addCertDER(BaseSecurity::RootCert, NULL, certDER, false);
            
            status = ::SecKeychainItemFreeAttributesAndData(NULL, data);
         }
      }
      
      // free the certificate handle
      if (itemRef != NULL)
         ::CFRelease(itemRef);
      
      if (status != noErr)
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
