#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include <ostream>
#include <fstream>

#include "resiprocate/Contents.hxx"
#include "resiprocate/MultipartSignedContents.hxx"
#include "resiprocate/Pkcs7Contents.hxx"
#include "resiprocate/PlainContents.hxx"
#include "resiprocate/Security.hxx"
#include "resiprocate/SecurityAttributes.hxx"
#include "resiprocate/Transport.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Random.hxx"
#include "resiprocate/os/SHA1Stream.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Timer.hxx"
#include "resiprocate/os/ParseBuffer.hxx"
#include "resiprocate/os/FileSystem.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"


#if defined(USE_SSL)

#if !defined(_WIN32)
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

static const char* PEM = ".pem";

static const Data& 
pemTypePrefixes(  Security::PEMType pType )
{
   static const Data rootCert("root_cert_");
   static const Data domainCert("domain_cert_");
   static const Data domainKey("domain_key_");
   static const Data userCert("user_cert_");
   static const Data userKey("user_key_");
   static const Data unkonwKey("user_key_");

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
   return unkonwKey;
}


static Data
readIntoData(const Data& filename)
{
   DebugLog( << "Trying to read file " << filename );

#if !defined(_WIN32) 
   ifstream is;
   is.open(filename.c_str(), ios::binary );

   if ( !is.is_open() )
   {
      ErrLog( << "Could not open file " << filename << " for read");
      throw BaseSecurity::Exception("Could not read file ", 
                                    __FILE__,__LINE__);
   }
   
   assert(is.is_open());
   
   // get length of file:
   is.seekg (0, ios::end);
   int length = is.tellg();
   is.seekg (0, ios::beg);
   
   char* buffer = new char [length + 1];
   
   // read data as a block:
   is.read (buffer,length);
   buffer[length] = '\0';
   Data target(Data::Take, buffer, length);
   
   is.close();
#else
   OFSTRUCT of;
   memset((void*)&of, 0, sizeof(OFSTRUCT));
   of.cBytes = sizeof(OFSTRUCT);

   HFILE hFile = OpenFile(filename.c_str(), &of, OF_READ); 
   if (hFile == HFILE_ERROR)
   {
      ErrLog( << "Could not open file " << filename << " for read");
      throw BaseSecurity::Exception("Could not read file ", 
         __FILE__,__LINE__);
   }
   DWORD dwSize = GetFileSize((HANDLE)hFile, NULL);

   char* buffer = new char [dwSize + 1];
   DWORD nBytesRead;
   bool bReturn = ReadFile((HANDLE)hFile, buffer, dwSize, &nBytesRead, NULL);
   if (!bReturn)
   {
      ErrLog( << "Could not read file " << filename << " for read");
      delete []buffer;
      throw BaseSecurity::Exception("Could not read file ", 
         __FILE__,__LINE__);
   }
   
   buffer[dwSize] = '\0';
   Data target(Data::Take, buffer, dwSize);

   CloseHandle((HANDLE)hFile);
#endif

   return target;
}


static Data
getAor(const Data& filename, const  Security::PEMType &pemType )
{
   const Data& prefix = pemTypePrefixes( pemType );
   return filename.substr(prefix.size(), filename.size() - prefix.size() - ::strlen(PEM));
}

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

Security::Security(const Data& directory, bool serverAuthentication)
   : mPath(directory),
     mServerAuthentication(serverAuthentication)
{
   // since the preloader won't work otherwise and VERY difficult to figure
   // out. 
   if ( mPath[mPath.size()-1] != Symbols::SLASH[0] )
   {
      mPath += Symbols::SLASH;
   }
}


