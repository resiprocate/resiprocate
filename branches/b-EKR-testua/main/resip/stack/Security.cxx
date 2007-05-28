#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include <ostream>
#include <fstream>

#include "resip/stack/Contents.hxx"
#include "resip/stack/MultipartSignedContents.hxx"
#include "resip/stack/Pkcs7Contents.hxx"
#include "resip/stack/PlainContents.hxx"
#include "resip/stack/Security.hxx"
#include "resip/stack/SecurityAttributes.hxx"
#include "resip/stack/Transport.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "rutil/SHA1Stream.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Timer.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/FileSystem.hxx"
#include "rutil/WinLeakCheck.hxx"


#if defined(USE_SSL)

#if !defined(WIN32)
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <dirent.h>
#endif

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

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

static const Data PEM(".pem");

static const Data 
pemTypePrefixes(  Security::PEMType pType )
{
   static const Data rootCert("root_cert_");
   static const Data domainCert("domain_cert_");
   static const Data domainKey("domain_key_");
   static const Data userCert("user_cert_");
   static const Data userKey("user_key_");
   static const Data unknownKey("user_key_");

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
         assert(0);
      }
   }
   return unknownKey;
}

static Data
readIntoData(const Data& filename)
{
   DebugLog( << "Trying to read file " << filename );
   
   ifstream is;
   is.open(filename.c_str(), ios::binary );
   if ( !is.is_open() )
   {
      ErrLog( << "Could not open file " << filename << " for read");
      throw BaseSecurity::Exception("Could not read file ", 
                                    __FILE__,__LINE__);
   }
   
   assert(is.is_open());
   
   int length = 0;
   
   // get length of file:
#if !defined(__MSL_CPP__) || (__MSL_CPP_ >= 0x00012000)
   is.seekg (0, ios::end);
   length = is.tellg();
   is.seekg (0, ios::beg);
#else
   // this is a work around for a bug in CodeWarrior 9's implementation of seekg.
   // http://groups.google.ca/group/comp.sys.mac.programmer.codewarrior/browse_frm/thread/a4279eb75f3bd55a
   FILE * tmpFile = fopen(filename.c_str(), "r+b");
   assert(tmpFile != NULL);
   fseek(tmpFile, 0, SEEK_END);
   length = ftell(tmpFile);
   fseek(tmpFile, 0, SEEK_SET);
#endif // __MWERKS__
   
   // tellg/tell will return -1 if the stream is bad
   if (length == -1)
   {
      ErrLog( << "Could not seek into file " << filename);
      throw BaseSecurity::Exception("Could not seek into file ", 
                                    __FILE__,__LINE__);
   }
   
   // !jf! +1 is a workaround for a bug in Data::c_str() that adds the 0 without
   // resizing. 
   char* buffer = new char [length+1]; 
   
   // read data as a block:
   is.read (buffer,length);
   
   Data target(Data::Take, buffer, length);
   
   is.close();
   
   return target;
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
   char cBuf1[500];
   char cBuf2[500];
   X509 *pErrCert;
   int iErr = 0;
   int iDepth = 0;
   pErrCert = X509_STORE_CTX_get_current_cert(pInStore);
   iErr = X509_STORE_CTX_get_error(pInStore);
   iDepth = X509_STORE_CTX_get_error_depth(pInStore);

   if (NULL != pErrCert)
      X509_NAME_oneline(X509_get_subject_name(pErrCert),cBuf1,256);

   sprintf(cBuf2,", depth=%d %s\n",iDepth,cBuf1);
   if(!iInCode)
      ErrLog(<< "Error when verifying server's chain of certificates: " << X509_verify_cert_error_string(pInStore->error) << cBuf2 );
 
   return iInCode;
}
 
}

BaseSecurity::CipherList BaseSecurity::ExportableSuite("!SSLv2:aRSA+AES:aDSS+AES:@STRENGTH:aRSA+3DES:aDSS+3DES:aRSA+RC4+MEDIUM:aDSS+RC4+MEDIUM:aRSA+DES:aDSS+DES:aRSA+RC4:aDSS+RC4");
BaseSecurity::CipherList BaseSecurity::StrongestSuite("!SSLv2:aRSA+AES:aDSS+AES:@STRENGTH:aRSA+3DES:aDSS+3DES");

Security::Security(const CipherList& cipherSuite, bool serverAuthentication) : 
   BaseSecurity(cipherSuite),
   mServerAuthentication(serverAuthentication)
{
#ifdef WIN32
   mPath = "C:\\sipCerts\\";
#else
   mPath = getenv("HOME");
   mPath += "/.sipCerts/";
#endif
}

Security::Security(const Data& directory, const CipherList& cipherSuite, bool serverAuthentication) : 
   BaseSecurity(cipherSuite), 
   mPath(directory),
   mServerAuthentication(serverAuthentication)
{
   // since the preloader won't work otherwise and VERY difficult to figure
   // out. 
   if ( !mPath.postfix(Symbols::SLASH))
   {
      mPath += Symbols::SLASH;
   }
}


