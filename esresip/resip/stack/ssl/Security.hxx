/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_SECURITY_HXX)
#define RESIP_SECURITY_HXX

#include <map>
#include <vector>
#include <set>
#include <list>

#include "rutil/Socket.hxx"
#include "rutil/BaseException.hxx"
#include "resip/stack/SecurityTypes.hxx"
#include "resip/stack/SecurityAttributes.hxx"

// If USE_SSL is not defined, Security will not be built, and this header will 
// not be installed. If you are including this file from a source tree, and are 
// getting link errors, the source tree was probably built without USE_SSL.
//#if defined(USE_SSL)
//#else
//// to ensure compilation and object size invariance.
//typedef void BIO;
//typedef void SSL;
//typedef void X509;
//typedef void X509_STORE;
//typedef void SSL_CTX;
//typedef void EVP_PKEY;
//#endif

#include <openssl/ssl.h>

namespace resip
{

class Contents;
class Pkcs7Contents;
class Security;
class MultipartSignedContents;
class SipMessage;

/**
   @internal
*/
class BaseSecurity
{
   public:
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line);
            const char* name() const { return "SecurityException"; }
      };

      class CipherList
      {
         public:
            CipherList(){}
            CipherList(const Data& cipherList) : mCipherList(cipherList) {}            
            Data cipherList() const { return mCipherList; }
         private:
            Data mCipherList;
      };

      public:

      typedef enum 
      {
         SubjectAltName,
         CommonName
      }NameType;

      struct PeerName
      {
         NameType mType;
         Data mName;
         PeerName(NameType type, Data name): mType(type), mName(name){}
      };
      
      static CipherList ExportableSuite;
      static CipherList StrongestSuite;
      
      BaseSecurity(const CipherList& cipherSuite = ExportableSuite);
      virtual ~BaseSecurity();

      // used to initialize the openssl library
      static void initialize();

      typedef enum
      {
         RootCert=1,
         DomainCert,
         DomainPrivateKey,
         UserCert,
         UserPrivateKey
      } PEMType;

      virtual void preload()=0;

      // name refers to the domainname or username which could be converted to a
      // filename by convention
      virtual void onReadPEM(const Data& name, PEMType type, Data& buffer) const =0;
      virtual void onWritePEM(const Data& name, PEMType type, const Data& buffer) const =0;
      virtual void onRemovePEM(const Data& name, PEMType type) const =0;

      struct CertificateInfo
      {
            Data name;
            Data fingerprint;
            Data validFrom;
            Data validTo;
      };

      typedef std::vector<CertificateInfo> CertificateInfoContainer;
      CertificateInfoContainer getRootCertDescriptions() const;

      // All of these guys can throw SecurityException

      void addRootCertPEM(const Data& x509PEMEncodedRootCerts);

      void addDomainCertPEM(const Data& domainName, const Data& certPEM);
      void addDomainCertDER(const Data& domainName, const Data& certDER);
      bool hasDomainCert(const Data& domainName) const;
      void removeDomainCert(const Data& domainName);
      Data getDomainCertDER(const Data& domainName) const;

      void addDomainPrivateKeyPEM(const Data& domainName, const Data& privateKeyPEM);
      bool hasDomainPrivateKey(const Data& domainName) const;
      void removeDomainPrivateKey(const Data& domainName);
// !bwc! This function calls getPrivateKeyPEM, which was never finished.
//      Data getDomainPrivateKeyPEM(const Data& domainName) const;

      void addUserCertPEM(const Data& userAtHost, const Data& certPEM);
      void addUserCertDER(const Data& userAtHost, const Data& certDER);
      bool hasUserCert(const Data& userAtHost) const;
      void removeUserCert(const Data& userAtHost);
      Data getUserCertDER(const Data& userAtHost) const;

      void setUserPassPhrase(const Data& userAtHost, const Data& passPhrase);
      bool hasUserPassPhrase(const Data& userAtHost) const;
      void removeUserPassPhrase(const Data& userAtHost);
      Data getUserPassPhrase(const Data& userAtHost) const;

      void addUserPrivateKeyPEM(const Data& userAtHost, const Data& certPEM);
      void addUserPrivateKeyDER(const Data& userAtHost, const Data& certDER);
      bool hasUserPrivateKey(const Data& userAtHost) const;
      void removeUserPrivateKey(const Data& userAtHost);
