#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifdef USE_SSL

#include "resip/stack/ssl/Security.hxx"

#include <ostream>
#include <fstream>
#include <stdexcept>

#include "resip/stack/Contents.hxx"
#include "resip/stack/MultipartSignedContents.hxx"
#include "resip/stack/Pkcs7Contents.hxx"
#include "resip/stack/PlainContents.hxx"
#include "resip/stack/SecurityAttributes.hxx"
#include "resip/stack/Transport.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/ResipAssert.h"
#include "rutil/BaseException.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Timer.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/FileSystem.hxx"
#include "rutil/WinLeakCheck.hxx"

#include "rutil/ssl/SHA1Stream.hxx"

#if !defined(WIN32)
#include <sys/types.h>
#include <sys/uio.h>
#if defined(__ANDROID__)
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <unistd.h>
#include <dirent.h>
#endif

#include <sys/types.h>
#include <openssl/opensslv.h>
#if !defined(LIBRESSL_VERSION_NUMBER)
#include <openssl/e_os2.h>
#endif
#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/pkcs7.h>
#include <openssl/ossl_typ.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L

inline const unsigned char *ASN1_STRING_get0_data(const ASN1_STRING *x)
{
    return ASN1_STRING_data(const_cast< ASN1_STRING* >(x));
}

#endif // OPENSSL_VERSION_NUMBER < 0x10100000L

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

static const Data PEM(".pem");

static const Data rootCert("root_cert_");
static const Data domainCert("domain_cert_");
static const Data domainKey("domain_key_");
static const Data userCert("user_cert_");
static const Data userKey("user_key_");
static const Data unknownKey("user_key_");

static const Data 
pemTypePrefixes(  Security::PEMType pType )
{
   switch (pType)
   {
      case  Security::RootCert:         return rootCert;
      case  Security::DomainCert:       return domainCert;
      case  Security::DomainPrivateKey: return domainKey;
      case  Security::UserCert:         return userCert;
      case  Security::UserPrivateKey:   return userKey;
      default:
      {
         ErrLog( << "Some unkonw pem type prefix requested" << (int)(pType) );
         resip_assert(0);
      }
   }
   return unknownKey;
}

static Data
getAor(const Data& filename, const  Security::PEMType &pemType )
{
   const Data& prefix = pemTypePrefixes( pemType );
   return filename.substr(prefix.size(), filename.size() - prefix.size() - PEM.size());
}

extern "C"
{
   
static int 
verifyCallback(int iInCode, X509_STORE_CTX *pInStore)
{
   char cBuf1[257];
   char cBuf2[501];
   X509 *pErrCert;
   int iErr = 0;
   int iDepth = 0;
   pErrCert = X509_STORE_CTX_get_current_cert(pInStore);
   iErr = X509_STORE_CTX_get_error(pInStore);
   iDepth = X509_STORE_CTX_get_error_depth(pInStore);

   if (NULL != pErrCert)
      X509_NAME_oneline(X509_get_subject_name(pErrCert),cBuf1,256);

   snprintf(cBuf2, 500, ", depth=%d %s\n", iDepth, cBuf1);
   if(!iInCode)
   {
      ErrLog(<< "Error when verifying peer's chain of certificates: " << X509_verify_cert_error_string(X509_STORE_CTX_get_error(pInStore)) << cBuf2 );
      DebugLog(<<"additional validation checks may have failed but only one is ever logged - please check peer certificate carefully");
   }
 
   return iInCode;
}
 
}

// .amr. RFC 5922 mandates exact match only on certificates, so this is the default, but RFC 2459 and RFC 3261 don't prevent wildcards, so enable if you want that mode.
bool BaseSecurity::mAllowWildcardCertificates = false;
BaseSecurity::CipherList BaseSecurity::ExportableSuite("HIGH:RC4-SHA:-COMPLEMENTOFDEFAULT");
BaseSecurity::CipherList BaseSecurity::StrongestSuite("HIGH:-COMPLEMENTOFDEFAULT");

/**
 * Note:
 *
 * When SSLv23 mode is selected and the options flags SSL_OP_NO_SSLv2
 * and SSL_OP_NO_SSLv3 are set, SSLv23_method() will allow a dynamic
 * choice of TLS v1.0, v1.1 or v1.2 on each connection.
 *
 * If SSL_OP_NO_SSLv3 is removed (by an application changing the value
 * of BaseSecurity::OpenSSLCTXSetOptions before instantiating
 * resip::Security) then using SSLv23_method() will allow a dynamic
 * choice of SSL v3.0 or any of the TLS versions on each connection.
 */
long BaseSecurity::OpenSSLCTXSetOptions = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3;
long BaseSecurity::OpenSSLCTXClearOptions = 0;

Security::Security(const CipherList& cipherSuite, const Data& defaultPrivateKeyPassPhrase, const Data& dHParamsFilename) :
   BaseSecurity(cipherSuite, defaultPrivateKeyPassPhrase, dHParamsFilename)
{
#ifdef WIN32
   mPath = "C:\\sipCerts\\";
#else
   const char* env=getenv("HOME");
   if(env)
   {
      mPath = env;
   }
   mPath += "/.sipCerts/";
#endif
}

Security::Security(const Data& directory, const CipherList& cipherSuite, const Data& defaultPrivateKeyPassPhrase, const Data& dHParamsFilename) :
   BaseSecurity(cipherSuite, defaultPrivateKeyPassPhrase, dHParamsFilename),
   mPath(directory)
{
   // since the preloader won't work otherwise and VERY difficult to figure out.
   if (!mPath.empty() && !mPath.postfix(Symbols::SLASH))
   {
      mPath += Symbols::SLASH;
   }
}

void
Security::addCADirectory(const Data& caDirectory)
{
   mCADirectories.push_back(caDirectory);
   Data &_dir = mCADirectories.back();
   if ( !_dir.postfix(Symbols::SLASH))
   {
      _dir += Symbols::SLASH;
   }
}

void
Security::addCAFile(const Data& caFile)
{
   mCAFiles.push_back(caFile);
}

void
Security::loadCADirectory(const Data& _dir)
{
   FileSystem::Directory dir(_dir);
   FileSystem::Directory::iterator it(dir);
   for (; it != dir.end(); ++it)
   {
      try
      {
         if (!it.is_directory())  // This can throw!
         {
            Data name = *it;
            Data fileName = _dir + name;
            loadCAFile(fileName);
         }
      }
      catch (Exception& e)
      {
         ErrLog(<< "loadCADirectory: Some problem reading " << *it << ": " << e);
      }
      catch (...)
      {
         ErrLog(<< "loadCADirectory: Some problem reading " << *it);
      }
   }
}

void
Security::loadCAFile(const Data& _file)
{
   try
   {
      addRootCertPEM(Data::fromFile(_file));
      InfoLog(<<"Successfully loaded " << _file);
   }
   catch (Exception& e)
   {
      ErrLog(<< "loadCAFile: Some problem reading " << _file << ": " << e);
   }
   catch (...)
   {
      ErrLog(<< "loadCAFile: Some problem reading " << _file);
   }
}

void
Security::preload()
{
   int count = 0;
#ifndef WIN32
   // We only do this for UNIX platforms at present
   // If no other source of trusted roots exists,
   // and if mPath is a file, check if it is a root certificate
   // or a collection of root certificates
   struct stat s;
   Data fileName(mPath);
   if(fileName.postfix("/"))
   {
      fileName.truncate(fileName.size() - 1);
   }
   if(fileName.size() > 0)
   {
      StackLog(<<"calling stat() for " << fileName);
      if(stat(fileName.c_str(), &s) < 0)
      {
         ErrLog(<<"Error calling stat() for " << fileName.c_str()
                << ": " << strerror(errno));
      }
      else
      {
         if(!S_ISDIR(s.st_mode))
         {
            WarningLog(<<"mPath argument is a file rather than a directory, "
                         "treating mPath as a file of trusted root certificates");
            loadCAFile(fileName);
            count++;
         }
      }
   }
#endif

   FileSystem::Directory dir(mPath);
   FileSystem::Directory::iterator it(dir);
   for (; it != dir.end(); ++it)
   {
      Data name = *it;
           
      if (name.postfix(PEM))
      {
         Data fileName = mPath + name;
         bool attemptedToLoad = true;
         
         DebugLog(<< "Checking to load file " << name );
         try
         {
            if (name.prefix(pemTypePrefixes(UserCert)))
            {
               addCertPEM( UserCert, getAor(name, UserCert), Data::fromFile(fileName), false );
            }
            else if (name.prefix(pemTypePrefixes(UserPrivateKey)))
            {
               addPrivateKeyPEM( UserPrivateKey, getAor(name, UserPrivateKey), Data::fromFile(fileName), false);
            }
            else if (name.prefix(pemTypePrefixes(DomainCert)))
            {
               addCertPEM( DomainCert, getAor(name, DomainCert), Data::fromFile(fileName), false);
            }
            else if (name.prefix(pemTypePrefixes(DomainPrivateKey)))
            {
               addPrivateKeyPEM( DomainPrivateKey, getAor(name, DomainPrivateKey), Data::fromFile(fileName), false);
            }
            else if (name.prefix(pemTypePrefixes(RootCert)))
            {
               addRootCertPEM(Data::fromFile(fileName));
            }
            else
            {
               DebugLog(<< "PEM file " << name << " does not have appropriate resip prefix, skipping...");
               attemptedToLoad = false;
            }
         }
         catch (Exception& e)
         {
            ErrLog(<< "Some problem reading " << fileName << ": " << e);
         }
         catch (...)
         {  
            ErrLog(<< "Some problem reading " << fileName );
         }
         
         if(attemptedToLoad)
         {
            InfoLog(<<"Successfully loaded " << fileName );
            count++;
         }
      }
   }
   InfoLog(<<"Files loaded by prefix: " << count);

   if(count == 0 && mCADirectories.empty() && mCAFiles.empty() && mPath.size() > 0)
   {
      // If no other source of trusted roots exists,
      // assume mPath was meant to be in mCADirectories
      WarningLog(<<"No root certificates found using legacy prefixes, "
                   "treating mPath as a normal directory of root certs");
      loadCADirectory(mPath);
   }

   std::list<Data>::iterator it_d = mCADirectories.begin();
   for (; it_d != mCADirectories.end(); ++it_d)
   {
      loadCADirectory(*it_d);
   }

   std::list<Data>::iterator it_f = mCAFiles.begin();
   for (; it_f != mCAFiles.end(); ++it_f)
   {
      loadCAFile(*it_f);
   }
}

// Generic password callback to copy over provided userdata/password
int pem_passwd_cb(char *buf, int size, int rwflag, void *password)
{
   if(password)
   {
      strncpy(buf, (char *)(password), size);
      buf[size - 1] = '\0';
      return (int)strlen(buf);
   }
   else
   {
       return 0;
   }
}

SSL_CTX* 
Security::createDomainCtx(const SSL_METHOD* method, const Data& domain, const Data& certificateFilename, const Data& privateKeyFilename, const Data& privateKeyPassPhrase)
{
#if (OPENSSL_VERSION_NUMBER >= 0x1000000fL )
   SSL_CTX* ctx = SSL_CTX_new(method);
#else
   SSL_CTX* ctx = SSL_CTX_new((SSL_METHOD*)method);
#endif
   resip_assert(ctx);

   X509_STORE* x509Store = X509_STORE_new();
   resip_assert(x509Store);

   // Load root certs into store
   X509List::iterator it;
   for(it = mRootCerts.begin(); it != mRootCerts.end(); it++)
   {
      X509_STORE_add_cert(x509Store,*it);
   }
   SSL_CTX_set_cert_store(ctx, x509Store);

   updateDomainCtx(ctx, domain, certificateFilename, privateKeyFilename, privateKeyPassPhrase);

   SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE, verifyCallback);
   SSL_CTX_set_cipher_list(ctx, mCipherList.cipherList().c_str());
   setDHParams(ctx);
   SSL_CTX_set_options(ctx, BaseSecurity::OpenSSLCTXSetOptions);
   SSL_CTX_clear_options(ctx, BaseSecurity::OpenSSLCTXClearOptions);

   return ctx;
}