void
Security::preload()
{
#if 1
   FileSystem::Directory dir(mPath);
   FileSystem::Directory::iterator it(dir);
   for (; it != dir.end(); ++it)
   {
      Data name = *it;
           
      if (name.postfix(PEM))
      {
         Data fileName = mPath + name;
         
         DebugLog(<< "Trying to load file " << name );
         try
         {
            if (name.prefix(pemTypePrefixes(UserCert)))
            {
               addCertPEM( UserCert, getAor(name, UserCert), readIntoData(fileName), false );
            }
            else if (name.prefix(pemTypePrefixes(UserPrivateKey)))
            {
               addPrivateKeyPEM( UserPrivateKey, getAor(name, UserPrivateKey), readIntoData(fileName), false);
            }
            else if (name.prefix(pemTypePrefixes(DomainCert)))
            {
               addCertPEM( DomainCert, getAor(name, DomainCert), readIntoData(fileName), false);
            }
            else if (name.prefix(pemTypePrefixes(DomainPrivateKey)))
            {
               addPrivateKeyPEM( DomainPrivateKey, getAor(name, DomainPrivateKey), readIntoData(fileName), false);
            }
            else if (name.prefix(pemTypePrefixes(RootCert)))
            {
               addRootCertPEM(readIntoData(fileName));
            }
         }
         catch (...)
         {  
            ErrLog(<< "Some problem reading " << fileName );
         }
         
         InfoLog(<<"Sucessfully loaded " << fileName );
      }
   }
#else
   // CJ - TODO - delete this old crap 
   DIR* dir = opendir( mPath.c_str() );

   if (!dir )
   {
      ErrLog( << "Error reading public key directory  " << mPath );
      return;      
   }

   struct dirent * d = 0;
   while (1)
   {
      d = readdir(dir);
      if ( !d )
      {
         break;
      }

      Data name( d->d_name );
      Data fileName = mPath+name;
      
      if (name.postfix(PEM))
      {
         InfoLog( << "Going to try to read file " << fileName );
         
         try
         {
            if (name.prefix(pemTypePrefixes(UserCert)))
            {
               addCertPEM( UserCert, getAor(name, UserCert), readIntoData(fileName), false );
            }
            else if (name.prefix(pemTypePrefixes(UserPrivateKey)))
            {
               addPrivateKeyPEM( UserPrivateKey, getAor(name, UserPrivateKey), readIntoData(fileName), false);
            }
            else if (name.prefix(pemTypePrefixes(DomainCert)))
            {
               addCertPEM( DomainCert, getAor(name, DomainCert), readIntoData(fileName), false);
            }
            else if (name.prefix(pemTypePrefixes(DomainPrivateKey)))
            {
               addPrivateKeyPEM( DomainPrivateKey, getAor(name, DomainPrivateKey), readIntoData(fileName), false);
            }
            else if (name.prefix(pemTypePrefixes(RootCert)))
            {
               addRootCertPEM(readIntoData(fileName));
            }
         }
         catch (...)
         {  
            ErrLog(<< "Some problem reading " << fileName );
         }
         
         InfoLog(<<"Sucessfully loaded " << fileName );
      }
   }
   closedir( dir );
#endif
}


void
Security::onReadPEM(const Data& name, PEMType type, Data& buffer) const
{
   Data filename = mPath + pemTypePrefixes(type) + name + PEM;

   InfoLog (<< "Reading PEM file " << filename << " into " << name);
   // .dlb. extra copy
   buffer = readIntoData(filename);
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
   assert(0);
   // TODO - should delete file 
}


void
BaseSecurity::addCertDER (PEMType type, 
                          const Data& key, 
                          const Data& certDER, 
                          bool write) const
{
   assert( !certDER.empty() );

   X509* cert = 0;

#if (OPENSSL_VERSION_NUMBER < 0x0090800fL )
   unsigned char* in = (unsigned char*)certDER.data();
#else
   unsigned const char* in = (unsigned const char*)certDER.data();
#endif

   if (d2i_X509(&cert,&in,certDER.size()) == 0)
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
                          bool write) const
{
   assert( !certPEM.empty() );
   X509* cert=NULL;
   
   BIO* in = BIO_new_mem_buf(const_cast<char*>(certPEM.c_str()), -1);
   if ( !in )
   {
      ErrLog(<< "Could not create BIO buffer from '" << certPEM << "'");
      throw Exception("Could not create BIO buffer", __FILE__,__LINE__);
   }
   cert = PEM_read_bio_X509(in,0,0,0);
   if (cert == NULL)
   {
	   ErrLog( << "Could not load X509 cert from '" << certPEM << "'" );
	   BIO_free(in); 
	   throw Exception("Could not load X509 cert from BIO buffer", __FILE__,__LINE__);
   }
   
   addCertX509(type,name,cert,write);
   
   BIO_free(in);
}