void
Security::preload()
{
   FileSystem::Directory dir(mPath);
   FileSystem::Directory::iterator it(dir);
   for (; it != dir.end(); ++it)
   {
      Data name = *it;
      Data pemData(Data::Share, PEM, ::strlen(PEM));
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
            else
            {
               InfoLog(<<"No matching PEM type: " << fileName );
               continue;
            }
         }
         catch (...)
         {  
            ErrLog(<< "Some problem reading " << fileName );
         }
         
         InfoLog(<<"Sucessfully loaded " << fileName );
      }
   }
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
Security::onWritePEM(const Data& name, PEMType type, const char* buffer) const
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
      size_t size = 0;
      if (buffer)
      {
         size = ::strlen(buffer);
         if (size)
         {
            str.write(buffer, size);
         }
      }
      if (!str)
      {
         ErrLog (<< "Failed writing to " << filename << " " << size << " bytes");
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
   unsigned char* in = (unsigned char*)certDER.c_str();
   if (d2i_X509(&cert,(const unsigned char**)&in,certDER.size()) == 0)
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
   try
   {
      BIO_set_close(in, BIO_NOCLOSE);
      cert = PEM_read_bio_X509(in,0,0,0);
   }
   catch(...)
   {
      BIO_free(in); 
      throw;
   }
   
   addCertX509(type,name,cert,write);
   
   BIO_free(in);
}


void
BaseSecurity::addCertX509(PEMType type, const Data& key, X509* cert, bool write) const
{
   assert(cert);
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
         //Data  buf(Data::Borrow, p, len);
         //this->onWritePEM(key, type, buf);
         
         this->onWritePEM(key, type, p);
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
   return   Data(Data::Take, (char*)buffer, len);
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
            //Data passPhrase = iter->second;
            
            //kstr = (char*)passPhrase.c_str(); // TODO !cj! mem leak 
            //klen = passPhrase.size();
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
         //Data  pem(Data::Borrow, p, len);
         //onWritePEM(name, type, pem );
         onWritePEM(name, type, p);
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
      BIO_set_close(in, BIO_NOCLOSE);

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
      if (passPhrase) 
      {
         free( passPhrase ); passPhrase=NULL;
      }
      BIO_free(in);
      throw;
   }
   
   if (passPhrase) 
   {
      free( passPhrase ); passPhrase=NULL;
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
      BIO_set_close(in, BIO_NOCLOSE);
      
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
      if (passPhrase) 
      {
         free( passPhrase ); passPhrase=NULL;
      }
      BIO_free(in);
      throw;
   }

   if (passPhrase) 
   {
      free( passPhrase ); passPhrase=NULL;
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


void
BaseSecurity::getPrivateKeyPEM( PEMType type,
                                const Data& key,
                                char*& pkPEM) const
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
   //char* buf = 0;
   //int len = BIO_get_mem_data(out, &buf);
   //Data retVal(Data::Borrow, buf, len);
   
   int len = BIO_get_mem_data(out, &pkPEM);

   BIO_free(out);
   // !nash! is buf need to be freed?
   //return retVal;
}


void
BaseSecurity::getPrivateKeyDER( PEMType type,
                                const Data& key,
                                char*& pkDER) const
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
   //char* buf = 0;
   //int len = BIO_get_mem_data(out, &buf);
   //Data retVal(Data::Borrow, buf, len);
   int len = BIO_get_mem_data(out, &pkDER);

   BIO_free(out);
   
   //return retVal;
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


Security::Exception::Exception(const char* msg, const char* file, const int line)
:  BaseException(msg,file,line)
{
}


BaseSecurity::BaseSecurity () :
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

   // static char* cipher="RSA+SHA+AES+3DES";
   // static char* cipher="TLS_RSA_WITH_AES_128_CBC_SHA:TLS_RSA_WITH_3DES_EDE_CBC_SHA";
   //static char* cipher="ALL";
   //static char* cipher="RSA+DSS+AES+3DES+DES+RC4+SHA1+MD5";
   static char* cipher="TLSv1";
     
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
   ret = SSL_CTX_set_cipher_list(mTlsCtx, cipher);
   assert(ret);

   mSslCtx = SSL_CTX_new( SSLv23_method() );
   assert(mSslCtx);
   SSL_CTX_set_cert_store(mSslCtx, mRootSslCerts);
   SSL_CTX_set_verify(mSslCtx, SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE, verifyCallback);
   ret = SSL_CTX_set_cipher_list(mSslCtx, cipher);
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
   // cleanup certificates
   clearMap(mDomainCerts, X509_free);
   clearMap(mUserCerts, X509_free);

   // cleanup private keys
   clearMap(mDomainPrivateKeys, EVP_PKEY_free);
   clearMap(mUserPrivateKeys, EVP_PKEY_free);

   // cleanup root certs
   //X509_STORE_free(mRootCerts);

   // cleanup SSL_CTXes
   if (mTlsCtx)
   {
      SSL_CTX_free(mTlsCtx);
      mTlsCtx = 0;
   }
   if (mSslCtx)
   {
      SSL_CTX_free(mSslCtx);
      mSslCtx = 0;
   }
}