void
Security::updateDomainCtx(SSL_CTX* ctx, const Data& domain, const Data& certificateFilename, const Data& privateKeyFilename, const Data& privateKeyPassPhrase)
{
   // Load domain cert chain and private key
   if(!domain.empty())
   {
      // Set Private Key PassPhrase
      SSL_CTX_set_default_passwd_cb(ctx, pem_passwd_cb);
      if(!privateKeyPassPhrase.empty())
      {
          SSL_CTX_set_default_passwd_cb_userdata(ctx, (void*)privateKeyPassPhrase.c_str()); 
      }
      Data certFilename(certificateFilename.empty() ? mPath + pemTypePrefixes(DomainCert) + domain + PEM : certificateFilename);
      if(SSL_CTX_use_certificate_chain_file(ctx, certFilename.c_str()) != 1)
      {
         ErrLog (<< "Error reading domain chain file " << certFilename);
         SSL_CTX_free(ctx);
         throw BaseSecurity::Exception("Failed opening PEM chain file", __FILE__,__LINE__);
      }

      // Check if we have the domain cert in the main storage yet - needed for Identity header calculations
      if(mDomainCerts.find(domain) == mDomainCerts.end())
      {
         // Add to storage
         addCertPEM( DomainCert, domain, Data::fromFile(certFilename), false);
         InfoLog(<< "Security::updateDomainCtx: Successfully loaded domain cert and added to Security storage, domain=" << domain << ", filename=" <<  certFilename);
      }
      else
      {
         InfoLog(<< "Security::updateDomainCtx: Successfully loaded domain cert, domain=" << domain << ", filename=" <<  certFilename);
      }

      Data keyFilename(privateKeyFilename.empty() ? mPath + pemTypePrefixes(DomainPrivateKey) + domain + PEM : privateKeyFilename);
      if(SSL_CTX_use_PrivateKey_file(ctx, keyFilename.c_str(), SSL_FILETYPE_PEM) != 1)
      {
         ErrLog (<< "Error reading domain private key file " << keyFilename);
         SSL_CTX_free(ctx);
         throw BaseSecurity::Exception("Failed opening PEM private key file", __FILE__,__LINE__);
      }
      if (!SSL_CTX_check_private_key(ctx))
      {
         ErrLog (<< "Invalid domain private key from file: " << keyFilename);
         SSL_CTX_free(ctx);
         throw BaseSecurity::Exception("Invalid domain private key", __FILE__,__LINE__);
      }

      // Check if we have the domain cert in the main storage yet - needed for Identity header calculations
      if(mDomainPrivateKeys.find(domain) == mDomainPrivateKeys.end())
      {
         // Add to storage
         addPrivateKeyPEM( DomainPrivateKey, domain, Data::fromFile(keyFilename), false, privateKeyPassPhrase);
         InfoLog(<< "Security::updateDomainCtx: Successfully loaded domain private key and added to Security storage, domain=" << domain << ", filename=" <<  keyFilename);
      }
      else
      {
          InfoLog(<< "Security::updateDomainCtx: Successfully loaded domain private key, domain=" << domain << ", filename=" <<  keyFilename);
      }
   }
}

void
Security::onReadPEM(const Data& name, PEMType type, Data& buffer) const
{
   Data filename = mPath + pemTypePrefixes(type) + name + PEM;

   InfoLog (<< "Reading PEM file " << filename << " into " << name);
   // .dlb. extra copy
   buffer = Data::fromFile(filename);
}

void
Security::onWritePEM(const Data& name, PEMType type, const Data& buffer) const
{
   Data filename = mPath + pemTypePrefixes(type) + name + PEM;
   InfoLog (<< "Writing PEM file " << filename << " for " << name);
   ofstream str(filename.c_str(), ios::binary);
   if (!str)
   {
      ErrLog (<< "Can't write to " << filename);
      throw BaseSecurity::Exception("Failed opening PEM file", __FILE__,__LINE__);
   }
   else
   {
      str.write(buffer.data(), buffer.size());
      if (!str)
      {
         ErrLog (<< "Failed writing to " << filename << " " << buffer.size() << " bytes");
         throw BaseSecurity::Exception("Failed writing PEM file", __FILE__,__LINE__);
      }
   }
}

void
Security::onRemovePEM(const Data& name, PEMType type) const
{
   resip_assert(0);
   // TODO - should delete file 
}

void
BaseSecurity::addCertDER (PEMType type, 
                          const Data& key, 
                          const Data& certDER, 
                          bool write)
{
   if( certDER.empty() )
   {
      ErrLog(<< "File is empty. Skipping.");
      return;
   }

   X509* cert = 0;

#if (OPENSSL_VERSION_NUMBER < 0x0090800fL )
   unsigned char* in = (unsigned char*)certDER.data();
#else
   unsigned const char* in = (unsigned const char*)certDER.data();
#endif

   if (d2i_X509(&cert,&in,(long)certDER.size()) == 0)
   {
      ErrLog(<< "Could not read DER certificate from " << certDER );
      throw BaseSecurity::Exception("Could not read DER certificate ", 
                                    __FILE__,__LINE__);
   }
   addCertX509(type,key,cert,write);
}

void
BaseSecurity::addCertPEM (PEMType type, 
                          const Data& name, 
                          const Data& certPEM, 
                          bool write)
{
   if( certPEM.empty() )
   {
      ErrLog(<< name << " is empty. Skipping.");
      return;
   }
   X509* cert=NULL;
   
   BIO* in = BIO_new_mem_buf(const_cast<char*>(certPEM.c_str()), -1);
   if ( !in )
   {
      ErrLog(<< "Could not create BIO buffer from '" << certPEM << "'");
      throw Exception("Could not create BIO buffer", __FILE__,__LINE__);
   }
   while(!BIO_eof(in))
   {
      cert = PEM_read_bio_X509(in,0,0,0);
      if (cert == NULL)
      {
         ErrLog( << "Could not load X509 cert from '" << certPEM << "'" );
         BIO_free(in);
         throw Exception("Could not load X509 cert from BIO buffer", __FILE__,__LINE__);
      }
   
      addCertX509(type,name,cert,write);
      if(type != RootCert)
      {
         break;
      }
   }
   
   BIO_free(in);
}

void
BaseSecurity::addCertX509(PEMType type, const Data& key, X509* cert, bool write)
{
   switch (type)
   {
      case DomainCert:
      {
         mDomainCerts.insert(std::make_pair(key, cert));
      }
      break;
      case UserCert:
      { 
         mUserCerts.insert(std::make_pair(key, cert));
      }
      break;
      case RootCert:
      {
         mRootCerts.push_back(cert);
         X509_STORE_add_cert(mRootTlsCerts,cert);
         X509_STORE_add_cert(mRootSslCerts,cert);
      }
      break;
      default:
      {
         resip_assert(0);
      }
   }
   
   if (write)
   {
      // creates a read/write BIO buffer.
      BIO *out = BIO_new(BIO_s_mem());
      if(!out)
      {
         ErrLog(<< "Failed to create BIO: this cert will not be added.");
         resip_assert(0);
         return;
      }

      try
      {
         int ret = PEM_write_bio_X509(out, cert);
         if(!ret)
         {
            resip_assert(0);
            throw Exception("PEM_write_bio_X509 failed: this cert will not be "
                              "added.", __FILE__,__LINE__);
         }
         
         (void)BIO_flush(out);
         // get content in BIO buffer to our buffer.
         char* p = 0;
         size_t len = BIO_get_mem_data(out,&p);
         if(!p || !len)
         {
            resip_assert(0);
            throw Exception("BIO_get_mem_data failed: this cert will not be "
                              "added.", __FILE__,__LINE__);
         }
         Data buf(Data::Borrow, p, (Data::size_type)len);
         
         this->onWritePEM(key, type, buf);
      }
      catch(Exception& e)
      {
         ErrLog(<<"Caught exception: " << e);
      }
      catch(std::exception& e)
      {
         ErrLog(<<"Caught unknown exception, rethrowing");
         BIO_free(out);
         throw e;
      }
      BIO_free(out);
   }
}

bool
BaseSecurity::hasCert (PEMType type, const Data& aor) const
{
   resip_assert( !aor.empty() );
   const X509Map& certs = (type == DomainCert ? mDomainCerts : mUserCerts);

   X509Map::const_iterator where = certs.find(aor);
   if (where != certs.end())
   {
      return true;
   }
   
   try
   {
      Data certPEM;
      onReadPEM(aor, type, certPEM);
      if (certPEM.empty())
      {
         return false;
      }
      BaseSecurity*  mutable_this = const_cast<BaseSecurity*>(this);
      mutable_this->addCertPEM(type, aor, certPEM, false);
   }
   catch (Exception& e)
   {
      ErrLog(<<"Caught exception: " << e);
      return   false;
   }
   catch (...)
   {
      ErrLog(<<"Caught exception: ");
      return   false;
   }

   resip_assert(  certs.find(aor) != certs.end() );
   
   return   true;
}

void
BaseSecurity::removeCert (PEMType type, const Data& aor)
{
   resip_assert( !aor.empty() );
   X509Map& certs = (type == DomainCert ? mDomainCerts : mUserCerts);

   X509Map::iterator iter = certs.find(aor);
   if (iter != certs.end())
   {
      X509_free(iter->second);
      certs.erase(iter);

      onRemovePEM(aor, type);
   } 

   resip_assert(  certs.find(aor) == certs.end() );
}

Data
BaseSecurity::getCertDER (PEMType type, const Data& key) const
{
   resip_assert( !key.empty() );

   if (hasCert(type, key) == false)
   {
      ErrLog(<< "Could not find certificate for '" << key << "'");
      throw BaseSecurity::Exception("Could not find certificate", __FILE__,__LINE__);
   }

   const X509Map& certs = (type == DomainCert ? mDomainCerts : mUserCerts);
   BaseSecurity::X509Map::const_iterator where = certs.find(key);
   if (where == certs.end())
   {
      // not supposed to happen,
      // hasCert() should have inserted a value into certs
      // or we should have throwed.
      resip_assert(0);
   }

   //assert(0); // the code following this has no hope of working 
   
   X509* x = where->second;
   unsigned char* buffer=0;
   int len = i2d_X509(x, &buffer);

   // !kh!
   // Although len == 0 is not an error, I am not sure what quite to do.
   // Asserting for now.
   resip_assert(len != 0);
   if(len < 0)
   {
      ErrLog(<< "Could encode certificate of '" << key << "' to DER form");
      throw BaseSecurity::Exception("Could encode certificate to DER form", __FILE__,__LINE__);
   }
   Data certDER((char*)buffer, len);
   OPENSSL_free(buffer);
   return certDER;
}

void 
BaseSecurity::addPrivateKeyPKEY(PEMType type, 
                                const Data& name, 
                                EVP_PKEY* pKey, 
                                bool write)
{ 
   PrivateKeyMap& privateKeys = (type == DomainPrivateKey ? 
                                 mDomainPrivateKeys : mUserPrivateKeys);

   /*
   // make a copy of the the key 
   resip_assert( EVP_PKEY_type(pKey->type) == EVP_PKEY_RSA );
   RSA* rsa = EVP_PKEY_get1_RSA(pKey);
   resip_assert( rsa );
   EVP_PKEY* nKey = EVP_PKEY_new();
   resip_assert( nKey );
   EVP_PKEY_set1_RSA(nKey, rsa);
   */
   
   //privateKeys.insert(std::make_pair(name, nKey));
   privateKeys.insert(std::make_pair(name, pKey));
      
   if (write)
   {
      // figure out a passPhrase to encrypt with 
      char* kstr=NULL;
      int klen=0;
      if (type != DomainPrivateKey)
      {
         PassPhraseMap::const_iterator iter = mUserPassPhrases.find(name);
         if(iter != mUserPassPhrases.end())
         {
            kstr = (char*)iter->second.c_str(); 
            klen = (int)iter->second.size();
         }
      }

      BIO *bio = BIO_new(BIO_s_mem());
      if(!bio)
      {
         ErrLog(<< "BIO_new failed: cannot add private key.");
         resip_assert(0);
      }

      try
      {
         resip_assert( EVP_des_ede3_cbc() );
         const EVP_CIPHER* cipher = EVP_des_ede3_cbc();
         if (kstr == NULL )
         {
            cipher = NULL;
         }
#if 0 // TODO - need to figure out what format to write in 
         int ret = PEM_write_bio_PrivateKey(bio, pKey, cipher, 
                                            (unsigned char*)kstr, klen,
                                            NULL, NULL);
#else
         int ret = PEM_write_bio_PKCS8PrivateKey(bio, pKey, cipher, 
                                                 kstr, klen,
                                                 NULL, NULL);
#endif
         if(!ret)
         {
            resip_assert(0);
            throw Exception("PEM_write_bio_PKCS8PrivateKey failed: cannot add"
                              " private key.", __FILE__, __LINE__);
         }

         (void)BIO_flush(bio);
         char* p = 0;
         size_t len = BIO_get_mem_data(bio,&p);
         if(!p || !len)
         {
            resip_assert(0);
            throw Exception("BIO_get_mem_data failed: cannot add"
                              " private key.", __FILE__, __LINE__);
         }
         Data pem(Data::Borrow, p, (Data::size_type)len);
         onWritePEM(name, type, pem );
      }
      catch(Exception& e)
      {
         ErrLog( << "Caught exception: " << e);
      }
      catch(std::exception& e)
      {
         ErrLog(<<"Caught unknown exception, rethrowing: " << e.what());
         BIO_free(bio);
         throw e;
      }
      BIO_free(bio);
   }
}