void
BaseSecurity::addCertX509(PEMType type, const Data& key, X509* cert, bool write) const
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
         X509_STORE_add_cert(mRootTlsCerts,cert);
         X509_STORE_add_cert(mRootSslCerts,cert);
         X509_free(cert);
      }
      break;
      default:
      {
         assert(0);
      }
   }
   
   if (write)
   {
      // creates a read/write BIO buffer.
      BIO *out = BIO_new(BIO_s_mem());
      assert(out);
      try
      {
         int ret = PEM_write_bio_X509(out, cert);
         assert(ret);
         
         BIO_flush(out);
         // get content in BIO buffer to our buffer.
         char* p = 0;
         size_t len = BIO_get_mem_data(out,&p);
         assert(p);
         assert(len);
         Data  buf(Data::Borrow, p, len);
         
         this->onWritePEM(key, type, buf);
      }
      catch(...)
      {
         BIO_free(out);
         throw;
      }
      BIO_free(out);
   }
}


bool
BaseSecurity::hasCert (PEMType type, const Data& aor) const
{
   assert( !aor.empty() );
   X509Map& certs = (type == DomainCert ? mDomainCerts : mUserCerts);

   X509Map::iterator where = certs.find(aor);
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
   catch (...)
   {
      return   false;
   }

   assert(  certs.find(aor) != certs.end() );
   
   return   true;
}


void
BaseSecurity::removeCert (PEMType type, const Data& aor)
{
   assert( !aor.empty() );
   X509Map& certs = (type == DomainCert ? mDomainCerts : mUserCerts);

   X509Map::iterator iter = certs.find(aor);
   if (iter != certs.end())
   {
      X509_free(iter->second);
      certs.erase(iter);

      onRemovePEM(aor, type);
   } 

   assert(  certs.find(aor) == certs.end() );
}