// !bwc! These functions call stuff that was never finished
//      Data getUserPrivateKeyPEM(const Data& userAtHost) const;
//      Data getUserPrivateKeyDER(const Data& userAtHost) const;

      void generateUserCert(const Data& userAtHost, int expireDays=365, int keyLen=1024);

      // Produces a detached signature
      MultipartSignedContents* sign(const Data& senderAor, Contents* );
      Pkcs7Contents* encrypt(Contents* , const Data& recipCertName );
      MultipartSignedContents* signAndEncrypt( const Data& senderAor, Contents* , const Data& recipCertName );

      Data computeIdentity( const Data& signerDomain, const Data& in ) const;
      bool checkIdentity( const Data& signerDomain, const Data& in, const Data& sig, X509* cert=NULL ) const;

      void checkAndSetIdentity(SipMessage& msg, const Data& derCert=Data::Empty ) const;

      // returns NULL if it fails
      Contents* decrypt( const Data& decryptorAor, const Pkcs7Contents* );
      
      // returns NULL if fails. returns the data that was originally signed
      Contents* checkSignature( MultipartSignedContents*, 
                                Data* signedBy, SignatureStatus* sigStat );
      // returns the first SubjectAltName or commonName, if subjectAltName does not exist
      static Data getCertName(X509 *cert);

      // retrieves a list of all certificate names (subjectAltNAme's and CommonName)
      static void getCertNames(X509 *cert, std::list<PeerName> &peerNames);

      static bool isSelfSigned(const X509* cert);

      // match with wildcards
      static int matchHostName(const Data& certificateName, const Data& domainName);

      // allow particular classes to acces the functions below 
      // friend class TlsConnection;
   public:
      SSL_CTX*       getTlsCtx ();
      SSL_CTX*       getSslCtx ();
      
      X509*     getDomainCert( const Data& domain );
      EVP_PKEY* getDomainKey(  const Data& domain );
      X509*     getUserCert(const Data& userAtHost);
      EVP_PKEY* getUserPrivateKey(const Data& userAtHost);

      resip::Data getSystemCAFile() const;
      resip::Data getSystemKeyPath() const;
      resip::Data getSystemCertPath() const;

      void getCertsAndKeys(const resip::Data& path, 
                           std::vector<std::pair<X509*, EVP_PKEY*> >& certsAndKeys,
                           bool skipOldResipLoad=true,
                           bool ignoreKeys=false);

      bool isProbableCertKeyFile(const resip::Data& name) const;

      bool oldResipLoad(const resip::Data& path,
                        const resip::Data& name);

      // .bwc. Reads the entirety of a single PEM file, and attempts to match 
      // any cert/key pairs within.
      void readPem(const resip::Data& pem, 
                     std::vector<std::pair<X509*, EVP_PKEY*> >& certsAndKeys,
                     bool ignoreKeys=false);

      void tryReadDer(const resip::Data& der,
                        std::vector<std::pair<X509*, EVP_PKEY*> >& certsAndKeys,
                        bool ignoreKeys=false);

      // .bwc. This goes through the vector searching for all unmatched 
      // certs/keys, and attempts to match them to other stuff in the vector.
      // If multiple distinct certs match a given key, we will make extra copies 
      // of the key in question, and it will appear multiple times in the vector 
      // once we're done.
      void tryMatchCertsToKeys(std::vector<std::pair<X509*, EVP_PKEY*> >& certsAndKeys);

      void appendCertsAndKeys(
                     std::vector<std::pair<X509*, EVP_PKEY*> >& certsAndKeys,
                     std::vector<std::pair<X509*, EVP_PKEY*> >& toAppend) const;

      X509* extractCertPem(const resip::Data& name,
                           const resip::Data& data) const;

      EVP_PKEY* extractKeyPem(const resip::Data& name,
                              const resip::Data& data,
                              const resip::Data& passPhrase) const;

      bool isCertKeyPair(const char* name) const;
      bool isCert(const char* name) const;
      bool isKey(const char* name) const;

      void setupIdentities(std::vector<std::pair<X509*, EVP_PKEY*> >& certsAndKeys);

      void getUserIdentities(X509* cert, std::list<resip::Data>& userIdentities) const;
      void getDomainIdentities(X509* cert, std::list<resip::Data>& domainIdentities) const;
      void getCNs(X509* cert, std::list<resip::Data>& identities) const;

      // .bwc. Values for parameter 'type' can be found in openssl/x509v3.h;
      // these are macros that start with "GEN_".
      void getSubjAltNames(X509* cert, int type, std::list<resip::Data>& res) const;

      bool looksLikeFQDN(const resip::Data& cn) const;

      // map of name to certificates
      typedef std::map<Data,X509*>     X509Map;
      typedef std::map<Data,EVP_PKEY*> PrivateKeyMap;
      typedef std::map<Data,Data>      PassPhraseMap;
      typedef std::map<Data, Data> FileChecksumMap;

   protected:
      SSL_CTX*       mTlsCtx;
      SSL_CTX*       mSslCtx;
      static void dumpAsn(char*, Data);

      // root cert list
      X509_STORE*    mRootTlsCerts;
      X509_STORE*    mRootSslCerts;

      X509Map        mDomainCerts;
      PrivateKeyMap  mDomainPrivateKeys;

      X509Map        mUserCerts;
      PassPhraseMap  mUserPassPhrases;
      PrivateKeyMap  mUserPrivateKeys;

      // .bwc. These are the structures that actually own certs and private keys
      std::set<X509*> mCertRefs;
      std::set<EVP_PKEY*> mKeyRefs;
      
      FileChecksumMap mChecksums;

      void addCertPEM (PEMType type, const Data& name, const Data& certPEM, bool write);
      void addCertDER (PEMType type, const Data& name, const Data& certDER, bool write);
      bool hasCert    (PEMType type, const Data& name) const;
      void removeCert (PEMType type, const Data& name);
      Data getCertDER (PEMType type, const Data& name) const;
      void addCertX509(PEMType type, const Data& name, X509* cert, bool write);

      void addPrivateKeyPEM (PEMType type, const Data& name, const Data& privateKeyPEM, bool write);
      void addPrivateKeyDER (PEMType type, const Data& name, const Data& privateKeyDER, bool write);
      bool hasPrivateKey    (PEMType type, const Data& name) const;
      void removePrivateKey (PEMType type, const Data& name);