void
BaseSecurity::addPrivateKeyDER( PEMType type,
                                const Data& name,
                                const Data& privateKeyDER,
                                bool write,
                                const Data& privateKeyPassPhrase)
{
   resip_assert( !name.empty() );
   if( privateKeyDER.empty() )
   {
      ErrLog(<< name << " is empty. Skipping.");
      return;
   }

   char* passPhrase = 0;
   if (privateKeyPassPhrase.empty())
   {
      if (type == UserPrivateKey)
      {
         PassPhraseMap::const_iterator iter = mUserPassPhrases.find(name);
         if(iter != mUserPassPhrases.end())
         {
            passPhrase = const_cast<char*>(iter->second.c_str());
         }
      }
      else
      {
         if(!mDefaultPrivateKeyPassPhrase.empty())
         {
            passPhrase = const_cast<char*>(mDefaultPrivateKeyPassPhrase.c_str());
         }
      }
   }
   else
   {
      passPhrase = const_cast<char*>(privateKeyPassPhrase.c_str());
   }
   
   BIO* in = BIO_new_mem_buf(const_cast<char*>(privateKeyDER.c_str()), -1);
   if ( !in )
   {
      ErrLog(<< "Could create BIO buffer from '" << privateKeyDER << "'");
      throw Exception("Could not create BIO buffer", __FILE__,__LINE__);
   }
   
   try
   {
      EVP_PKEY* privateKey;
      if (d2i_PKCS8PrivateKey_bio(in, &privateKey, 0, passPhrase) == 0)
      {
         ErrLog(<< "Could not read private key from <" << privateKeyDER << ">" );
         throw Exception("Could not read private key ", __FILE__,__LINE__);
      }
      
      addPrivateKeyPKEY(type,name,privateKey,write);
   }
   catch(std::exception& e)
   {
      ErrLog(<<"Caught exception: ");
      BIO_free(in);
      throw e;
   }
   
   BIO_free(in);
}

void
BaseSecurity::addPrivateKeyPEM( PEMType type,
                                const Data& name,
                                const Data& privateKeyPEM,
                                bool write,
                                const Data& privateKeyPassPhrase)
{
   resip_assert( !name.empty() );
   if( privateKeyPEM.empty() )
   {
      ErrLog(<< name << " is empty. Skipping.");
      return;
   }

   BIO* in = BIO_new_mem_buf(const_cast<char*>(privateKeyPEM.c_str()), -1);
   if ( !in )
   {
      ErrLog(<< "Could create BIO buffer from '" << privateKeyPEM << "'");
      throw Exception("Could not create BIO buffer", __FILE__,__LINE__);
   }
   
   char* passPhrase = 0;
   try
   {
      if (privateKeyPassPhrase.empty())
      {
         if (type == UserPrivateKey)
         {
            PassPhraseMap::const_iterator iter = mUserPassPhrases.find(name);
            if(iter != mUserPassPhrases.end())
            {
               passPhrase = const_cast<char*>(iter->second.c_str());
            }
         }
         else
         {
            if(!mDefaultPrivateKeyPassPhrase.empty())
            {
               passPhrase = const_cast<char*>(mDefaultPrivateKeyPassPhrase.c_str());
            }
         }
      }
      else
      {
         passPhrase = const_cast<char*>(privateKeyPassPhrase.c_str());
      }
      
      EVP_PKEY* privateKey=0;
      if ( ( privateKey = PEM_read_bio_PrivateKey(in, NULL, pem_passwd_cb, passPhrase)) == NULL)
      {
         char buffer[120];
         unsigned long err = ERR_get_error();
         ERR_error_string(err, buffer);
         if(ERR_GET_LIB(err) == ERR_LIB_EVP && ERR_GET_FUNC(err) == EVP_F_EVP_DECRYPTFINAL_EX && ERR_GET_REASON(err) == EVP_R_BAD_DECRYPT)
         {
            ErrLog(<< "Could not read private key (error=" << buffer << ") - likely incorrect password provided, may load correctly when transports are added with appropriate password");
         }
         else
         {
            ErrLog(<< "Could not read private key (error=" << buffer << ") from <" << privateKeyPEM << ">" );
         }
         throw Exception("Could not read private key ", __FILE__,__LINE__);
      }
      
      addPrivateKeyPKEY(type,name,privateKey,write);
   }
   catch(std::exception& e)
   {
      ErrLog(<<"Caught exception: ");
      BIO_free(in);
      throw e;
   }

   BIO_free(in);
}

bool
BaseSecurity::hasPrivateKey( PEMType type,
                             const Data& key ) const
{
   resip_assert( !key.empty() );

   const PrivateKeyMap& privateKeys = (type == DomainPrivateKey 
                                 ? mDomainPrivateKeys : mUserPrivateKeys);

   PrivateKeyMap::const_iterator where = privateKeys.find(key);
   if (where != privateKeys.end())
   {
      return true;
   }
   
   Data privateKeyPEM;
   try
   {
      onReadPEM(key, type, privateKeyPEM);
      BaseSecurity* mutable_this = const_cast<BaseSecurity*>(this);
      mutable_this->addPrivateKeyPEM(type, key, privateKeyPEM, false);
   }
   catch(std::exception& e)
   {
      ErrLog(<<"Caught exception: " << e.what());
      return   false;
   }
   catch(...)
   {
      ErrLog(<<"Caught unknown class!");
      return   false;
   }

   return   true;
}

Data
BaseSecurity::getPrivateKeyPEM( PEMType type,
                                const Data& key) const
{
   resip_assert( !key.empty() );

   if ( !hasPrivateKey(type, key) )
   {
      ErrLog(<< "Could find private key for '" << key << "'");
      throw Exception("Could not find private key", __FILE__,__LINE__);
   }

   const PrivateKeyMap& privateKeys = (type == DomainPrivateKey ? mDomainPrivateKeys : mUserPrivateKeys);

   PrivateKeyMap::const_iterator where = privateKeys.find(key);
   char* p = 0;
   if (type != DomainPrivateKey)
   {
      PassPhraseMap::const_iterator iter = mUserPassPhrases.find(key);
      if (iter != mUserPassPhrases.end())
      {
         p = const_cast<char*>(iter->second.c_str());
      }
   }

   resip_assert(0); // TODO - following code has no hope of working 
    
   // !kh!
   // creates a read/write BIO buffer.
   BIO *out = BIO_new(BIO_s_mem());
   resip_assert(out);
   EVP_PKEY* pk = where->second;
   resip_assert(pk);

   // write pk to out using key phrase p, with no cipher.
   int ret = PEM_write_bio_PrivateKey(out, pk, 0, 0, 0, 0, p);  // paraters
                                                                // are in the wrong order
   (void)ret;
   resip_assert(ret == 1);

   // get content in BIO buffer to our buffer.
   // hand our buffer to a Data object.
   (void)BIO_flush(out);
   char* buf = 0;
   int len = BIO_get_mem_data(out, &buf);
   Data retVal(Data::Borrow, buf, len);

   BIO_free(out);

   return retVal;
}

Data
BaseSecurity::getPrivateKeyDER( PEMType type,
                                const Data& key) const
{
   resip_assert( !key.empty() );

   if ( !hasPrivateKey(type, key) )
   {
      ErrLog(<< "Could find private key for '" << key << "'");
      throw Exception("Could not find private key", __FILE__,__LINE__);
   }

   const PrivateKeyMap& privateKeys = (type == DomainPrivateKey ? mDomainPrivateKeys : mUserPrivateKeys);

   PrivateKeyMap::const_iterator where = privateKeys.find(key);
   char* p = 0;
   if (type != DomainPrivateKey)
   {
      PassPhraseMap::const_iterator iter = mUserPassPhrases.find(key);
      if(iter != mUserPassPhrases.end())
      {
         p = const_cast<char*>(iter->second.c_str());
      }
   }

   resip_assert(0); // TODO - following code has no hope of working 
    
   // !kh!
   // creates a read/write BIO buffer.
   BIO *out = BIO_new(BIO_s_mem());
   resip_assert(out);
   EVP_PKEY* pk = where->second;
   resip_assert(pk);

   // write pk to out using key phrase p, with no cipher.
   int ret = i2d_PKCS8PrivateKey_bio(out, pk, 0, 0, 0, 0, p);
   (void)ret;
   resip_assert(ret == 1);

   // get content in BIO buffer to our buffer.
   // hand our buffer to a Data object.
   (void)BIO_flush(out);
   char* buf = 0;
   int len = BIO_get_mem_data(out, &buf);
   Data retVal(Data::Borrow, buf, len);

   BIO_free(out);
   
   return retVal;
}

void
BaseSecurity::removePrivateKey(PEMType type, const Data& key)
{
   resip_assert( !key.empty() );

   PrivateKeyMap& privateKeys = (type == DomainPrivateKey ? mDomainPrivateKeys : mUserPrivateKeys);

   resip_assert( !key.empty() );
   PrivateKeyMap::iterator iter = privateKeys.find(key);
   if (iter != privateKeys.end())
   {
      EVP_PKEY_free(iter->second);
      privateKeys.erase(iter);

      onRemovePEM(key, type);
   }
}


Security::Exception::Exception(const Data& msg, const Data& file, const int line)
:  BaseException(msg,file,line)
{
}

BaseSecurity::BaseSecurity (const CipherList& cipherSuite, const Data& defaultPrivateKeyPassPhrase, const Data& dHParamsFilename) :
   mTlsCtx(0),
   mSslCtx(0),
   mCipherList(cipherSuite),
   mDefaultPrivateKeyPassPhrase(defaultPrivateKeyPassPhrase),
   mDHParamsFilename(dHParamsFilename),
   mRootTlsCerts(0),
   mRootSslCerts(0)
{ 
   DebugLog(<< "BaseSecurity::BaseSecurity");
   
   int ret;
   initialize(); 
   
   mRootTlsCerts = X509_STORE_new();
   mRootSslCerts = X509_STORE_new();
   resip_assert(mRootTlsCerts && mRootSslCerts);

   mTlsCtx = SSL_CTX_new( TLSv1_method() );
   if (!mTlsCtx)
   {
      ErrLog(<< "SSL_CTX_new failed, dumping OpenSSL error stack:");
      while (ERR_peek_error())
      {
         char errBuf[120];
         ERR_error_string(ERR_get_error(), errBuf);
         ErrLog(<< "OpenSSL error stack: " << errBuf);
      }
   }
   resip_assert(mTlsCtx);

   SSL_CTX_set_default_passwd_cb(mTlsCtx, pem_passwd_cb);
   SSL_CTX_set_cert_store(mTlsCtx, mRootTlsCerts);
   SSL_CTX_set_verify(mTlsCtx, SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE, verifyCallback);
   ret = SSL_CTX_set_cipher_list(mTlsCtx, cipherSuite.cipherList().c_str());
   resip_assert(ret);
   setDHParams(mTlsCtx);
   SSL_CTX_set_options(mTlsCtx, BaseSecurity::OpenSSLCTXSetOptions);
   SSL_CTX_clear_options(mTlsCtx, BaseSecurity::OpenSSLCTXClearOptions);
   
   mSslCtx = SSL_CTX_new( SSLv23_method() );
   resip_assert(mSslCtx);
   SSL_CTX_set_default_passwd_cb(mSslCtx, pem_passwd_cb);
   SSL_CTX_set_cert_store(mSslCtx, mRootSslCerts);
   SSL_CTX_set_verify(mSslCtx, SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE, verifyCallback);
   ret = SSL_CTX_set_cipher_list(mSslCtx,cipherSuite.cipherList().c_str());
   resip_assert(ret);
   setDHParams(mSslCtx);
   SSL_CTX_set_options(mSslCtx, BaseSecurity::OpenSSLCTXSetOptions);
   SSL_CTX_clear_options(mSslCtx, BaseSecurity::OpenSSLCTXClearOptions);
}


template<class T, class Func> 
void clearMap(T& m, Func& clearFunc)
{
   for (typename T::iterator it = m.begin(); it != m.end(); it++)
   {
      clearFunc(it->second);
   }
   m.clear();
}