Data
BaseSecurity::getCertDER (PEMType type, const Data& key) const
{
   assert( !key.empty() );

   if (hasCert(type, key) == false)
   {
      ErrLog(<< "Could find certificate for '" << key << "'");
      throw BaseSecurity::Exception("Could not find certificate", __FILE__,__LINE__);
   }

   X509Map& certs = (type == DomainCert ? mDomainCerts : mUserCerts);
   BaseSecurity::X509Map::iterator where = certs.find(key);
   if (where == certs.end())
   {
      // not supposed to happen,
      // hasCert() should have inserted a value into certs
      // or we should have throwed.
      assert(0);
   }

   //assert(0); // the code following this has no hope of working 
   
   X509* x = where->second;
   unsigned char* buffer=0;
   int len = i2d_X509(x, &buffer);

   // !kh!
   // Although len == 0 is not an error, I am not sure what quite to do.
   // Asserting for now.
   assert(len != 0);
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
                                bool write) const 
{ 
   PrivateKeyMap& privateKeys = (type == DomainPrivateKey ? 
                                 mDomainPrivateKeys : mUserPrivateKeys);

   /*
   // make a copy of the the key 
   assert( EVP_PKEY_type(pKey->type) == EVP_PKEY_RSA );
   RSA* rsa = EVP_PKEY_get1_RSA(pKey);
   assert( rsa );
   EVP_PKEY* nKey = EVP_PKEY_new();
   assert( nKey );
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
            klen = iter->second.size();
         }
      }

      BIO *bio = BIO_new(BIO_s_mem());
      assert(bio);
      try
      {
         assert( EVP_des_ede3_cbc() );
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
         assert(ret);
         
         BIO_flush(bio);
         char* p = 0;
         size_t len = BIO_get_mem_data(bio,&p);
         assert(p);
         assert(len);
         Data  pem(Data::Borrow, p, len);
         onWritePEM(name, type, pem );
      }
      catch(...)
      {
         BIO_free(bio);
         throw;
      }
      BIO_free(bio);
   }
}


void
BaseSecurity::addPrivateKeyDER( PEMType type,
                                const Data& name,
                                const Data& privateKeyDER,
                                bool write ) const
{
   assert( !name.empty() );
   assert( !privateKeyDER.empty() );

   char* passPhrase = 0;
   if (type != DomainPrivateKey)
   {
      PassPhraseMap::const_iterator iter = mUserPassPhrases.find(name);
      if(iter != mUserPassPhrases.end())
      {
         passPhrase = const_cast<char*>(iter->second.c_str());
      }
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
   catch(...)
   {
      BIO_free(in);
      throw;
   }
   
   BIO_free(in);
}


void
BaseSecurity::addPrivateKeyPEM( PEMType type,
                                const Data& name,
                                const Data& privateKeyPEM,
                                bool write ) const 
{
   assert( !name.empty() );
   assert( !privateKeyPEM.empty() );

   BIO* in = BIO_new_mem_buf(const_cast<char*>(privateKeyPEM.c_str()), -1);
   if ( !in )
   {
      ErrLog(<< "Could create BIO buffer from '" << privateKeyPEM << "'");
      throw Exception("Could not create BIO buffer", __FILE__,__LINE__);
   }
   
   char* passPhrase = 0;
   try
   {
      if (type == UserPrivateKey)
      {
         PassPhraseMap::const_iterator iter = mUserPassPhrases.find(name);
         if(iter != mUserPassPhrases.end())
         {
            passPhrase = const_cast<char*>(iter->second.c_str());
         }
      }
      
      EVP_PKEY* privateKey=0;
      if ( ( privateKey = PEM_read_bio_PrivateKey(in, NULL, 0, passPhrase)) == NULL)
      {
         ErrLog(<< "Could not read private key from <" << privateKeyPEM << ">" );
         throw Exception("Could not read private key ", __FILE__,__LINE__);
      }
      
      addPrivateKeyPKEY(type,name,privateKey,write);
   }
   catch(...)
   {
      BIO_free(in);
      throw;
   }

   BIO_free(in);
}


bool
BaseSecurity::hasPrivateKey( PEMType type,
                             const Data& key ) const
{
   assert( !key.empty() );

   PrivateKeyMap& privateKeys = (type == DomainPrivateKey 
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
   catch(...)
   {
      return   false;
   }

   return   true;
}


Data
BaseSecurity::getPrivateKeyPEM( PEMType type,
                                const Data& key) const
{
   assert( !key.empty() );

   if ( !hasPrivateKey(type, key) )
   {
      ErrLog(<< "Could find private key for '" << key << "'");
      throw Exception("Could not find private key", __FILE__,__LINE__);
   }

   PrivateKeyMap& privateKeys = (type == DomainPrivateKey ? mDomainPrivateKeys : mUserPrivateKeys);

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

   assert(0); // TODO - following code has no hope of working 
    
   // !kh!
   // creates a read/write BIO buffer.
   BIO *out = BIO_new(BIO_s_mem());
   assert(out);
   EVP_PKEY* pk = where->second;
   assert(pk);

   // write pk to out using key phrase p, with no cipher.
   int ret = PEM_write_bio_PrivateKey(out, pk, 0, 0, 0, 0, p);  // paraters
                                                                // are in the wrong order
   assert(ret == 1);

   // get content in BIO buffer to our buffer.
   // hand our buffer to a Data object.
   BIO_flush(out);
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
   assert( !key.empty() );

   if ( !hasPrivateKey(type, key) )
   {
      ErrLog(<< "Could find private key for '" << key << "'");
      throw Exception("Could not find private key", __FILE__,__LINE__);
   }

   PrivateKeyMap& privateKeys = (type == DomainPrivateKey ? mDomainPrivateKeys : mUserPrivateKeys);

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

   assert(0); // TODO - following code has no hope of working 
    
   // !kh!
   // creates a read/write BIO buffer.
   BIO *out = BIO_new(BIO_s_mem());
   assert(out);
   EVP_PKEY* pk = where->second;
   assert(pk);

   // write pk to out using key phrase p, with no cipher.
   int ret = i2d_PKCS8PrivateKey_bio(out, pk, 0, 0, 0, 0, p);
   assert(ret == 1);

   // get content in BIO buffer to our buffer.
   // hand our buffer to a Data object.
   BIO_flush(out);
   char* buf = 0;
   int len = BIO_get_mem_data(out, &buf);
   Data retVal(Data::Borrow, buf, len);

   BIO_free(out);
   
   return retVal;
}


void
BaseSecurity::removePrivateKey(PEMType type, const Data& key)
{
   assert( !key.empty() );

   PrivateKeyMap& privateKeys = (type == DomainPrivateKey ? mDomainPrivateKeys : mUserPrivateKeys);

   assert( !key.empty() );
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


BaseSecurity::BaseSecurity (const CipherList& cipherSuite) :
   mTlsCtx(0),
   mSslCtx(0),
   mRootTlsCerts(0),                    
   mRootSslCerts(0)
{ 
   DebugLog(<< "BaseSecurity::BaseSecurity");
   
   int ret;
   initialize(); 
   
   mRootTlsCerts = X509_STORE_new();
   mRootSslCerts = X509_STORE_new();
   assert(mRootTlsCerts && mRootSslCerts);

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
   assert(mTlsCtx);

   SSL_CTX_set_cert_store(mTlsCtx, mRootTlsCerts);
   SSL_CTX_set_verify(mTlsCtx, SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE, verifyCallback);
   ret = SSL_CTX_set_cipher_list(mTlsCtx, cipherSuite.cipherList().c_str());
   assert(ret);
   
   mSslCtx = SSL_CTX_new( SSLv23_method() );
   assert(mSslCtx);
   SSL_CTX_set_cert_store(mSslCtx, mRootSslCerts);
   SSL_CTX_set_verify(mSslCtx, SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE, verifyCallback);
   ret = SSL_CTX_set_cipher_list(mSslCtx,cipherSuite.cipherList().c_str());
   assert(ret);
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
         
BaseSecurity::~BaseSecurity ()
{
   DebugLog(<< "BaseSecurity::~BaseSecurity");

   // cleanup certificates
   clearMap(mDomainCerts, X509_free);
   clearMap(mUserCerts, X509_free);

   // cleanup private keys
   clearMap(mDomainPrivateKeys, EVP_PKEY_free);
   clearMap(mUserPrivateKeys, EVP_PKEY_free);

/*
// !abr! This intentional memory leak appears to be unnecessary. Derek to verify.
// !dcm! - still crashses...I think if there were no certs to load. 

   // sailesh@counterpath.com : this code leaks memory but it's necessary on
   // mac and windows. if we don't have this code then SSL_CTX_new( TLSv1_method() )
   // returns NULL when BaseSecurity::BaseSecurity() is called the second time.
   X509_STORE_free(mRootTlsCerts);
   X509_STORE_free(mRootSslCerts);
*/

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
   assert(0); // TODO 
   return   CertificateInfoContainer();
}