// !bwc! These functions were never finished.
//      Data getPrivateKeyPEM (PEMType type, const Data& name) const;
//      Data getPrivateKeyDER (PEMType type, const Data& name) const;
      void addPrivateKeyPKEY(PEMType type, const Data& name, EVP_PKEY* pKey, bool write);
};

/**
   @ingroup resip_config
   @brief Encapsulates certs and keys. Passed as an argument to 
      SipStack::SipStack().
*/
class Security : public BaseSecurity
{
   public:
      Security(const Data& pathToCerts, const CipherList& = ExportableSuite);
      Security(const CipherList& = ExportableSuite);

      ///@brief Attempts to load server-side certificates.
      virtual void preload();
      
      ///@brief Called when reading a PEM file.  Can be overridden if PEM files won't be stored as files on disk.
      ///
      /// @param name entity whose cert is being retrieved, in the form of user@host
      /// @param type cert type to load
      /// @param buffer buffer to place the cert into once it's been read
      virtual void onReadPEM(const Data& name, PEMType type, Data& buffer) const;
      ///@brief Called when write a PEM file.  Can be overridden if PEM files won't be stored as files on disk.
      ///
      /// @param name entity whose cert is being retrieved, in the form of user@host
      /// @param type cert type to load
      /// @param buffer buffer containing the cert to be written
      virtual void onWritePEM(const Data& name, PEMType type, const Data& buffer) const;
      ///@brief not called yet
      virtual void onRemovePEM(const Data& name, PEMType type) const;

   private:
      Data mPath;
};

}

#endif

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