template<class T, class Func> 
void clearList(T& m, Func& clearFunc)
{
   for (typename T::iterator it = m.begin(); it != m.end(); it++)
   {
      clearFunc(*it);
   }
   m.clear();
}

BaseSecurity::~BaseSecurity ()
{
   DebugLog(<< "BaseSecurity::~BaseSecurity");

   // cleanup certificates
   clearList(mRootCerts, X509_free);
   clearMap(mDomainCerts, X509_free);
   clearMap(mUserCerts, X509_free);

   // cleanup private keys
   clearMap(mDomainPrivateKeys, EVP_PKEY_free);
   clearMap(mUserPrivateKeys, EVP_PKEY_free);

   // cleanup SSL_CTXes
   if (mTlsCtx)
   {
      SSL_CTX_free(mTlsCtx);mTlsCtx=0;  // This free's X509_STORE (mRootTlsCerts)
   }
   if (mSslCtx)
   {
      SSL_CTX_free(mSslCtx);mSslCtx=0;  // This free's X509_STORE (mRootSslCerts)
   }

}

void
BaseSecurity::initialize ()
{
  Timer::getTimeMs(); // initalize time offsets
}

Security::CertificateInfoContainer
BaseSecurity::getRootCertDescriptions() const
{
   // !kh!
   // need to be implemented.
   resip_assert(0); // TODO 
   return   CertificateInfoContainer();
}

void
BaseSecurity::addRootCertPEM(const Data& x509PEMEncodedRootCerts)
{ 
   resip_assert( mRootTlsCerts && mRootSslCerts );
#if 1
   addCertPEM(RootCert,Data::Empty,x509PEMEncodedRootCerts,false);
#else
   resip_assert( !x509PEMEncodedRootCerts.empty() );

   static X509_LOOKUP_METHOD x509_pemstring_lookup =
  {
     "Load cert from PEM string into cache",
     NULL,  /* new */
     NULL,  /* free */
     NULL,   /* init */
     NULL,  /* shutdown */
     pemstring_ctrl,/* ctrl */
     NULL,  /* get_by_subject */
     NULL,  /* get_by_issuer_serial */
     NULL,  /* get_by_fingerprint */
     NULL,  /* get_by_alias */
  };

   if (mRootCerts == 0)
   {
      mRootCerts = X509_STORE_new();
   }
   
   resip_assert( mRootCerts );

   X509_LOOKUP* lookup = X509_STORE_add_lookup(mRootCerts, &x509_pemstring_lookup);

   if (lookup == NULL)
      throw Exception("Error in BaseSecurity::addRootCertPEM()", __FILE__,__LINE__);

   // !kh!
   // bug, no error handling here.
   X509_LOOKUP_ctrl(lookup, X509_L_FILE_LOAD, x509PEMEncodedRootCerts.c_str(), 0, 0);
#endif
}

void
BaseSecurity::addDomainCertPEM(const Data& domainName, const Data& certPEM)
{
   addCertPEM(DomainCert, domainName, certPEM, false);
}


void
BaseSecurity::addDomainCertDER(const Data& domainName, const Data& certDER)
{
   addCertDER(DomainCert, domainName, certDER, false);
}

bool
BaseSecurity::hasDomainCert(const Data& domainName) const
{
   return   hasCert(DomainCert, domainName);
}


void
BaseSecurity::removeDomainCert(const Data& domainName)
{
   return   removeCert(DomainCert, domainName);
}

Data
BaseSecurity::getDomainCertDER(const Data& domainName) const
{
   return   getCertDER(DomainCert, domainName);
}

void
BaseSecurity::addDomainPrivateKeyPEM(const Data& domainName, const Data& privateKeyPEM, const Data& privateKeyPassPhrase)
{
   addPrivateKeyPEM(DomainPrivateKey, domainName, privateKeyPEM, false, privateKeyPassPhrase);
}

bool
BaseSecurity::hasDomainPrivateKey(const Data& domainName) const
{
   return hasPrivateKey(DomainPrivateKey, domainName);
}

void
BaseSecurity::removeDomainPrivateKey(const Data& domainName)
{
   removePrivateKey(DomainPrivateKey, domainName);
}


Data
BaseSecurity::getDomainPrivateKeyPEM(const Data& domainName) const
{
   return   getPrivateKeyPEM(DomainPrivateKey, domainName);
}

void
BaseSecurity::addUserCertPEM(const Data& aor, const Data& certPEM)
{
   addCertPEM(UserCert, aor, certPEM, false);
}


void
BaseSecurity::addUserCertDER(const Data& aor, const Data& certDER)
{
   addCertDER(UserCert, aor, certDER, false);
}

bool
BaseSecurity::hasUserCert(const Data& aor) const
{
   return   hasCert(UserCert, aor);
}


void
BaseSecurity::removeUserCert(const Data& aor)
{
   removeCert(UserCert, aor);
}

Data
BaseSecurity::getUserCertDER(const Data& aor) const
{
   return   getCertDER(UserCert, aor);
}

void
BaseSecurity::setUserPassPhrase(const Data& aor, const Data& passPhrase)
{
   resip_assert(!aor.empty());

   PassPhraseMap::iterator iter = mUserPassPhrases.find(aor);
   if (iter == mUserPassPhrases.end())
   {
      mUserPassPhrases.insert(std::make_pair(aor, passPhrase));
   }
}

bool
BaseSecurity::hasUserPassPhrase(const Data& aor) const
{
   resip_assert(aor.empty());

   PassPhraseMap::const_iterator iter = mUserPassPhrases.find(aor);
   if (iter == mUserPassPhrases.end())
   {
      return   false;
   }
   else
   {
      return   true;
   }
}

void
BaseSecurity::removeUserPassPhrase(const Data& aor)
{
   resip_assert(aor.empty());

   PassPhraseMap::iterator iter = mUserPassPhrases.find(aor);
   if(iter != mUserPassPhrases.end())
   {
      mUserPassPhrases.erase(iter);
   }
}

Data
BaseSecurity::getUserPassPhrase(const Data& aor) const
{
   resip_assert(aor.empty());

   PassPhraseMap::const_iterator iter = mUserPassPhrases.find(aor);
   if(iter == mUserPassPhrases.end())
   {
      return   iter->second;
   }
   else
   {
      return   Data::Empty;
   }
}

void
BaseSecurity::addUserPrivateKeyPEM(const Data& aor, const Data& cert, const Data& privateKeyPassPhrase)
{
   addPrivateKeyPEM(UserPrivateKey, aor, cert, false, privateKeyPassPhrase);
}

void
BaseSecurity::addUserPrivateKeyDER(const Data& aor, const Data& cert, const Data& privateKeyPassPhrase)
{
   addPrivateKeyDER(UserPrivateKey, aor, cert, false, privateKeyPassPhrase);
}

bool
BaseSecurity::hasUserPrivateKey(const Data& aor) const
{
   return hasPrivateKey(UserPrivateKey, aor);
}

void
BaseSecurity::removeUserPrivateKey(const Data& aor)
{
   removePrivateKey(UserPrivateKey, aor);
}

Data
BaseSecurity::getUserPrivateKeyPEM(const Data& aor) const
{
   return   getPrivateKeyPEM(UserPrivateKey, aor);
}

Data
BaseSecurity::getUserPrivateKeyDER(const Data& aor) const
{
   return   getPrivateKeyDER(UserPrivateKey, aor);
}

void
BaseSecurity::generateUserCert (const Data& pAor, int expireDays, int keyLen )
{
   int ret;
   
   InfoLog( <<"Generating new user cert for " << pAor );
 
   Data domain;
   Data aor;

   try
   {
      Uri uri( Data("sip:")+pAor );
      aor = uri.getAor();
      domain = uri.host();
   }
   catch (...)
   { 
      ErrLog( <<"Invalid aor passed to generateUserCert");
      throw Exception("Bad aor passed to generateUserCert", __FILE__,__LINE__);
   }
   
   // Make sure that necessary algorithms exist:
   resip_assert(EVP_sha256());

#if OPENSSL_VERSION_NUMBER < 0x00908000l
   RSA* rsa = RSA_generate_key(keyLen, RSA_F4, NULL, NULL);
#else
   RSA* rsa = NULL;
   {
      BIGNUM *e = BN_new();
      RSA *r = NULL;
      if(!e) goto done;
      if(! BN_set_word(e, RSA_F4)) goto done;
      r = RSA_new();
      if(!r) goto done;
      if (RSA_generate_key_ex(r, keyLen, e, NULL) == -1)
         goto done;

      rsa = r;
      r = NULL;
   done:
      if (e)
         BN_free(e);
      if (r)
         RSA_free(r);
    }
#endif
   resip_assert(rsa);    // couldn't make key pair
   
   EVP_PKEY* privkey = EVP_PKEY_new();
   resip_assert(privkey);
   ret = EVP_PKEY_set1_RSA(privkey, rsa);
   resip_assert(ret);

   X509* cert = X509_new();
   resip_assert(cert);
   
   X509_NAME* subject = X509_NAME_new();
   X509_EXTENSION* ext = X509_EXTENSION_new();
   
   // set version to X509v3 (starts from 0)
   X509_set_version(cert, 2L);
   
   int serial = Random::getRandom();  // get an int worth of randomness
   resip_assert(sizeof(int)==4);
   ASN1_INTEGER_set(X509_get_serialNumber(cert),serial);
   
   ret = X509_NAME_add_entry_by_txt( subject, "O",  MBSTRING_ASC, 
                                     (unsigned char *) domain.data(), (int)domain.size(), 
                                     -1, 0);
   resip_assert(ret);
   ret = X509_NAME_add_entry_by_txt( subject, "CN", MBSTRING_ASC, 
                                     (unsigned char *) aor.data(), (int)aor.size(), 
                                     -1, 0);
   resip_assert(ret);
   
   ret = X509_set_issuer_name(cert, subject);
   resip_assert(ret);
   ret = X509_set_subject_name(cert, subject);
   resip_assert(ret);
   
   const long duration = 60*60*24*expireDays;   
   X509_gmtime_adj(X509_get_notBefore(cert),0);
   X509_gmtime_adj(X509_get_notAfter(cert), duration);
   
   ret = X509_set_pubkey(cert, privkey);
   resip_assert(ret);
   
   Data subjectAltNameStr = Data("URI:sip:") + aor
      + Data(",URI:im:")+aor
      + Data(",URI:pres:")+aor;
   ext = X509V3_EXT_conf_nid( NULL , NULL , NID_subject_alt_name, 
                              (char*) subjectAltNameStr.c_str() );
   X509_add_ext( cert, ext, -1);
   X509_EXTENSION_free(ext);
   
   static char CA_FALSE[] = "CA:FALSE";
   ext = X509V3_EXT_conf_nid(NULL, NULL, NID_basic_constraints, CA_FALSE);
   ret = X509_add_ext( cert, ext, -1);
   resip_assert(ret);
   X509_EXTENSION_free(ext);
   
   // TODO add extensions NID_subject_key_identifier and NID_authority_key_identifier
   
   ret = X509_sign(cert, privkey, EVP_sha256());
   resip_assert(ret);
   
   addCertX509( UserCert, aor, cert, true /* write */ );
   addPrivateKeyPKEY( UserPrivateKey, aor, privkey, true /* write */ );
}

