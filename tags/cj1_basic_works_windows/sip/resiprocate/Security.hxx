#if defined(USE_SSL) && !defined(SECURITY_HXX)
#define SECURITY_HXX
namespace Vocal2
{
 
class Body;

class Security
{
   public:
      Security( );
      ~Security();
      
      /* The security stuff is for managing certifaces that are used by the
       * S/MIME and TLS stuff. Filenames can be provided for all these items but
       * the default asumption if no filename is provided is that the itme can
       * be loaded from the default location. The default location is a file in
       * the director ~/.certs on UNIX machines and certs in the directory where
       * the application is installed in Windows. The format of the files is
       * assumed to be PEM but pkcs7 and pkcs12 may also be supported. 
       *
       * The root certificates are in root.pem.
       * The public key for this user or proxy is in id.pem
       * The private key for this user or proxy is in id_key.pem
       * Any other .pem files are assumed to be other users public certs.
       *
       */

      /* load function return true if they worked, false otherwise */
      bool loadAllCerts( const Data& password, char* directoryPath=NULL );
      bool loadRootCerts( char* filePath=NULL );

      bool loadMyPublicCert( char* filePath=NULL );
      bool loadMyPrivateKey(  const Data& password, char* filePath=NULL );
      bool createMyKey( const Data& password, 
                        char* filePathPublicCert=NULL, char* filePathPrivateKey=NULL  );

      bool loadPublicCert( char* filePath=NULL );
      bool savePublicCert( const Data& certName, char* filePath=NULL );
      
      /* stuff for importing certificates from other formats */
      bool importPkcs12Info( char* filePathForP12orPFX , const Data& password, 
                             char* filePathPublicCert=NULL, char* filePathPrivateKey=NULL  );
      bool importPkcs7Info(  char* filePathForP7c, char* filePathCert=NULL);

      /* stuff for exporting certificates to other formats */
      bool exportPkcs12Info( char* filePathForP12orPFX , const Data& password, 
                             char* filePathPublicCert=NULL, char* filePathPrivateKey=NULL  );
      bool exportMyPkcs7Info(  char* filePathForP7c );
      bool exportPkcs7Info(  char* filePathForP7c, char* filePathCert=NULL );
      bool exportPkcs7Info(  char* filePathForP7c, const Data& recipCertName );

      
      /* stuff to build messages 
       *  This is pertty straight forwartd - use ONE of the functions below to
       *  form a new body. */
      Body* sign( Body* );
      Body* encrypt( Body* , const Data& recipCertName );
      Body* signAndEncrypt( Body* , const Data& recipCertName );
      
      /* stuff to receive messages 
      *    This is a bit more complex - first operationNeeded is called to find
      *    out the state of the body. If it is a newGoodSignatuer, either
      *    addCertifiace or addAndSaveCertificate should be called. If it is a
      *    badSignature, then then  getCertifiaceInfo should be called and the
      *    user should be asked if they want to trust this certificate. If they
      *    do then then addANdSaveCertificate is called. At this point, uncode
      *    should be called. This will decrypt the message if needed and will
      *    also unwrap any signatures. It will fail if the signatures are not
      *    trusted. */ 
      enum OperationNeeded {
         isPlain, // Body is not signed or encrypted
         isSigned, // It is signed with trusted signature 
         isEncrypted, // It is a valid encryped 
         isSignedAndEncrypted, // it is signed by trusted party and encrypted
                               // for us
         badEncryption, // decrytpion failed - may have been for someone else 
         newGoodSignature, // signature is new and is signed by a root we trust 
         untrustedSignature, // signature is new and is not signed by someone we
                             // trust 
         badSignature // signature is not valid
      };
      OperationNeeded operationNeeded( Body* );
      struct CertificateInfo 
      {
            char name[1024];
            char fingerprint[1024];
            char validFrom[128];
            char validTo[128];
      };
      CertificateInfo getCertificateInfo( Body* );
      void addCertificate( Body*  ); // Just puts cert in trust list but does
                                     // not save on disk for future
                                     // sessions. You almost allways don't want
                                     // to use this but should use addAndAve
      void addAndSaveCertificate( Body*, char* filePath=NULL ); // add cert and
                                                                // saves on disk
      Body* uncode( Body* ); // returns NULL if fails 
               

   private:
      // need a map of certName to certificates

      // need a root cert list 

      // need my public cert

      // need my private key 
};


}

#endif


/* ====================================================================
 * The Vovida Software License, Version 1.0  *  * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved. *  * Redistribution and use in source and binary forms, with or without * modification, are permitted provided that the following conditions * are met: *  * 1. Redistributions of source code must retain the above copyright *    notice, this list of conditions and the following disclaimer. *  * 2. Redistributions in binary form must reproduce the above copyright *    notice, this list of conditions and the following disclaimer in *    the documentation and/or other materials provided with the *    distribution. *  * 3. The names "VOCAL", "Vovida Open Communication Application Library", *    and "Vovida Open Communication Application Library (VOCAL)" must *    not be used to endorse or promote products derived from this *    software without prior written permission. For written *    permission, please contact vocal@vovida.org. * * 4. Products derived from this software may not be called "VOCAL", nor *    may "VOCAL" appear in their name, without prior written *    permission of Vovida Networks, Inc. *  * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL, * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH * DAMAGE. *  * ==================================================================== *  * This software consists of voluntary contributions made by Vovida * Networks, Inc. and many individuals on behalf of Vovida Networks, * Inc.  For more information on Vovida Networks, Inc., please see * <http://www.vovida.org/>. *
 */