void
BaseSecurity::addRootCertPEM(const Data& x509PEMEncodedRootCerts)
{ 
   assert( mRootTlsCerts && mRootSslCerts );
#if 1
   addCertPEM(RootCert,Data::Empty,x509PEMEncodedRootCerts,false);
#else
   assert( !x509PEMEncodedRootCerts.empty() );

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
   
   assert( mRootCerts );

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
   addCertPEM(DomainCert, domainName, certPEM, true);
}


void
BaseSecurity::addDomainCertDER(const Data& domainName, const Data& certDER)
{
   addCertDER(DomainCert, domainName, certDER, true);
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
BaseSecurity::addDomainPrivateKeyPEM(const Data& domainName, const Data& privateKeyPEM)
{
   addPrivateKeyPEM(DomainPrivateKey, domainName, privateKeyPEM, true);
}


bool
BaseSecurity::hasDomainPrivateKey(const Data& domainName) const
{
   return   hasPrivateKey(DomainPrivateKey, domainName);
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
   addCertPEM(UserCert, aor, certPEM, true);
}


void
BaseSecurity::addUserCertDER(const Data& aor, const Data& certDER)
{
   addCertDER(UserCert, aor, certDER, true);
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
   assert(!aor.empty());

   PassPhraseMap::iterator iter = mUserPassPhrases.find(aor);
   if (iter == mUserPassPhrases.end())
   {
      mUserPassPhrases.insert(std::make_pair(aor, passPhrase));
   }
}


bool
BaseSecurity::hasUserPassPhrase(const Data& aor) const
{
   assert(aor.empty());

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
   assert(aor.empty());

   PassPhraseMap::iterator iter = mUserPassPhrases.find(aor);
   if(iter != mUserPassPhrases.end())
   {
      mUserPassPhrases.erase(iter);
   }
}


Data
BaseSecurity::getUserPassPhrase(const Data& aor) const
{
   assert(aor.empty());

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
BaseSecurity::addUserPrivateKeyPEM(const Data& aor, const Data& cert)
{
   addPrivateKeyPEM(UserPrivateKey, aor, cert, true);
}


void
BaseSecurity::addUserPrivateKeyDER(const Data& aor, const Data& cert)
{
   addPrivateKeyDER(UserPrivateKey, aor, cert, true);
}


bool
BaseSecurity::hasUserPrivateKey(const Data& aor) const
{
   return   hasPrivateKey(UserPrivateKey, aor);
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
   assert(EVP_sha1());

   RSA* rsa = RSA_generate_key(keyLen, RSA_F4, NULL, NULL);
   assert(rsa);    // couldn't make key pair
   
   EVP_PKEY* privkey = EVP_PKEY_new();
   assert(privkey);
   ret = EVP_PKEY_set1_RSA(privkey, rsa);
   assert(ret);

   X509* cert = X509_new();
   assert(cert);
   
   X509_NAME* subject = X509_NAME_new();
   X509_EXTENSION* ext = X509_EXTENSION_new();
   
   // set version to X509v3 (starts from 0)
   X509_set_version(cert, 2L);
   
   int serial = Random::getRandom();  // get an int worth of randomness
   assert(sizeof(int)==4);
   ASN1_INTEGER_set(X509_get_serialNumber(cert),serial);
   
   ret = X509_NAME_add_entry_by_txt( subject, "O",  MBSTRING_ASC, 
                                     (unsigned char *) domain.data(), domain.size(), 
                                     -1, 0);
   assert(ret);
   ret = X509_NAME_add_entry_by_txt( subject, "CN", MBSTRING_ASC, 
                                     (unsigned char *) aor.data(), aor.size(), 
                                     -1, 0);
   assert(ret);
   
   ret = X509_set_issuer_name(cert, subject);
   assert(ret);
   ret = X509_set_subject_name(cert, subject);
   assert(ret);
   
   const long duration = 60*60*24*expireDays;   
   X509_gmtime_adj(X509_get_notBefore(cert),0);
   X509_gmtime_adj(X509_get_notAfter(cert), duration);
   
   ret = X509_set_pubkey(cert, privkey);
   assert(ret);
   
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
   assert(ret);
   X509_EXTENSION_free(ext);
   
   // TODO add extensions NID_subject_key_identifier and NID_authority_key_identifier
   
   ret = X509_sign(cert, privkey, EVP_sha1());
   assert(ret);
   
   addCertX509( UserCert, aor, cert, true /* write */ );
   addPrivateKeyPKEY( UserPrivateKey, aor, privkey, true /* write */ );
}


MultipartSignedContents*
BaseSecurity::sign(const Data& senderAor, Contents* contents)
{
   assert( contents );

   // form the multipart
   MultipartSignedContents* multi = new MultipartSignedContents;
   multi->header(h_ContentType).param( p_micalg ) = "sha1";
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
   int s = bodyData.size();
   BIO* in=BIO_new_mem_buf( (void*)p,s);
   assert(in);
   DebugLog( << "created in BIO");

   BIO* out = BIO_new(BIO_s_mem()); // TODO - mem leak 
   assert(out);
   DebugLog( << "created out BIO" );

   STACK_OF(X509)* chain = sk_X509_new_null();
   assert(chain);

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
   assert(rv);

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
   BIO_flush(out);

   char* outBuf=0;
   long size = BIO_get_mem_data(out,&outBuf);
   assert( size > 0 );

   Data outData(outBuf,size);
   static char RESIP_SIGN_OUT_SIG[] = "resip-sign-out-sig";
   Security::dumpAsn(RESIP_SIGN_OUT_SIG,outData);

   Pkcs7SignedContents* sigBody = new Pkcs7SignedContents( outData );
   assert( sigBody );

   // add the signature to it
   sigBody->header(h_ContentType).param( p_name ) = "smime.p7s";
   sigBody->header(h_ContentDisposition).param( p_handling ) = "required";
   sigBody->header(h_ContentDisposition).param( p_filename ) = "smime.p7s";
   sigBody->header(h_ContentDisposition).value() =  "attachment" ;
   sigBody->header(h_ContentTransferEncoding).value() = "binary";
   multi->parts().push_back( sigBody );

   assert( multi->parts().size() == 2 );

   BIO_free(in);
   BIO_free(out);
   sk_X509_free(chain);

   return multi;
}


Pkcs7Contents*
BaseSecurity::encrypt(Contents* bodyIn, const Data& recipCertName )
{
   assert( bodyIn );

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
   int s = bodyData.size();

   BIO* in = BIO_new_mem_buf( (void*)p,s);
   assert(in);
   DebugLog( << "created in BIO");

   BIO* out = BIO_new(BIO_s_mem());
   assert(out);
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
   assert(cert);

   STACK_OF(X509) *certs = sk_X509_new_null();
   assert(certs);
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
   assert( cipher );

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

   BIO_flush(out);

   char* outBuf=0;
   long size = BIO_get_mem_data(out,&outBuf);
   assert( size > 0 );

   Data outData(outBuf,size);
   assert( (long)outData.size() == size );

   InfoLog( << "Encrypted body size is " << outData.size() );
   InfoLog( << "Encrypted body is <" << outData.escaped() << ">" );

   static char RESIP_ENCRYPT_OUT[] = "resip-encrypt-out";
   Security::dumpAsn(RESIP_ENCRYPT_OUT, outData);

   Pkcs7Contents* outBody = new Pkcs7Contents( outData );
   assert( outBody );

   outBody->header(h_ContentType).param( p_smimeType ) = "enveloped-data";
   outBody->header(h_ContentType).param( p_name ) = "smime.p7m";
   outBody->header(h_ContentDisposition).param( p_handling ) = "required";
   outBody->header(h_ContentDisposition).param( p_filename ) = "smime.p7";
   outBody->header(h_ContentDisposition).value() =  "attachment" ;
   outBody->header(h_ContentTransferEncoding).value() = "binary";

   BIO_free(in);
   BIO_free(out);
   sk_X509_free(certs);

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

   if (mDomainPrivateKeys.count(signerDomain) == 0)
   {
      InfoLog( << "No private key for " << signerDomain );
      throw Exception("Missing private key when computing identity",__FILE__,__LINE__);
   }

   EVP_PKEY* pKey = mDomainPrivateKeys[signerDomain];
   assert( pKey );
 
   if ( pKey->type !=  EVP_PKEY_RSA )
   {
      ErrLog( << "Private key (type=" << pKey->type <<"for " 
              << signerDomain << " is not of type RSA" );
      throw Exception("No RSA private key when computing identity",__FILE__,__LINE__);
   }

   assert( pKey->type ==  EVP_PKEY_RSA );
   RSA* rsa = EVP_PKEY_get1_RSA(pKey);

   unsigned char result[4096];
   int resultSize = sizeof(result);
   assert( resultSize >= RSA_size(rsa) );

   SHA1Stream sha;
   sha << in;
   Data hashRes =  sha.getBin();
   DebugLog( << "hash of string is 0x" << hashRes.hex() );

#if 1
   int r = RSA_sign(NID_sha1, (unsigned char *)hashRes.data(), hashRes.size(),
                    result, (unsigned int*)( &resultSize ),
            rsa);
   assert( r == 1 );
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
      if (mDomainCerts.count(signerDomain) == 0)
      {
         ErrLog( << "No public key for " << signerDomain );
         throw Exception("Missing public key when verifying identity",__FILE__,__LINE__);
      }
      cert = mDomainCerts[signerDomain];
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
   assert( pKey );

   assert( pKey->type ==  EVP_PKEY_RSA );
   RSA* rsa = EVP_PKEY_get1_RSA(pKey);

#if 1
   int ret = RSA_verify(NID_sha1, (unsigned char *)hashRes.data(),
                        hashRes.size(), (unsigned char*)sig.data(), sig.size(),
                        rsa);
#else
   unsigned char result[4096];
   int resultSize = sizeof(result);
   assert( resultSize >= RSA_size(rsa) );

   resultSize = RSA_public_decrypt(sig.size(),(unsigned char*)sig.data(),
                                   result, rsa, RSA_PKCS1_PADDING );
   assert( resultSize != -1 );
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
BaseSecurity::checkAndSetIdentity( const SipMessage& msg, const Data& certDer) const
{
   auto_ptr<SecurityAttributes> sec(new SecurityAttributes);
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
         if (d2i_X509(&cert,&in,certDer.size()) == 0)
         {
            DebugLog(<< "Could not read DER certificate from " << certDer );
            cert = NULL;
         }
      }
      if ( certDer.empty() || cert )
      {
         if ( checkIdentity(msg.header(h_From).uri().host(),
                            msg.getCanonicalIdentityString(),
                            msg.header(h_Identity).value(),
                            cert ) )
         {
            sec->setIdentity(msg.header(h_From).uri().getAor());
            sec->setIdentityStrength(SecurityAttributes::Identity);
         }
         else
         {
            sec->setIdentity(msg.header(h_From).uri().getAor());
            sec->setIdentityStrength(SecurityAttributes::FailedIdentity);
         }
      }
      else
      {
         sec->setIdentity(msg.header(h_From).uri().getAor());
         sec->setIdentityStrength(SecurityAttributes::FailedIdentity);
      }
   }
   catch (BaseException&)
   {
      sec->setIdentity(msg.header(h_From).uri().getAor());
      sec->setIdentityStrength(SecurityAttributes::FailedIdentity);
   }
   msg.setSecurityAttributes(sec);
}


Contents*
BaseSecurity::decrypt( const Data& decryptorAor, const Pkcs7Contents* contents)
{

   DebugLog( << "decryptor Aor: <" << decryptorAor << ">" );

   int flags=0;
   flags |= PKCS7_BINARY;

   // for now, assume that this is only a singed message
   assert( contents );

   Data text = contents->getBodyData();
   DebugLog( << "uncode body = <" << text.escaped() << ">" );
   DebugLog( << "uncode body size = " << text.size() );

   static char RESIP_ASN_DECRYPT[] = "resip-asn-decrypt";
   Security::dumpAsn(RESIP_ASN_DECRYPT, text );

   BIO* in = BIO_new_mem_buf( (void*)text.c_str(), text.size());
   assert(in);
   InfoLog( << "created in BIO");

   BIO* out;
   out = BIO_new(BIO_s_mem());
   assert(out);
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
   BIO_flush(in);

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
         InfoLog( << "Unkown pkcs7 type" );
         break;
   }

   STACK_OF(X509)* certs = sk_X509_new_null();
   assert( certs );

   //   flags |= PKCS7_NOVERIFY;

   assert( mRootTlsCerts );

   switch (type)
   {
      case NID_pkcs7_signedAndEnveloped:
      {
         BIO_free(in);
         BIO_free(out);
         sk_X509_free(certs);
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
            InfoLog( << "Don't have a private key for " << decryptorAor << " for  PKCS7_decrypt" );
            throw Exception("Missing private key", __FILE__, __LINE__);
         }
         else if (mUserCerts.count(decryptorAor) == 0)
         {
            BIO_free(in);
            BIO_free(out);
            sk_X509_free(certs);
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
            return 0;
         }
      }
      break;

      default:
         BIO_free(in);
         BIO_free(out);
         sk_X509_free(certs);
         ErrLog(<< "Got PKCS7 data that could not be handled type=" << type );
         throw Exception("Unsupported PKCS7 data type", __FILE__, __LINE__);
   }

   BIO_flush(out);   
   BUF_MEM* bufMem;
   BIO_get_mem_ptr(out, &bufMem);

   int len = bufMem->length;
   char* buffer = new char[len];
   memcpy(buffer, bufMem->data, len);

   BIO_set_close(out, BIO_CLOSE);
   BIO_free(in);
   BIO_free(out);
   sk_X509_free(certs);

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
   assert( it != multi->parts().end() );
   Contents* second = *it;

   assert( second );
   assert( first );

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

   BIO* in = BIO_new_mem_buf( (void*)sigData.data(),sigData.size());
   assert(in);
   InfoLog( << "created in BIO");

   BIO* out = BIO_new(BIO_s_mem());
   assert(out);
   InfoLog( << "created out BIO" );

   BIO* pkcs7Bio = BIO_new_mem_buf( (void*) textData.data(),textData.size());
   assert(pkcs7Bio);
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
   BIO_flush(in);

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
   assert( certs );

   if ( *signedBy == Data::Empty )
   {   
       //add all the certificates from mUserCerts stack to 'certs' stack  
       for(X509Map::iterator it = mUserCerts.begin(); it != mUserCerts.end(); it++)
       {
           assert(it->second);
           sk_X509_push(certs, it->second);
       }
   }
   else
   {
      if (mUserCerts.count( *signedBy ))
      {
         InfoLog( <<"Adding cert from " <<  *signedBy << " to check sig" );
         X509* cert = mUserCerts[ *signedBy ];
         assert(cert);
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
               catch (...)
               {
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

   assert( mRootTlsCerts );

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
         ErrLog(<< "Got PKCS7 data that could not be handled type=" << type );
         return 0;
   }

   BIO_flush(out);
   char* outBuf=0;
   long size = BIO_get_mem_data(out,&outBuf);
   assert( size >= 0 );

   Data outData(outBuf,size);
   DebugLog( << "uncoded body is <" << outData.escaped() << ">" );

   BIO_free(in);
   BIO_free(out);
   BIO_free(pkcs7Bio);
   sk_X509_free(certs);
   return first;
}


SSL_CTX*
BaseSecurity::getTlsCtx ()
{
   assert(mTlsCtx);
   return   mTlsCtx;
}


SSL_CTX*
BaseSecurity::getSslCtx ()
{
   assert(mSslCtx);
   return   mSslCtx;
}

Data 
BaseSecurity::getCertName(X509 *cert)
{
    X509_NAME *subj;
    int       extcount;

    assert(cert);

    if ((extcount = X509_get_ext_count(cert)) > 0)
    {
        for (int i = 0;  i < extcount;  i++)
        {
            char              *extstr;
            X509_EXTENSION    *ext;
 
            ext = X509_get_ext(cert, i);
            extstr = (char*) OBJ_nid2sn(OBJ_obj2nid(X509_EXTENSION_get_object(ext)));
 
            if (!strcmp(extstr, "subjectAltName"))
            {
                int                  j;

#if (OPENSSL_VERSION_NUMBER < 0x0090800fL )
                unsigned char *data;
#else
                unsigned const char  *data;
#endif

                STACK_OF(CONF_VALUE) *val;
                CONF_VALUE           *nval;
                X509V3_EXT_METHOD    *meth;
                void                 *ext_str = NULL;
 
                if (!(meth = X509V3_EXT_get(ext)))
                    break;
                data = ext->value->data;

#if (OPENSSL_VERSION_NUMBER > 0x00907000L)
                if (meth->it)
                  ext_str = ASN1_item_d2i(NULL, &data, ext->value->length,
                                          ASN1_ITEM_ptr(meth->it));
                else
                  ext_str = meth->d2i(NULL, &data, ext->value->length);
#else
                ext_str = meth->d2i(NULL, &data, ext->value->length);
#endif
                val = meth->i2v(meth, ext_str, NULL);
                for (j = 0;  j < sk_CONF_VALUE_num(val);  j++)
                {
                    nval = sk_CONF_VALUE_value(val, j);
                    if (!strcmp(nval->name, "DNS"))
                    {
                        //retrieve name, from nval->value                        
                        return Data(nval->value);
                    }
                }
            }
        }
    }
 
    char cname[256];
    memset(cname, 0, sizeof cname);

    if ((subj = X509_get_subject_name(cert)) &&
        X509_NAME_get_text_by_NID(subj, NID_commonName, cname, sizeof(cname)-1) > 0)
    {        
        return Data(cname);
    }

    ErrLog(<< "This certificate doesn't have neither subjectAltName nor commonName");
    return Data::Empty;
}
/**
   Matchtes subjectAltName and cnames 
   @todo    looks incomplete, make better
*/
static int 
matchHostName(char *certName, const char *domainName)
{
   const char *dot;
   dot = strchr(domainName, '.');
   if (dot == NULL)
   {
      char *pnt = strchr(certName, '.');
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
         domainName = dot + 1;
         certName += 2;
      }
   }
   return !strcasecmp(certName, domainName);
}

bool 
BaseSecurity::compareCertName(X509 *cert, const Data& domainName)
{
   assert(cert);

   Data certName = getCertName(cert);
   if(Data::Empty == certName)
   {
      InfoLog (<< "No cert name to match against " << domainName);
      return false;
   }

   DebugLog (<< "Matching " << certName << " cert against " << domainName);
   bool isMatching = matchHostName((char*)certName.c_str(), domainName.c_str()) ? true : false;

   return isMatching;
}

bool
BaseSecurity::isSelfSigned(X509 *cert)
{
   int iRet = X509_NAME_cmp(cert->cert_info->issuer, cert->cert_info->subject);
   return (iRet == 0);
}

void
BaseSecurity::dumpAsn( char* name, Data data)
{
#if 0 // for debugging
   assert(name);

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