MultipartSignedContents*
BaseSecurity::sign(const Data& senderAor, Contents* contents)
{
   resip_assert( contents );

   // form the multipart
   MultipartSignedContents* multi = new MultipartSignedContents;
   multi->header(h_ContentType).param( p_micalg ) = "sha256";
   multi->header(h_ContentType).param( p_protocol ) = "application/pkcs7-signature";

   // add the main body to it
   Contents* body =  contents->clone();
   multi->parts().push_back( body );

   Data bodyData;
   DataStream strm( bodyData );
   body->encodeHeaders( strm );
   body->encode( strm );
   strm.flush();

   DebugLog( << "signing data <" << bodyData.escaped() << ">" );
   //Security::dumpAsn("resip-sign-out-data",bodyData);

   const char* p = bodyData.data();
   int s = (int)bodyData.size();
   BIO* in=BIO_new_mem_buf( (void*)p,s);
   resip_assert(in);
   DebugLog( << "created in BIO");

   BIO* out = BIO_new(BIO_s_mem()); // TODO - mem leak 
   resip_assert(out);
   DebugLog( << "created out BIO" );

   STACK_OF(X509)* chain = sk_X509_new_null();
   resip_assert(chain);

   DebugLog( << "searching for cert/key for <" << senderAor << ">" );
   if (mUserCerts.count(senderAor) == 0 ||
       mUserPrivateKeys.count(senderAor) == 0)
   {
      BIO_free(in);
      BIO_free(out);
      sk_X509_free(chain);
      WarningLog (<< "Tried to sign with no cert or private key for " << senderAor);
      throw Exception("No cert or private key to sign with",__FILE__,__LINE__);
   }
   
   X509* publicCert = mUserCerts[senderAor];
   EVP_PKEY* privateKey = mUserPrivateKeys[senderAor];

   int rv = X509_check_private_key(publicCert, privateKey);
   if(!rv)
   {
      BIO_free(in);
      BIO_free(out);
      sk_X509_free(chain);
      ErrLog (<< "X509_check_private_key failed for " << senderAor);
      return 0;
   }

   // compute the signature
   int flags = 0;
   flags |= PKCS7_BINARY; 
   flags |= PKCS7_DETACHED; 
   if ( true ) // don't do caps
   {
      flags |= PKCS7_NOSMIMECAP; 
      flags |= PKCS7_NOATTR; 
   }
   if ( true ) // don't do certs 
   {
      flags |= PKCS7_NOCERTS;  
   }

   PKCS7* pkcs7 = PKCS7_sign( publicCert, privateKey, chain, in, flags);
   if ( !pkcs7 )
   {
      BIO_free(in);
      BIO_free(out);
      sk_X509_free(chain);
      ErrLog( << "Error creating PKCS7 signature object" );
      return 0;
   }
   DebugLog( << "created PKCS7 signature object " );

   i2d_PKCS7_bio(out,pkcs7);
   (void)BIO_flush(out);

   char* outBuf=0;
   long size = BIO_get_mem_data(out,&outBuf);
   resip_assert( size > 0 );

   Data outData(outBuf,size);
   static char RESIP_SIGN_OUT_SIG[] = "resip-sign-out-sig";
   Security::dumpAsn(RESIP_SIGN_OUT_SIG,outData);

   Pkcs7SignedContents* sigBody = new Pkcs7SignedContents( outData );
   resip_assert( sigBody );

   // add the signature to it
   sigBody->header(h_ContentType).param( p_name ) = "smime.p7s";
   sigBody->header(h_ContentDisposition).param( p_handling ) = "required";
   sigBody->header(h_ContentDisposition).param( p_filename ) = "smime.p7s";
   sigBody->header(h_ContentDisposition).value() =  "attachment" ;
   sigBody->header(h_ContentTransferEncoding).value() = "binary";
   multi->parts().push_back( sigBody );

   resip_assert( multi->parts().size() == 2 );

   BIO_free(in);
   BIO_free(out);
   sk_X509_free(chain);
   PKCS7_free(pkcs7);

   return multi;
}


Pkcs7Contents*
BaseSecurity::encrypt(Contents* bodyIn, const Data& recipCertName )
{
   resip_assert( bodyIn );

   int flags = 0 ;
   flags |= PKCS7_BINARY;
   flags |= PKCS7_NOCERTS;

   Data bodyData;
   DataStream strm(bodyData);
   bodyIn->encodeHeaders(strm);
   bodyIn->encode( strm );
   strm.flush();

   InfoLog( << "body data to encrypt is <" << bodyData.escaped() << ">" );

   const char* p = bodyData.data();
   int s = (int)bodyData.size();

   BIO* in = BIO_new_mem_buf( (void*)p,s);
   resip_assert(in);
   DebugLog( << "created in BIO");

   BIO* out = BIO_new(BIO_s_mem());
   resip_assert(out);
   DebugLog( << "created out BIO" );

   InfoLog( << "target cert name is <" << recipCertName << ">" );
   if (mUserCerts.count(recipCertName) == 0)
   {
      BIO_free(in);
      BIO_free(out);
      WarningLog (<< "Tried to encrypt with no cert or private key for " << recipCertName);
      throw Exception("No cert or private key to encrypt with",__FILE__,__LINE__);
   }

   X509* cert = mUserCerts[recipCertName];
   resip_assert(cert);

   STACK_OF(X509) *certs = sk_X509_new_null();
   resip_assert(certs);
   sk_X509_push(certs, cert);

// if you think you need to change the following few lines, please email fluffy
// the value of OPENSSL_VERSION_NUMBER ( in opensslv.h ) and the signature of
// PKCS_encrypt found ( in pkcs7.h ) and the OS you are using
#if (  OPENSSL_VERSION_NUMBER > 0x009060ffL )
//   const EVP_CIPHER* cipher =  EVP_des_ede3_cbc();
   const EVP_CIPHER* cipher =  EVP_aes_128_cbc(); 
#else
   //const EVP_CIPHER* cipher = EVP_enc_null();
   EVP_CIPHER* cipher =  EVP_des_ede3_cbc();
#endif
   resip_assert( cipher );

#if (OPENSSL_VERSION_NUMBER < 0x0090705fL )
#warning PKCS7_encrypt() is broken in OpenSSL 0.9.7d
#endif

   PKCS7* pkcs7 = PKCS7_encrypt( certs, in, cipher, flags);
   if ( !pkcs7 )
   {
      BIO_free(in);
      BIO_free(out);
      sk_X509_free(certs);
      ErrLog( << "Error creating PKCS7 encrypt object" );
      //throw Exception("Can't encrypt",__FILE__,__LINE__);
      return 0;
   }
   DebugLog( << "created PKCS7 encrypt object " );

   i2d_PKCS7_bio(out,pkcs7);

   (void)BIO_flush(out);

   char* outBuf=0;
   long size = BIO_get_mem_data(out,&outBuf);
   resip_assert( size > 0 );

   Data outData(outBuf,size);
   resip_assert( (long)outData.size() == size );

   InfoLog( << "Encrypted body size is " << outData.size() );
   InfoLog( << "Encrypted body is <" << outData.escaped() << ">" );

   static char RESIP_ENCRYPT_OUT[] = "resip-encrypt-out";
   Security::dumpAsn(RESIP_ENCRYPT_OUT, outData);

   Pkcs7Contents* outBody = new Pkcs7Contents( outData );
   resip_assert( outBody );

   outBody->header(h_ContentType).param( p_smimeType ) = "enveloped-data";
   outBody->header(h_ContentType).param( p_name ) = "smime.p7m";
   outBody->header(h_ContentDisposition).param( p_handling ) = "required";
   outBody->header(h_ContentDisposition).param( p_filename ) = "smime.p7";
   outBody->header(h_ContentDisposition).value() =  "attachment" ;
   outBody->header(h_ContentTransferEncoding).value() = "binary";

   BIO_free(in);
   BIO_free(out);
   sk_X509_free(certs);
   PKCS7_free(pkcs7);

   return outBody;
}


MultipartSignedContents *
BaseSecurity::signAndEncrypt( const Data& senderAor, Contents* body, const Data& recipCertName )
{
   //assert(0);
   //return 0;
   return sign(senderAor, encrypt(body, recipCertName));
}


Data
BaseSecurity::computeIdentity( const Data& signerDomain, const Data& in ) const
{
   DebugLog( << "Compute identity for " << in );

   PrivateKeyMap::const_iterator k(mDomainPrivateKeys.find(signerDomain));
   if (k == mDomainPrivateKeys.end())
   {
      InfoLog( << "No private key for " << signerDomain );
      throw Exception("Missing private key when computing identity",__FILE__,__LINE__);
   }

   EVP_PKEY* pKey = k->second;
   resip_assert( pKey );

   RSA* rsa = EVP_PKEY_get1_RSA(pKey);

   if ( !rsa )
   {
      ErrLog( << "Private key (type=" << EVP_PKEY_id(pKey) <<"for "
              << signerDomain << " is not of type RSA" );
      throw Exception("No RSA private key when computing identity",__FILE__,__LINE__);
   }

   resip_assert( rsa );

   unsigned char result[4096];
   int resultSize = sizeof(result);
   resip_assert( resultSize >= RSA_size(rsa) );

   SHA1Stream sha;
   sha << in;
   Data hashRes =  sha.getBin();
   DebugLog( << "hash of string is 0x" << hashRes.hex() );

#if 1
   int r = RSA_sign(NID_sha256, (unsigned char *)hashRes.data(), (unsigned int)hashRes.size(),
                    result, (unsigned int*)( &resultSize ),
            rsa);
   if( r != 1 )
   {
      ErrLog(<< "RSA_sign failed with return " << r);
      resip_assert(0);
      return Data::Empty;
   }
#else
   resultSize = RSA_private_encrypt(hashResLen, hashRes,
                                    result, rsa, RSA_PKCS1_PADDING);
   if ( resultSize == -1 )
   {
      DebugLog( << "Problem doing RSA encrypt for identity");
      while (1)
      {
         const char* file;
         int line;

         unsigned long code = ERR_get_error_line(&file,&line);
         if ( code == 0 )
         {
            break;
         }

         char buf[256];
         ERR_error_string_n(code,buf,sizeof(buf));
         ErrLog( << buf  );
         InfoLog( << "Error code = " << code << " file="<<file<<" line=" << line );
      }

      return Data::Empty;
   }
#endif

   Data res(result,resultSize);
   DebugLog( << "rsa encrypt of hash is 0x"<< res.hex() );

   Data enc = res.base64encode();

   static char IDENTITY_IN[] = "identity-in";
   static char IDENTITY_IN_HASH[] = "identity-in-hash";
   static char IDENTITY_IN_RSA[] = "identity-in-rsa";
   static char IDENTITY_IN_BASE64[] = "identity-in-base64";

   Security::dumpAsn(IDENTITY_IN, in );
   Security::dumpAsn(IDENTITY_IN_HASH, hashRes );
   Security::dumpAsn(IDENTITY_IN_RSA,res);
   Security::dumpAsn(IDENTITY_IN_BASE64,enc);

   return enc;
}


bool
BaseSecurity::checkIdentity( const Data& signerDomain, const Data& in, const Data& sigBase64, X509* pCert ) const
{
   X509* cert =  pCert;
   if (!cert)
   {
      X509Map::const_iterator x=mDomainCerts.find(signerDomain);
      if (x == mDomainCerts.end())
      {
         ErrLog( << "No public key for " << signerDomain );
         throw Exception("Missing public key when verifying identity",__FILE__,__LINE__);
      }
      cert = x->second;
   }
   
   DebugLog( << "Check identity for " << in );
   DebugLog( << " base64 data is " << sigBase64 );

   Data sig = sigBase64.base64decode();
   DebugLog( << "decoded sig is 0x"<< sig.hex() );

   SHA1Stream sha;
   sha << in;
   Data hashRes =  sha.getBin();
   DebugLog( << "hash of string is 0x" << hashRes.hex() );

   EVP_PKEY* pKey = X509_get_pubkey( cert );
   resip_assert( pKey );

   RSA* rsa = EVP_PKEY_get1_RSA(pKey);
   resip_assert( rsa );

#if 1
   int ret = RSA_verify(NID_sha256, (unsigned char *)hashRes.data(),
                        (unsigned int)hashRes.size(), (unsigned char*)sig.data(), (unsigned int)sig.size(),
                        rsa);
#else
   unsigned char result[4096];
   int resultSize = sizeof(result);
   resip_assert( resultSize >= RSA_size(rsa) );

   resultSize = RSA_public_decrypt(sig.size(),(unsigned char*)sig.data(),
                                   result, rsa, RSA_PKCS1_PADDING );
   resip_assert( resultSize != -1 );
   //assert( resultSize == SHA_DIGEST_LENGTH );
   Data recievedHash(result,resultSize);
   dumpAsn("identity-out-decrypt", recievedHash );

   bool ret =  ( computedHash == recievedHash );
#endif

   DebugLog( << "rsa verify result is " << ret  );

   static char IDENTITY_OUT_MSG[] = "identity-out-msg";
   static char IDENTITY_OUT_BASE64[] = "identity-out-base64";
   static char IDENTITY_OUT_SIG[] = "identity-out-sig";
   static char IDENTITY_OUT_HASH[] = "identity-out-hash";

   Security::dumpAsn(IDENTITY_OUT_MSG, in );
   Security::dumpAsn(IDENTITY_OUT_BASE64,sigBase64);
   Security::dumpAsn(IDENTITY_OUT_SIG, sig);
   Security::dumpAsn(IDENTITY_OUT_HASH, hashRes );

   return (ret != 0);
}