void
BaseSecurity::initialize ()
{
   // TODO !cj! - this should only be called once - ues pthread_once
   // It is not a huge bug if this runs twice because the calls cause no harm 
   static bool done=false;
   if (!done)
   {
      DebugLog( << "Setting up SSL library" );
      
      SSL_library_init();
      SSL_load_error_strings();
      OpenSSL_add_all_algorithms();
      
      Random::initialize();
      Timer::getTimeMs(); // initalize time offsets
      
      // make sure that necessary algorithms exist:
      assert(EVP_des_ede3_cbc());
      
      done = true;
   }
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
   	NULL,		      /* new */
   	NULL,		      /* free */
   	NULL, 		   /* init */
   	NULL,		      /* shutdown */
   	pemstring_ctrl,/* ctrl */
   	NULL,		      /* get_by_subject */
   	NULL,		      /* get_by_issuer_serial */
   	NULL,		      /* get_by_fingerprint */
   	NULL,		      /* get_by_alias */
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


void
BaseSecurity::getDomainPrivateKeyPEM(const Data& domainName, char*& pkPEM) const
{
   getPrivateKeyPEM(DomainPrivateKey, domainName, pkPEM);
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
   assert(aor.empty());

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


void
BaseSecurity::getUserPrivateKeyPEM(const Data& aor, char*& pkPEM) const
{
   pkPEM = NULL;
   getPrivateKeyPEM(UserPrivateKey, aor, pkPEM);
}


void
BaseSecurity::getUserPrivateKeyDER(const Data& aor, char*& pkDER) const
{
   pkDER = NULL;
   getPrivateKeyDER(UserPrivateKey, aor, pkDER);
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
   
   ret = X509_NAME_add_entry_by_txt( subject, "O",  MBSTRING_UTF8, 
                                     (unsigned char *) domain.c_str(), domain.size(), 
                                     -1, 0);
   assert(ret);
   ret = X509_NAME_add_entry_by_txt( subject, "CN", MBSTRING_UTF8, 
                                     (unsigned char *) aor.c_str(), aor.size(), 
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
   
   ext = X509V3_EXT_conf_nid(NULL, NULL, NID_basic_constraints, "CA:FALSE");
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
   DebugLog( << "Doing multipartSign" );
   assert( contents );

   // form the multipart
   MultipartSignedContents* multi = new MultipartSignedContents;
   multi->header(h_ContentType).param( p_micalg ) = "sha1";
   multi->header(h_ContentType).param( p_protocol ) = "application/pkcs7-signature";

   // add the main body to it
   Contents* body =  contents->clone();

#if 0
   // this need to be set in body before it is passed in
   body->header(h_ContentTransferEncoding).value() = StringCategory(Data("binary"));
#endif
   multi->parts().push_back( body );

   // compute the signature
   int flags = 0;
   flags |= PKCS7_BINARY;
   flags |= PKCS7_DETACHED;

#if 0 // TODO !cj! - decide what to do with these 
   flags |= PKCS7_NOCERTS; 
   flags |= PKCS7_NOATTR;
   flags |= PKCS7_NOSMIMECAP;
#endif

   Data bodyData;
   DataStream strm( bodyData );
   body->encodeHeaders( strm );
   body->encode( strm );
   strm.flush();

   DebugLog( << "sign the data <" << bodyData << ">" );
   Security::dumpAsn("resip-sign-out-data",bodyData);

   const char* p = bodyData.c_str();
   int s = bodyData.size();
   BIO* in=BIO_new_mem_buf( (void*)p,s);
   assert(in);
   DebugLog( << "ceated in BIO");

   BIO* out = BIO_new(BIO_s_mem()); // TODO - mem leak 
   assert(out);
   DebugLog( << "created out BIO" );

   STACK_OF(X509)* chain = sk_X509_new_null();
   assert(chain);

   DebugLog( << "checking" );
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

   int i = X509_check_private_key(publicCert, privateKey);
   DebugLog( << "checked cert and key ret=" << i  );

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
   Security::dumpAsn("resip-sign-out-sig",outData);

   Pkcs7Contents* sigBody = new Pkcs7Contents( outData );
   assert( sigBody );

   //sigBody->header(h_ContentType).type() = "application";
   //sigBody->header(h_ContentType).subType() = "pkcs7-signature";
   //sigBody->header(h_ContentType).param( "smime-type" ) = "signed-data";
   sigBody->header(h_ContentType).param( p_name ) = "smime.p7s";
   sigBody->header(h_ContentDisposition).param( p_handling ) = "required";
   sigBody->header(h_ContentDisposition).param( p_filename ) = "smime.p7s";
   sigBody->header(h_ContentDisposition).value() =  "attachment" ;
   sigBody->header(h_ContentTransferEncoding).value() = "binary";

   // add the signature to it
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
#if 0 // TODO !cj!
   // will cause it not to send certs in the signature
   flags |= PKCS7_NOCERTS;
#endif

   Data bodyData;
   DataStream strm(bodyData);
   bodyIn->encodeHeaders(strm);
   bodyIn->encode( strm );
   strm.flush();

   InfoLog( << "body data to encrypt is <" << bodyData.escaped() << ">" );

   const char* p = bodyData.c_str();
   int s = bodyData.size();

   BIO* in = BIO_new_mem_buf( (void*)p,s);
   assert(in);
   DebugLog( << "ceated in BIO");

   BIO* out = BIO_new(BIO_s_mem());
   assert(out);
   DebugLog( << "created out BIO" );

   InfoLog( << "target cert name is " << recipCertName );
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
   const EVP_CIPHER* cipher =  EVP_des_ede3_cbc();
#else
   EVP_CIPHER* cipher =  EVP_des_ede3_cbc();
#endif
   //const EVP_CIPHER* cipher = EVP_aes_128_cbc();
   //const EVP_CIPHER* cipher = EVP_enc_null();
   assert( cipher );

   PKCS7* pkcs7 = PKCS7_encrypt( certs, in, cipher, flags);
   if ( !pkcs7 )
   {
      BIO_free(in);
      BIO_free(out);
      sk_X509_free(certs);
      ErrLog( << "Error creating PKCS7 encrypt object" );
      throw Exception("Can't encrypt",__FILE__,__LINE__);
   }
   DebugLog( << "created PKCS7 encrypt object " );

   i2d_PKCS7_bio(out,pkcs7);

   BIO_flush(out);

   char* outBuf=0;
   long size = BIO_get_mem_data(out,&outBuf);
   assert( size > 0 );

   Data outData(outBuf,size);
   assert( (long)outData.size() == size );

   InfoLog( << Data("Encrypted body size is ") << outData.size() );
   InfoLog( << Data("Encrypted body is <") << outData.escaped() << ">" );

   Security::dumpAsn("resip-encrpt-out",outData);

   Pkcs7Contents* outBody = new Pkcs7Contents( outData );
   assert( outBody );

   //outBody->header(h_ContentType).type() = "application";
   //outBody->header(h_ContentType).subType() = "pkcs7-mime";
   outBody->header(h_ContentType).param( p_smimeType ) = "enveloped-data";
   outBody->header(h_ContentType).param( p_name ) = "smime.p7m";
   outBody->header(h_ContentDisposition).param( p_handling ) = "required";
   outBody->header(h_ContentDisposition).param( p_filename ) = "smime.p7";
   outBody->header(h_ContentDisposition).value() =  "attachment" ;

   BIO_free(in);
   BIO_free(out);
   sk_X509_free(certs);

   return outBody;
}


Pkcs7Contents*
BaseSecurity::signAndEncrypt( const Data& senderAor, Contents* body, const Data& recipCertName )
{
   assert(0);
   return 0;
   //return sign(senderAor, encrypt(body, recipCertName));
}


Data
BaseSecurity::computeIdentity( const Data& signerDomain, const Data& in ) const
{
   DebugLog( << "Compute identity for " << in );

   if (mDomainPrivateKeys.count(signerDomain) == 0)
   {
      ErrLog( << "No private key for " << signerDomain );
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
   int r = RSA_sign(NID_sha1, (unsigned char *)hashRes.c_str(), hashRes.size(),
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

   Security::dumpAsn("identity-in", in );
   Security::dumpAsn("identity-in-hash", hashRes );
   Security::dumpAsn("identity-in-rsa",res);
   Security::dumpAsn("identity-in-base64",enc);

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
   int ret = RSA_verify(NID_sha1, (unsigned char *)hashRes.c_str(),
                        hashRes.size(), (unsigned char*)sig.c_str(), sig.size(),
                        rsa);
#else
   unsigned char result[4096];
   int resultSize = sizeof(result);
   assert( resultSize >= RSA_size(rsa) );

   resultSize = RSA_public_decrypt(sig.size(),(unsigned char*)sig.c_str(),
                                   result, rsa, RSA_PKCS1_PADDING );
   assert( resultSize != -1 );
   //assert( resultSize == SHA_DIGEST_LENGTH );
   Data recievedHash(result,resultSize);
   dumpAsn("identity-out-decrypt", recievedHash );

   bool ret =  ( computedHash == recievedHash );
#endif

   DebugLog( << "rsa verify result is " << ret  );

   dumpAsn("identity-out-msg", in );
   dumpAsn("identity-out-base64", sigBase64 );
   dumpAsn("identity-out-sig", sig );
   dumpAsn("identity-out-hash", hashRes );

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
         unsigned char* in = (unsigned char*)certDer.c_str();
         if (d2i_X509(&cert,(const unsigned char**)&in,certDer.size()) == 0)
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
BaseSecurity::decrypt( const Data& decryptorAor, Pkcs7Contents* contents)
{
   int flags=0;
   flags |= PKCS7_BINARY;

   // for now, assume that this is only a singed message
   assert( contents );

   Data text = contents->getBodyData();
   DebugLog( << "uncode body = <" << text.escaped() << ">" );
   DebugLog( << "uncode body size = " << text.size() );

   Security::dumpAsn("resip-asn-decrypt", text );

   BIO* in = BIO_new_mem_buf( (void*)text.c_str(),text.size());
   assert(in);
   InfoLog( << "ceated in BIO");

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
   char* outBuf=0;
   long size = BIO_get_mem_data(out,&outBuf);
   assert( size >= 0 );

   Data outData(outBuf,size);
   DebugLog( << "uncoded body is <" << outData.escaped() << ">" );

   BIO_set_close(out, BIO_CLOSE);
   BIO_free(out);
   BIO_free(in);
   sk_X509_free(certs);

   // parse out the header information and form new body.
   // TODO !jf! this is a really crappy parser - shoudl do proper mime stuff
   ParseBuffer pb( outData.data(), outData.size() );

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
   // pre-parse headers
   ParseBuffer headersPb(headerStart, bodyStart-4-headerStart);
   ret->preParseHeaders(headersPb);

   DebugLog( << "Got body data of " << ret->getBodyData() );

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

   MultipartSignedContents::Parts::const_iterator i = multi->parts().begin();

   Contents* first = *i;
   ++i;
   assert( i != multi->parts().end() );
   Contents* second = *i;

#if 1
   Pkcs7SignedContents* sig = dynamic_cast<Pkcs7SignedContents*>( second );

   if ( !sig )
   {
      ErrLog( << "Don't know how to deal with signature type " );
      //throw Exception("Invalid contents passed to checkSignature", __FILE__,
      //__LINE__);
      return first;
   }
#endif

   int flags=0;
   flags |= PKCS7_BINARY;

   assert( second );
   assert( first );

   InfoLog( << "message to sign is " << *first );

   Data bodyData;
   DataStream strm( bodyData );
   first->encodeHeaders( strm );
   first->encode( strm );
   strm.flush();
   InfoLog( << "encoded version to sign is " << bodyData );

   // Data textData = first->getBodyData();
   Data textData = bodyData;
   Data sigData = sig->getBodyData();

   Security::dumpAsn( "resip-asn-uncode-signed-text", textData );
   Security::dumpAsn( "resip-asn-uncode-signed-sig", sigData );

   BIO* in = BIO_new_mem_buf( (void*)sigData.c_str(),sigData.size());
   assert(in);
   InfoLog( << "ceated in BIO");

   BIO* out = BIO_new(BIO_s_mem());
   assert(out);
   InfoLog( << "created out BIO" );

   InfoLog( << "verify <"    << textData.escaped() << ">" );
   InfoLog( << "signature <" << sigData.escaped() << ">" );

   BIO* pkcs7Bio = BIO_new_mem_buf( (void*) textData.c_str(),textData.size());
   assert(pkcs7Bio);
   InfoLog( << "ceated pkcs BIO");

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
         InfoLog( << "Unkown pkcs7 type" );
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
         InfoLog( <<"Adding cert from" <<  *signedBy << " to check sig" );
         X509* cert = mUserCerts[ *signedBy ];
         assert(cert);
         sk_X509_push(certs, cert);
      }
   }

   //flags |= PKCS7_NOINTERN;
   //flags |= PKCS7_NOVERIFY;
   //flags |= PKCS7_NOSIGS;

   STACK_OF(X509)* signers = PKCS7_get0_signers(pkcs7,certs, flags );
   if ( signers )
   {
      for (int i=0; i<sk_X509_num(signers); i++)
      {
         X509* x = sk_X509_value(signers,i);
         InfoLog(<< "Got a signer <" << i << ">" );

         GENERAL_NAMES* gens=0;
         gens = (GENERAL_NAMES*)X509_get_ext_d2i(x, NID_subject_alt_name, NULL, NULL);

         for(i = 0; i < sk_GENERAL_NAME_num(gens); i++)
         {
            GENERAL_NAME* gen = sk_GENERAL_NAME_value(gens, i);
            if(gen->type == GEN_URI)
            {
               ASN1_IA5STRING* uri = gen->d.uniformResourceIdentifier;
               int l = uri->length;
               unsigned char* dat = uri->data;
               Data name(dat,l);
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
         InfoLog(<<"Signed with serial " << hex << longSerial );
      }
   }
#endif

   assert( mRootTlsCerts );

   switch (type)
   {
      case NID_pkcs7_signed:
      {
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
            if ( flags & PKCS7_NOVERIFY )
            {
               DebugLog( << "Signature is notTrusted" );
               *sigStat = SignatureNotTrusted;
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
BaseSecurity::getCetName(X509 *cert)
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
                unsigned char        *data;
                STACK_OF(CONF_VALUE) *val;
                CONF_VALUE           *nval;
                X509V3_EXT_METHOD    *meth;
                void                 *ext_str = NULL;
 
                if (!(meth = X509V3_EXT_get(ext)))
                    break;
                data = ext->value->data;

#if (OPENSSL_VERSION_NUMBER > 0x00907000L)
                if (meth->it)
                  ext_str = ASN1_item_d2i(NULL, (const unsigned char**)&data, ext->value->length,
                                          ASN1_ITEM_ptr(meth->it));
                else
                  ext_str = meth->d2i(NULL, (const unsigned char**)&data, ext->value->length);
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

   Data certName = getCetName(cert);
   if(Data::Empty == certName)
      return false;

   bool isMatching = matchHostName((char*)certName.c_str(), domainName.c_str()) ? true : false;

   return isMatching;
}

void
BaseSecurity::dumpAsn( char* name, Data data)
{
#if 1 // !CJ! TODO turn off
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
         strm.write( data.c_str() , data.size() );
      }
      strm.flush();
   }
#endif
}

X509*     
BaseSecurity::getDomainCert( const Data& domain )
{
   return mDomainCerts[domain];
}

EVP_PKEY* 
BaseSecurity::getDomainKey(  const Data& domain )
{
   return mDomainPrivateKeys[domain];
   
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
