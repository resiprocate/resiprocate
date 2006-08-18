#if !defined(RESIP_SECURITY_HXX)
#define RESIP_SECURITY_HXX


#include <map>
#include <vector>

#include "rutil/Socket.hxx"
#include "rutil/BaseException.hxx"
#include "resip/stack/SecurityTypes.hxx"
#include "resip/stack/SecurityAttributes.hxx"

#if defined(USE_SSL)
#include <openssl/ssl.h>
#else
// to ensure compilation and object size invariance.
typedef void BIO;
typedef void SSL;
typedef void X509;
typedef void X509_STORE;
typedef void SSL_CTX;
typedef void EVP_PKEY;
#endif

namespace resip
{

class Contents;
class Pkcs7Contents;
class Security;
class MultipartSignedContents;
class SipMessage;


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
      Data getDomainPrivateKeyPEM(const Data& domainName) const;

      void addUserCertPEM(const Data& aor, const Data& certPEM);
      void addUserCertDER(const Data& aor, const Data& certDER);
      bool hasUserCert(const Data& aor) const;
      void removeUserCert(const Data& aor);
      Data getUserCertDER(const Data& aor) const;

      void setUserPassPhrase(const Data& aor, const Data& passPhrase);
      bool hasUserPassPhrase(const Data& aor) const;
      void removeUserPassPhrase(const Data& aor);
      Data getUserPassPhrase(const Data& aor) const;

      void addUserPrivateKeyPEM(const Data& aor, const Data& certPEM);
      void addUserPrivateKeyDER(const Data& aor, const Data& certDER);
      bool hasUserPrivateKey(const Data& aor) const;
      void removeUserPrivateKey(const Data& aor);
      Data getUserPrivateKeyPEM(const Data& aor) const;
      Data getUserPrivateKeyDER(const Data& aor) const;

      void generateUserCert(const Data& aor, int expireDays=365, int keyLen=1024);

      // Produces a detached signature
      MultipartSignedContents* sign(const Data& senderAor, Contents* );
      Pkcs7Contents* encrypt(Contents* , const Data& recipCertName );
      MultipartSignedContents* signAndEncrypt( const Data& senderAor, Contents* , const Data& recipCertName );

      Data computeIdentity( const Data& signerDomain, const Data& in ) const;
      bool checkIdentity( const Data& signerDomain, const Data& in, const Data& sig, X509* cert=NULL ) const;

      void checkAndSetIdentity( const SipMessage& msg, const Data& derCert=Data::Empty ) const;

      // returns NULL if it fails
      Contents* decrypt( const Data& decryptorAor, const Pkcs7Contents* );
      
      // returns NULL if fails. returns the data that was originally signed
      Contents* checkSignature( MultipartSignedContents*, 
                                Data* signedBy, SignatureStatus* sigStat );

      //returns SubjectAltName or commonName, if subjectAltName does not exist
      Data getCertName(X509 *cert);

      //compares (with wildcards) the hostname with the
      //subjectAltName/commonName from the 'cert' certificate
      bool compareCertName(X509 *cert, const Data& hostname);

      bool isSelfSigned(X509* cert);

      // allow particular classes to acces the fucntions below 
      // friend class TlsConnection;
   public:
      SSL_CTX*       getTlsCtx ();
      SSL_CTX*       getSslCtx ();
      
      X509*     getDomainCert( const Data& domain );
      EVP_PKEY* getDomainKey(  const Data& domain );
      X509*     getUserCert(const Data& aor);
      EVP_PKEY* getUserPrivateKey(const Data& aor);

      // map of name to certificates
      typedef std::map<Data,X509*>     X509Map;
      typedef std::map<Data,EVP_PKEY*> PrivateKeyMap;
      typedef std::map<Data,Data>      PassPhraseMap;

   protected:
      SSL_CTX*       mTlsCtx;
      SSL_CTX*       mSslCtx;
      static void dumpAsn(char*, Data);

      // root cert list
      mutable X509_STORE*    mRootTlsCerts;
      mutable X509_STORE*    mRootSslCerts;

      mutable X509Map        mDomainCerts;
      mutable PrivateKeyMap  mDomainPrivateKeys;

      mutable X509Map        mUserCerts;
      mutable PassPhraseMap  mUserPassPhrases;
      mutable PrivateKeyMap  mUserPrivateKeys;

      void addCertPEM (PEMType type, const Data& name, const Data& certPEM, bool write) const;
      void addCertDER (PEMType type, const Data& name, const Data& certDER, bool write) const;
      bool hasCert    (PEMType type, const Data& name) const;
      void removeCert (PEMType type, const Data& name);
      Data getCertDER (PEMType type, const Data& name) const;
      void addCertX509(PEMType type, const Data& name, X509* cert, bool write) const;

      void addPrivateKeyPEM (PEMType type, const Data& name, const Data& privateKeyPEM, bool write) const;
      void addPrivateKeyDER (PEMType type, const Data& name, const Data& privateKeyDER, bool write) const;
      bool hasPrivateKey    (PEMType type, const Data& name) const;
      void removePrivateKey (PEMType type, const Data& name);
      Data getPrivateKeyPEM (PEMType type, const Data& name) const;
      Data getPrivateKeyDER (PEMType type, const Data& name) const;
      void addPrivateKeyPKEY(PEMType type, const Data& name, EVP_PKEY* pKey, bool write) const;
};

class Security : public BaseSecurity
{
   public:
      Security(const Data& pathToCerts, const CipherList& = ExportableSuite);
      Security(const CipherList& = ExportableSuite);

      virtual void preload();

      virtual void onReadPEM(const Data& name, PEMType type, Data& buffer) const;
      virtual void onWritePEM(const Data& name, PEMType type, const Data& buffer) const;
      virtual void onRemovePEM(const Data& name, PEMType type) const;

   private:
      Data mPath;
};

}

#endif

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