void
BaseSecurity::checkAndSetIdentity(SipMessage& msg, const Data& certDer) const
{
   unique_ptr<SecurityAttributes> sec(new SecurityAttributes);
   X509* cert=NULL;
   
   try
   {
      if ( !certDer.empty() )
      {
#if (OPENSSL_VERSION_NUMBER < 0x0090800fL )
         unsigned char* in = (unsigned char*)certDer.data();
#else
         unsigned const char* in = (unsigned const char*)certDer.data();
#endif
         if (d2i_X509(&cert,&in,(long)certDer.size()) == 0)
         {
            DebugLog(<< "Could not read DER certificate from " << certDer );
            cert = NULL;
         }
      }
      if ( certDer.empty() || cert )
      {
         if ( checkIdentity(msg.const_header(h_From).uri().host(),
                            msg.getCanonicalIdentityString(),
                            msg.const_header(h_Identity).value(),
                            cert ) )
         {
            sec->setIdentity(msg.const_header(h_From).uri().getAor());
            sec->setIdentityStrength(SecurityAttributes::Identity);
         }
         else
         {
            sec->setIdentity(msg.const_header(h_From).uri().getAor());
            sec->setIdentityStrength(SecurityAttributes::FailedIdentity);
         }
      }
      else
      {
         sec->setIdentity(msg.const_header(h_From).uri().getAor());
         sec->setIdentityStrength(SecurityAttributes::FailedIdentity);
      }
   }
   catch (BaseException& e)
   {
      ErrLog(<<"Caught exception: "<< e);
      sec->setIdentity(msg.const_header(h_From).uri().getAor());
      sec->setIdentityStrength(SecurityAttributes::FailedIdentity);
   }
   msg.setSecurityAttributes(std::move(sec));
}


Contents*
BaseSecurity::decrypt( const Data& decryptorAor, const Pkcs7Contents* contents)
{

   DebugLog( << "decryptor Aor: <" << decryptorAor << ">" );

   int flags=0;
   flags |= PKCS7_BINARY;

   // for now, assume that this is only a singed message
   resip_assert( contents );

   Data text = contents->getBodyData();
   DebugLog( << "uncode body = <" << text.escaped() << ">" );
   DebugLog( << "uncode body size = " << text.size() );

   static char RESIP_ASN_DECRYPT[] = "resip-asn-decrypt";
   Security::dumpAsn(RESIP_ASN_DECRYPT, text );

   BIO* in = BIO_new_mem_buf( (void*)text.c_str(), (int)text.size());
   resip_assert(in);
   InfoLog( << "created in BIO");

   BIO* out;
   out = BIO_new(BIO_s_mem());
   resip_assert(out);
   InfoLog( << "created out BIO" );

   PKCS7* pkcs7 = d2i_PKCS7_bio(in, 0);
   if ( !pkcs7 )
   {
      ErrLog( << "Problems doing decode of PKCS7 object" );

      while (1)
      {
         const char* file;
         int line;

         unsigned long code = ERR_get_error_line(&file,&line);
         if ( code == 0 )
         {
            break;
         }

         char buf[256];
         ERR_error_string_n(code,buf,sizeof(buf));
         ErrLog( << buf  );
         InfoLog( << "Error code = " << code << " file=" << file << " line=" << line );
      }

      BIO_free(in);
      BIO_free(out);

      return 0;
   }
   (void)BIO_flush(in);

   int type=OBJ_obj2nid(pkcs7->type);
   switch (type)
   {
      case NID_pkcs7_signed:
         InfoLog( << "data is pkcs7 signed" );
         break;
      case NID_pkcs7_signedAndEnveloped:
         InfoLog( << "data is pkcs7 signed and enveloped" );
         break;
      case NID_pkcs7_enveloped:
         InfoLog( << "data is pkcs7 enveloped" );
         break;
      case NID_pkcs7_data:
         InfoLog( << "data i pkcs7 data" );
         break;
      case NID_pkcs7_encrypted:
         InfoLog( << "data is pkcs7 encrypted " );
         break;
      case NID_pkcs7_digest:
         InfoLog( << "data is pkcs7 digest" );
         break;
      default:
         InfoLog( << "Unknown pkcs7 type" );
         break;
   }

   STACK_OF(X509)* certs = sk_X509_new_null();
   resip_assert( certs );

   //   flags |= PKCS7_NOVERIFY;

   resip_assert( mRootTlsCerts );

   switch (type)
   {
      case NID_pkcs7_signedAndEnveloped:
      {
         BIO_free(in);
         BIO_free(out);
         sk_X509_free(certs);
         PKCS7_free(pkcs7);
         throw Exception("Signed and enveloped is not supported", __FILE__, __LINE__);
      }
      break;

      case NID_pkcs7_enveloped:
      {
         if (mUserPrivateKeys.count(decryptorAor) == 0)
         {
            BIO_free(in);
            BIO_free(out);
            sk_X509_free(certs);
            PKCS7_free(pkcs7);
            InfoLog( << "Don't have a private key for " << decryptorAor << " for  PKCS7_decrypt" );
            throw Exception("Missing private key", __FILE__, __LINE__);
         }
         else if (mUserCerts.count(decryptorAor) == 0)
         {
            BIO_free(in);
            BIO_free(out);
            sk_X509_free(certs);
            PKCS7_free(pkcs7);
            InfoLog( << "Don't have a public cert for " << decryptorAor << " for  PKCS7_decrypt" );
            throw Exception("Missing cert", __FILE__, __LINE__);
         }

         EVP_PKEY* privateKey = mUserPrivateKeys[decryptorAor];
         X509* publicCert = mUserCerts[decryptorAor];

         if ( PKCS7_decrypt(pkcs7, privateKey, publicCert, out, flags ) != 1 )
         {
            ErrLog( << "Problems doing PKCS7_decrypt" );
            while (1)
            {
               const char* file;
               int line;

               unsigned long code = ERR_get_error_line(&file,&line);
               if ( code == 0 )
               {
                  break;
               }

               char buf[256];
               ERR_error_string_n(code,buf,sizeof(buf));
               ErrLog( << buf  );
               InfoLog( << "Error code = " << code << " file=" << file << " line=" << line );
            }

            BIO_free(in);
            BIO_free(out);
            sk_X509_free(certs);
            PKCS7_free(pkcs7);
            return 0;
         }
      }
      break;

      default:
         BIO_free(in);
         BIO_free(out);
         sk_X509_free(certs);
         PKCS7_free(pkcs7);
         ErrLog(<< "Got PKCS7 data that could not be handled type=" << type );
         throw Exception("Unsupported PKCS7 data type", __FILE__, __LINE__);
   }

   (void)BIO_flush(out);   
   BUF_MEM* bufMem;
   BIO_get_mem_ptr(out, &bufMem);

   int len = (int)bufMem->length;
   char* buffer = new char[len];
   memcpy(buffer, bufMem->data, len);

   (void)BIO_set_close(out, BIO_CLOSE);
   BIO_free(in);
   BIO_free(out);
   sk_X509_free(certs);
   PKCS7_free(pkcs7);

   // parse out the header information and form new body.
   // TODO !jf! this is a really crappy parser - shoudl do proper mime stuff 
   ParseBuffer pb(buffer, len);

   const char* headerStart = pb.position();

   // pull out contents type only
   pb.skipToChars("Content-Type");
   pb.assertNotEof();

   pb.skipToChar(Symbols::COLON[0]);
   pb.skipChar();
   pb.assertNotEof();

   pb.skipWhitespace();
   const char* typeStart = pb.position();
   pb.assertNotEof();

   // determine contents-type header buffer
   pb.skipToTermCRLF();
   pb.assertNotEof();

   ParseBuffer subPb(typeStart, pb.position() - typeStart);
   Mime contentType;
   contentType.parse(subPb);

   pb.assertNotEof();

   // determine body start
   pb.reset(typeStart);
   const char* bodyStart = pb.skipToChars(Symbols::CRLFCRLF);
   pb.assertNotEof();
   bodyStart += 4;

   // determine contents body buffer
   pb.skipToEnd();
   Data tmp;
   pb.data(tmp, bodyStart);
   // create contents against body
   Contents* ret = Contents::createContents(contentType, tmp);
   ret->addBuffer(buffer);
   
   // pre-parse headers
   ParseBuffer headersPb(headerStart, bodyStart-4-headerStart);
   ret->preParseHeaders(headersPb);

   InfoLog( << "Got body data of " << ret->getBodyData() );

   return ret;
}





Contents*
BaseSecurity::checkSignature(MultipartSignedContents* multi,
                             Data* signedBy,
                             SignatureStatus* sigStat )
{
   if ( multi->parts().size() != 2 )
   {
      ErrLog(<< "Trying to decode a message with wrong number of contents " << multi->parts().size());
      throw Exception("Invalid contents passed to checkSignature", __FILE__, __LINE__);
   }

   MultipartSignedContents::Parts::const_iterator it = multi->parts().begin();
   Contents* first = *it;
   ++it;
   resip_assert( it != multi->parts().end() );
   Contents* second = *it;

   resip_assert( second );
   resip_assert( first );

   InfoLog( << "message to signature-check is " << *first );

   Pkcs7SignedContents* sig = dynamic_cast<Pkcs7SignedContents*>( second );
   if ( !sig )
   {
      ErrLog( << "Don't know how to deal with signature type " );
      //throw Exception("Invalid contents passed to checkSignature", __FILE__,
      //__LINE__);
      return first;
   }
   Data sigData = sig->getBodyData();

   Data textData;
   DataStream strm( textData );
   first->encodeHeaders( strm );
   first->encode( strm );
   strm.flush();

   InfoLog( << "text <"    << textData.escaped() << ">" );
   InfoLog( << "signature <" << sigData.escaped() << ">" );

   static char RESIP_ASN_UNCODE_SIGNED_TEXT[] = "resip-asn-uncode-signed-text";
   static char RESIP_ASN_UNCODE_SIGNED_SIG[] = "resip-asn-uncode-signed-sig";

   Security::dumpAsn( RESIP_ASN_UNCODE_SIGNED_TEXT, textData );
   Security::dumpAsn( RESIP_ASN_UNCODE_SIGNED_SIG, sigData );

   BIO* in = BIO_new_mem_buf( (void*)sigData.data(),(int)sigData.size());
   resip_assert(in);
   InfoLog( << "created in BIO");

   BIO* out = BIO_new(BIO_s_mem());
   resip_assert(out);
   InfoLog( << "created out BIO" );

   BIO* pkcs7Bio = BIO_new_mem_buf( (void*) textData.data(),(int)textData.size());
   resip_assert(pkcs7Bio);
   InfoLog( << "created pkcs7 BIO");

   PKCS7* pkcs7 = d2i_PKCS7_bio(in, 0);
   if ( !pkcs7 )
   {
      ErrLog( << "Problems doing decode of PKCS7 object <"
              << sigData.escaped() << ">" );

      while (1)
      {
         const char* file;
         int line;

         unsigned long code = ERR_get_error_line(&file,&line);
         if ( code == 0 )
         {
            break;
         }

         char buf[256];
         ERR_error_string_n(code,buf,sizeof(buf));
         ErrLog( << buf  );
         InfoLog( <<"Error code = "<< code <<" file=" << file << " line=" << line );
      }
      BIO_free(in);
      BIO_free(out);
      BIO_free(pkcs7Bio);

      return first;
   }
   (void)BIO_flush(in);

   int type=OBJ_obj2nid(pkcs7->type);
   switch (type)
   {
      case NID_pkcs7_signed:
         InfoLog( << "data is pkcs7 signed" );
         break;
      case NID_pkcs7_signedAndEnveloped:
         InfoLog( << "data is pkcs7 signed and enveloped" );
         break;
      case NID_pkcs7_enveloped:
         InfoLog( << "data is pkcs7 enveloped" );
         break;
      case NID_pkcs7_data:
         InfoLog( << "data is pkcs7 data" );
         break;
      case NID_pkcs7_encrypted:
         InfoLog( << "data is pkcs7 encrypted " );
         break;
      case NID_pkcs7_digest:
         InfoLog( << "data is pkcs7 digest" );
         break;
      default:
         InfoLog( << "Unknown pkcs7 type" );
         break;
   }

   STACK_OF(X509)* certs = 0;
   certs = sk_X509_new_null();
   resip_assert( certs );

   if ( *signedBy == Data::Empty )
   {   
       //add all the certificates from mUserCerts stack to 'certs' stack  
       for(X509Map::iterator it = mUserCerts.begin(); it != mUserCerts.end(); it++)
       {
           resip_assert(it->second);
           sk_X509_push(certs, it->second);
       }
   }
   else
   {
      if (mUserCerts.count( *signedBy ))
      {
         InfoLog( <<"Adding cert from " <<  *signedBy << " to check sig" );
         X509* cert = mUserCerts[ *signedBy ];
         resip_assert(cert);
         sk_X509_push(certs, cert);
      }
   }

   int flags = 0;
   flags |= PKCS7_NOINTERN;

   // matches on certificate issuer and serial number - they must be unique
   STACK_OF(X509)* signers = PKCS7_get0_signers(pkcs7, certs, flags);
   if ( signers )
   {

      DebugLog( << "Found " << sk_X509_num(signers) << " signers." );
      for (int i=0; i<sk_X509_num(signers); i++)
      {
         X509* x = sk_X509_value(signers, i);
         InfoLog(<< "Got a signer <" << i << "> : " << getCertName(x) );

         GENERAL_NAMES* gens = 0;
         gens = (GENERAL_NAMES*)X509_get_ext_d2i(x, NID_subject_alt_name, NULL, NULL);

         for (int j = 0; j < sk_GENERAL_NAME_num(gens); j++)
         {
            GENERAL_NAME* gen = sk_GENERAL_NAME_value(gens, j);
            if (gen->type == GEN_URI)
            {
               ASN1_IA5STRING* uri = gen->d.uniformResourceIdentifier;
               Data name(uri->data, uri->length);
               InfoLog(<< "subjectAltName of signing cert contains <" << name << ">" );
               try
               {
                  Uri n(name);
                  if ( n.scheme() == "sip" )
                  {
                     *signedBy = name;
                     InfoLog(<< "choose <" << name << "> signature" );
                 }
               }
               catch (ParseException& e)
               {
                  ErrLog(<<"Caught exception: "<< e);
               }
            }
         }

         sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);
      }
   }
   else
   {
      BIO_free(in);
      BIO_free(out);
      BIO_free(pkcs7Bio);
      sk_X509_free(certs);
      PKCS7_free(pkcs7);
      *sigStat = SignatureIsBad;
      InfoLog(<< "No valid signers of this messages" );
      return first;
   }

#if 0
   // this is debugging information to get the serial number of the signed
   // information
   STACK_OF(PKCS7_SIGNER_INFO) *sinfos;
   PKCS7_SIGNER_INFO *si;
   PKCS7_ISSUER_AND_SERIAL *ias;
   ASN1_INTEGER* asnSerial;
   long longSerial;
   X509_NAME* name;

   sinfos = PKCS7_get_signer_info(pkcs7);
   if ( sinfos  )
   {
      int num = sk_PKCS7_SIGNER_INFO_num(sinfos);
      for ( int i=0; i<num; i++ )
      {
         si = sk_PKCS7_SIGNER_INFO_value (sinfos, i) ;
         ias = si->issuer_and_serial;
         name = ias->issuer;
         asnSerial = ias->serial;
         longSerial = ASN1_INTEGER_get( (ASN1_INTEGER*)asnSerial );
         InfoLog( << "Signed with serial " << hex << longSerial );
         InfoLog( << "Name " << name );
      }
   }
#endif

   resip_assert( mRootTlsCerts );

   switch (type)
   {
      case NID_pkcs7_signed:
      {
         int flags = 0;

         if (isSelfSigned(sk_X509_value(signers,0)))
         {
            flags |= PKCS7_NOVERIFY;
         }

         if ( PKCS7_verify(pkcs7, certs, mRootTlsCerts, pkcs7Bio, out, flags ) != 1 )
         {
            ErrLog( << "Problems doing PKCS7_verify" );

            if ( sigStat )
            {
               *sigStat = SignatureIsBad;
            }

            while (1)
            {
               const char* file;
               int line;

               unsigned long code = ERR_get_error_line(&file,&line);
               if ( code == 0 )
               {
                  break;
               }

               char buf[256];
               ERR_error_string_n(code,buf,sizeof(buf));
               ErrLog( << buf  );
               InfoLog( << "Error code = " << code << " file=" << file << " line=" << line );
            }
            BIO_free(in);
            BIO_free(out);
            BIO_free(pkcs7Bio);
            sk_X509_free(certs);
            PKCS7_free(pkcs7);
            return first;
         }
         if ( sigStat )
         {
            if ( (flags & PKCS7_NOVERIFY) )
            {
               if (isSelfSigned(sk_X509_value(signers,0)))
               {
                  DebugLog( << "Signature is selfSigned");
                  *sigStat = SignatureSelfSigned;
               }
               else
               {
                  DebugLog( << "Signature is notTrusted" );
                  *sigStat = SignatureNotTrusted;
               }
            }
            else
            {
               if (false) // !jf! TODO look for this cert in store
               {
                  DebugLog( << "Signature is trusted" );
                  *sigStat = SignatureTrusted;
               }
               else
               {
                  DebugLog( << "Signature is caTrusted" );
                  *sigStat = SignatureCATrusted;
               }
            }
         }
      }
      break;

      default:
         BIO_free(in);
         BIO_free(out);
         BIO_free(pkcs7Bio);
         sk_X509_free(certs);
         PKCS7_free(pkcs7);
         ErrLog(<< "Got PKCS7 data that could not be handled type=" << type );
         return 0;
   }

   (void)BIO_flush(out);
   char* outBuf=0;
   long size = BIO_get_mem_data(out,&outBuf);
   resip_assert( size >= 0 );

   Data outData(outBuf,size);
   DebugLog( << "uncoded body is <" << outData.escaped() << ">" );

   BIO_free(in);
   BIO_free(out);
   BIO_free(pkcs7Bio);
   sk_X509_free(certs);
   PKCS7_free(pkcs7);
   return first;
}


SSL_CTX*
BaseSecurity::getTlsCtx ()
{
   resip_assert(mTlsCtx);
   return   mTlsCtx;
}


SSL_CTX*
BaseSecurity::getSslCtx ()
{
   resip_assert(mSslCtx);
   return   mSslCtx;
}

void 
BaseSecurity::getCertNames(X509 *cert, std::list<PeerName> &peerNames,
                           bool useEmailAsSIP) 
{
   if(NULL == cert)
      return;

   if(peerNames.size() > 0)
      peerNames.clear();

   Data commonName;

   // look at the Common Name to find the peerName of the cert 
   X509_NAME* subject = X509_get_subject_name(cert);
   if(NULL == subject)
   {
      ErrLog( << "Invalid certificate: subject not found ");
      return;
   }

   int i =-1;
   while( true )
   {
      i = X509_NAME_get_index_by_NID(subject, NID_commonName,i);
      if ( i == -1 )
      {
         break;
      }
      resip_assert( i != -1 );
      X509_NAME_ENTRY* entry = X509_NAME_get_entry(subject,i);
      resip_assert( entry );
      
      ASN1_STRING*	s = X509_NAME_ENTRY_get_data(entry);
      resip_assert( s );
      
      int t = ASN1_STRING_type(s);
      int l = ASN1_STRING_length(s);
      const unsigned char* d = ASN1_STRING_get0_data(s);
      Data name(d,l);
      DebugLog( << "got x509 string type=" << t << " len="<< l << " data=" << d );
      resip_assert( name.size() == (unsigned)l );
      
      DebugLog( << "Found common name in cert of " << name );
      
      commonName = name;
   }

#if 0  // junk code to print certificates extentions for debugging 
   int numExt = X509_get_ext_count(cert);
   ErrLog(<< "Got peer certificate with " << numExt << " extentions" );

   for ( int i=0; i<numExt; i++ )
   {
      X509_EXTENSION* ext = X509_get_ext(cert,i);
      resip_assert( ext );
      
      const char* str = OBJ_nid2sn(OBJ_obj2nid(X509_EXTENSION_get_object(ext)));
      resip_assert(str);
      DebugLog(<< "Got certificate extention" << str );

      if  ( OBJ_obj2nid(X509_EXTENSION_get_object(ext)) == NID_subject_alt_name )
      {   
         DebugLog(<< "Got subjectAltName extention" );
      }
   }
#endif 

   // Look at the SubjectAltName, and if found, set as peerName
   GENERAL_NAMES* gens;
   gens = (GENERAL_NAMES*)X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
   for(int i = 0; i < sk_GENERAL_NAME_num(gens); i++)
   {  
      GENERAL_NAME* gen = sk_GENERAL_NAME_value(gens, i);

      DebugLog(<< "subjectAltName of cert contains type <" << gen->type << ">" );

      if (gen->type == GEN_DNS)
      {
         ASN1_IA5STRING* asn = gen->d.dNSName;
         Data dns(asn->data, asn->length);
         PeerName peerName(SubjectAltName, dns);
         peerNames.push_back(peerName);
         InfoLog(<< "subjectAltName of TLS session cert contains DNS <" << dns << ">" );
      }
          
      if (gen->type == GEN_EMAIL)
      {
         if(useEmailAsSIP)
         {
            ASN1_IA5STRING* asn = gen->d.rfc822Name;
            Data email(asn->data, asn->length);
            PeerName peerName(SubjectAltName, email);
            peerNames.push_back(peerName);
            InfoLog(<< "subjectAltName of TLS session cert contains EMAIL <" << email << ">" );
         }
         else
            DebugLog(<< "subjectAltName of cert has EMAIL type" );
      }
          
      if (gen->type == GEN_IPADD)
      {
         // RFC 3280 "For IP Version 4, as specified in RFC 791, the octet string
         // MUST contain exactly four octets.  For IP Version 6, as specified in
         // RFC 1883, the octet string MUST contain exactly sixteen octets."
         ASN1_OCTET_STRING* asn = gen->d.iPAddress;
         if (asn->length == 4)
         {
            uint32_t ip = (asn->data[0] << 24) |
                (asn->data[1] << 16) |
                (asn->data[2] << 8) |
                (asn->data[3]);

            sockaddr_in sa;
            sa.sin_family = AF_INET;
            sa.sin_addr.s_addr = htonl(ip);

            char addrStr[INET_ADDRSTRLEN];
            if (inet_ntop(sa.sin_family, &(sa.sin_addr), addrStr, INET_ADDRSTRLEN) != NULL)
            {
                Data ipv4(addrStr);
                PeerName peerName(SubjectAltName, ipv4);
                peerNames.push_back(peerName);
                InfoLog(<< "subjectAltName of TLS session cert contains IP ADDRESS <" << ipv4 << ">" );
            }
         }
         else if (asn->length == 16)
         {
            sockaddr_in6 sa;
            sa.sin6_family = AF_INET6;
            sa.sin6_addr.s6_addr[0] = asn->data[0];
            sa.sin6_addr.s6_addr[1] = asn->data[1];
            sa.sin6_addr.s6_addr[2] = asn->data[2];
            sa.sin6_addr.s6_addr[3] = asn->data[3];
            sa.sin6_addr.s6_addr[4] = asn->data[4];
            sa.sin6_addr.s6_addr[5] = asn->data[5];
            sa.sin6_addr.s6_addr[6] = asn->data[6];
            sa.sin6_addr.s6_addr[7] = asn->data[7];
            sa.sin6_addr.s6_addr[8] = asn->data[8];
            sa.sin6_addr.s6_addr[9] = asn->data[9];
            sa.sin6_addr.s6_addr[10] = asn->data[10];
            sa.sin6_addr.s6_addr[11] = asn->data[11];
            sa.sin6_addr.s6_addr[12] = asn->data[12];
            sa.sin6_addr.s6_addr[13] = asn->data[13];
            sa.sin6_addr.s6_addr[14] = asn->data[14];
            sa.sin6_addr.s6_addr[15] = asn->data[15];

            char addrStr[INET6_ADDRSTRLEN];
            if (inet_ntop(sa.sin6_family, &(sa.sin6_addr), addrStr, INET6_ADDRSTRLEN) != NULL)
            {
                Data ipv6(addrStr);
                PeerName peerName(SubjectAltName, ipv6);
                peerNames.push_back(peerName);
                InfoLog(<< "subjectAltName of TLS session cert contains IP ADDRESS <" << ipv6 << ">" );
            }
         }
         else
            DebugLog(<< "subjectAltName of cert contains invalid IP ADDRESS" );
      }

      if(gen->type == GEN_URI) 
      {
         ASN1_IA5STRING* asn = gen->d.uniformResourceIdentifier;
         Uri uri(Data(asn->data, asn->length));
         try
         {
             PeerName peerName(SubjectAltName, uri.host());
             peerNames.push_back(peerName);
             InfoLog(<< "subjectAltName of TLS session cert contains URI <" << uri << ">" );
         }
         catch (...)
         {
             InfoLog(<< "subjectAltName of TLS session cert contains unparseable URI");
         }
      }
   }
   sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);

   // If there are no peer names from the subjectAltName, then use the commonName
   if(peerNames.empty())
   {
      PeerName peerName(CommonName, commonName);
      peerNames.push_back(peerName);
   }
}

Data 
BaseSecurity::getCertName(X509 *cert)
{
   Data certName;
   std::list<PeerName> cNames;

   //get all the names (subjectAltName or CommonName)
   getCertNames(cert, cNames);

   //prefere the subjectAltName
   for(std::list<PeerName>::const_iterator it = cNames.begin(); it != cNames.end(); it++)
   {
      if(it->mType == SubjectAltName)
      {
         return it->mName;
      }
   }

   //if not subjectAltName found, get the CommonName
   for(std::list<PeerName>::const_iterator it = cNames.begin(); it != cNames.end(); it++)
   {
      if(it->mType == CommonName)
      {
         return it->mName;
      }
   }
   ErrLog(<< "This certificate doesn't have neither subjectAltName nor commonName");
   return Data::Empty;
}
/**
   Applies the certificate and domain name matching rules
*/
int 
BaseSecurity::matchHostName(const Data& certificateName, const Data& domainName)
{
   if(mAllowWildcardCertificates)
      return matchHostNameWithWildcards(certificateName,domainName);
   return isEqualNoCase(certificateName,domainName);
}
/**
   Converts a string containing an SSL type name to the corresponding
   enum value.
*/
SecurityTypes::SSLType
BaseSecurity::parseSSLType(const Data& typeName)
{
   if(typeName == "TLSv1")
   {
      return SecurityTypes::TLSv1;
   }
   if(typeName == "SSLv23")
   {
      return SecurityTypes::SSLv23;
   }
   Data error = "Not a recognized SSL type: " + typeName;
   throw invalid_argument(error.c_str());
}
/**
   Converts a string containing an OpenSSL option name
   (for SSL_CTX_set_options) to the numeric value from ssl.h
*/
long
BaseSecurity::parseOpenSSLCTXOption(const Data& optionName)
{
   if(optionName == "SSL_OP_ALL")
   {
      return SSL_OP_ALL;
   }
   if(optionName == "SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION")
   {
      return SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
   }
   if(optionName == "SSL_OP_CIPHER_SERVER_PREFERENCE")
   {
      return SSL_OP_CIPHER_SERVER_PREFERENCE;
   }
#if defined SSL_OP_CISCO_ANYCONNECT
   if(optionName == "SSL_OP_CISCO_ANYCONNECT")
   {
      return SSL_OP_CISCO_ANYCONNECT;
   }
#endif
   if(optionName == "SSL_OP_COOKIE_EXCHANGE")
   {
      return SSL_OP_COOKIE_EXCHANGE;
   }
#if defined SSL_OP_CRYPTOPRO_TLSEXT_BUG
   if(optionName == "SSL_OP_CRYPTOPRO_TLSEXT_BUG")
   {
      return SSL_OP_CRYPTOPRO_TLSEXT_BUG;
   }
#endif
   if(optionName == "SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS")
   {
      return SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
   }
   if(optionName == "SSL_OP_EPHEMERAL_RSA")
   {
      return SSL_OP_EPHEMERAL_RSA;
   }
   if(optionName == "SSL_OP_LEGACY_SERVER_CONNECT")
   {
      return SSL_OP_LEGACY_SERVER_CONNECT;
   }
   if(optionName == "SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER")
   {
      return SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER;
   }
   if(optionName == "SSL_OP_MICROSOFT_SESS_ID_BUG")
   {
      return SSL_OP_MICROSOFT_SESS_ID_BUG;
   }
   if(optionName == "SSL_OP_MSIE_SSLV2_RSA_PADDING")
   {
      return SSL_OP_MSIE_SSLV2_RSA_PADDING;
   }
   if(optionName == "SSL_OP_NETSCAPE_CA_DN_BUG")
   {
      return SSL_OP_NETSCAPE_CA_DN_BUG;
   }
   if(optionName == "SSL_OP_NETSCAPE_CHALLENGE_BUG")
   {
      return SSL_OP_NETSCAPE_CHALLENGE_BUG;
   }
   if(optionName == "SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG")
   {
      return SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG;
   }
   if(optionName == "SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG")
   {
      return SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG;
   }
#if defined SSL_OP_NO_COMPRESSION
   if(optionName == "SSL_OP_NO_COMPRESSION")
   {
      return SSL_OP_NO_COMPRESSION;
   }
#endif
   if(optionName == "SSL_OP_NO_QUERY_MTU")
   {
      return SSL_OP_NO_QUERY_MTU;
   }
   if(optionName == "SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION")
   {
      return SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION;
   }
   if(optionName == "SSL_OP_NO_SSLv2")
   {
      return SSL_OP_NO_SSLv2;
   }
   if(optionName == "SSL_OP_NO_SSLv3")
   {
      return SSL_OP_NO_SSLv3;
   }
   if(optionName == "SSL_OP_NO_TICKET")
   {
      return SSL_OP_NO_TICKET;
   }
   if(optionName == "SSL_OP_NO_TLSv1")
   {
      return SSL_OP_NO_TLSv1;
   }
#ifdef SSL_OP_NO_TLSv1_1
   if(optionName == "SSL_OP_NO_TLSv1_1")
   {
      return SSL_OP_NO_TLSv1_1;
   }
#endif
#ifdef SSL_OP_NO_TLSv1_2
   if(optionName == "SSL_OP_NO_TLSv1_2")
   {
      return SSL_OP_NO_TLSv1_2;
   }
#endif
   if(optionName == "SSL_OP_PKCS1_CHECK_1")
   {
      return SSL_OP_PKCS1_CHECK_1;
   }
   if(optionName == "SSL_OP_PKCS1_CHECK_2")
   {
      return SSL_OP_PKCS1_CHECK_2;
   }
#ifdef SSL_OP_SAFARI_ECDHE_ECDSA_BUG
   if(optionName == "SSL_OP_SAFARI_ECDHE_ECDSA_BUG")
   {
      return SSL_OP_SAFARI_ECDHE_ECDSA_BUG;
   }
#endif
   if(optionName == "SSL_OP_SINGLE_DH_USE")
   {
      return SSL_OP_SINGLE_DH_USE;
   }
   if(optionName == "SSL_OP_SINGLE_ECDH_USE")
   {
      return SSL_OP_SINGLE_ECDH_USE;
   }
   if(optionName == "SSL_OP_SSLEAY_080_CLIENT_DH_BUG")
   {
      return SSL_OP_SSLEAY_080_CLIENT_DH_BUG;
   }
   if(optionName == "SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG")
   {
      return SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG;
   }
   if(optionName == "SSL_OP_TLS_BLOCK_PADDING_BUG")
   {
      return SSL_OP_TLS_BLOCK_PADDING_BUG;
   }
   if(optionName == "SSL_OP_TLS_D5_BUG")
   {
      return SSL_OP_TLS_D5_BUG;
   }
   if(optionName == "SSL_OP_TLS_ROLLBACK_BUG")
   {
      return SSL_OP_TLS_ROLLBACK_BUG;
   }
   Data error = "Not a recognized OpenSSL option name: " + optionName;
   throw invalid_argument(error.c_str());
}
/**
   Does a wildcard match on domain and certificate name
   @todo    looks incomplete, make better
*/
int 
BaseSecurity::matchHostNameWithWildcards(const Data& certificateName, const Data& domainName)
{
   const char *dot = NULL;

   const char *certName = certificateName.c_str();
   if(certName == NULL)
      return 0;

   const char *domName = domainName.c_str();
   if(domName == NULL)
      return 0;

   dot = strchr(domName, '.');
   if (dot == NULL)
   {
      char *pnt = (char *)strchr(certName, '.'); // bad
      /* hostname is not fully-qualified; unqualify the certName. */
      if (pnt != NULL) 
      {
         *pnt = '\0';
      }
   }
   else 
   {
      if (strncmp(certName, "*.", 2) == 0) 
      {
         domName = dot + 1;
         certName += 2;
      }
   }
   return !strcasecmp(certName, domName);
}

bool
BaseSecurity::isSelfSigned(const X509 *cert)
{
   X509 *mutableCert = const_cast<X509*>(cert);
   int iRet = X509_NAME_cmp(X509_get_issuer_name(mutableCert), X509_get_subject_name(mutableCert));
   return (iRet == 0);
}

void
BaseSecurity::dumpAsn( char* name, Data data)
{
#if 0 // for debugging
   resip_assert(name);

   if (true) // dump asn.1 stuff to debug file
   {
      ofstream strm(name, std::ios_base::trunc);
      if ( !strm )
      {
         ErrLog( <<"Could not write to " << name );
      }
      else
      {
         strm.write( data.data() , data.size() );
      }
      strm.flush();
   }
#endif
}

X509*     
BaseSecurity::getDomainCert( const Data& domain )
{
   return mDomainCerts.count(domain) ? mDomainCerts[domain] : 0;
}

X509*     
BaseSecurity::getUserCert( const Data& aor )
{
   return mUserCerts.count(aor) ? mUserCerts[aor] : 0;
}

EVP_PKEY* 
BaseSecurity::getDomainKey(  const Data& domain )
{
   return mDomainPrivateKeys.count(domain) ? mDomainPrivateKeys[domain] : 0;
}

EVP_PKEY*
BaseSecurity::getUserPrivateKey( const Data& aor )
{
   return mUserPrivateKeys.count(aor) ? mUserPrivateKeys[aor] : 0;
}

void
BaseSecurity::setDHParams(SSL_CTX* ctx)
{
   if(mDHParamsFilename.empty())
   {
      WarningLog(<< "unable to load DH parameters (required for PFS): TlsDHParamsFilename not specified");
   }
   else
   {
      DebugLog(<< "attempting to read DH parameters from " << mDHParamsFilename);

      BIO* bio = BIO_new_file(mDHParamsFilename.c_str(), "r");
      if(bio == NULL)
      {
         WarningLog(<< "unable to load DH parameters (required for PFS): BIO_new_file failed to open file " << mDHParamsFilename);
      }

      DH* dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
      if(dh == NULL)
      {
         WarningLog(<< "unable to load DH parameters (required for PFS): PEM_read_bio_DHparams failed for file " << mDHParamsFilename);
      }
      else
      {
         if(!SSL_CTX_set_tmp_dh(ctx, dh))
         {
            WarningLog(<< "unable to load DH parameters (required for PFS): SSL_CTX_set_tmp_dh failed for file " << mDHParamsFilename);
         }
         else
         {
            long options = SSL_OP_CIPHER_SERVER_PREFERENCE |
#if !defined(OPENSSL_NO_ECDH) && OPENSSL_VERSION_NUMBER >= 0x10000000L
                           SSL_OP_SINGLE_ECDH_USE |
#endif
                           SSL_OP_SINGLE_DH_USE;
            options = SSL_CTX_set_options(ctx, options);
            DebugLog(<<"DH parameters loaded, PFS cipher-suites enabled");
         }
         DH_free(dh);
      }
      BIO_free(bio);
   }

#if OPENSSL_VERSION_NUMBER < 0x10100000L

#ifndef SSL_CTRL_SET_ECDH_AUTO
#define SSL_CTRL_SET_ECDH_AUTO 94
#endif

   // FIXME: add WarningLog statements to the block below if it fails

   /* SSL_CTX_set_ecdh_auto(ctx,on) requires OpenSSL 1.0.2 which wraps: */
   if (SSL_CTX_ctrl(ctx, SSL_CTRL_SET_ECDH_AUTO, 1, NULL))
   {
      DebugLog(<<"ECDH initialized");
   }
   else
   {
#if !defined(OPENSSL_NO_ECDH) && OPENSSL_VERSION_NUMBER >= 0x10000000L
      EC_KEY* ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
      if (ecdh != NULL)
      {
         if (SSL_CTX_set_tmp_ecdh(ctx, ecdh))
         {
	    DebugLog(<<"ECDH initialized");
	 }
         else
         {
            WarningLog(<<"unable to initialize ECDH: SSL_CTX_set_tmp_ecdh failed");
         }
         EC_KEY_free(ecdh);
      }
      else
      {
         WarningLog(<<"unable to initialize ECDH: EC_KEY_new_by_curve_name failed");
      }
#else
      WarningLog(<<"unable to initialize ECDH: SSL_CTX_ctrl failed, OPENSSL_NO_ECDH defined or repro was compiled with an old OpenSSL version");
#endif
   }

#endif


}

#endif




/* ====================================================================
* The Vovida Software License, Version 1.0
*
* Copyright (c) 2002-2005 Vovida Networks, Inc.  All rights reserved.
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
