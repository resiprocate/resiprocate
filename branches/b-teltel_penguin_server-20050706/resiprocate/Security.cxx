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
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Timer.hxx"
#include "resiprocate/os/ParseBuffer.hxx"
#include "resiprocate/os/FileSystem.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"


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


static const Data rootCert("root_cert_");
static const Data domainCert("domain_cert_");
static const Data domainKey("domain_key_");
static const Data userCert("user_cert_");
static const Data userKey("user_key_");
static const Data pem(".pem");

static const Data pemTypePrefixes[] =
{
   rootCert,
   domainCert,
   domainKey,
   userCert,
   userKey
};


Data
readIntoData(const Data& filename)
{
   DebugLog( << "Trying to read file " << filename );
   
   ifstream is;
   is.open(filename.c_str(), ios::binary );
   assert(is.is_open()); // TODO should thorw not assert 
   
   // get length of file:
   is.seekg (0, ios::end);
   int length = is.tellg();
   is.seekg (0, ios::beg);
   
   char* buffer = new char [length];
   
   // read data as a block:
   is.read (buffer,length);
   
   Data target(Data::Take, buffer, length);
   
   is.close();
   
   return target;
}


Data
getAor(const Data& filename, const Data& prefix)
{
   return filename.substr(prefix.size(), filename.size() - prefix.size() - pem.size());
}


Security::Security(const Data& directory) : mPath(directory)
{
}


void
Security::preload()
{
#if 0
   FileSystem::Directory dir(mPath);
   char buffer[8192];
   Data fileT(Data::Borrow, buffer, sizeof(buffer));
   FileSystem::Directory::iterator it(dir);
   for (; it != dir.end(); ++it)
   {
      if (it->postfix(pem))
      {
         if (it->prefix(userCert))
         {
            addUserCertPEM(getAor(*it, userCert), readIntoData(*it));
         }
         else if (it->prefix(userKey))
         {
            addUserPrivateKeyPEM(getAor(*it, userKey), readIntoData(*it));
         }
         else if (it->prefix(domainCert))
         {
            addDomainPrivateKeyPEM(getAor(*it, domainCert), readIntoData(*it));
         }
         else if (it->prefix(domainKey))
         {
            addDomainPrivateKeyPEM(getAor(*it, domainKey), readIntoData(*it));
         }
         else if (it->prefix(rootCert))
         {
            addRootCertPEM(readIntoData(*it));
         }
      }
   }
#else
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

      if (name.postfix(pem))
      {
         InfoLog( << "Going to read file " << mPath + name );
         
         if (name.prefix(userCert))
         {
            addUserCertPEM(getAor(name, userCert), readIntoData(mPath + name));
         }
         else if (name.prefix(userKey))
         {
            addUserPrivateKeyPEM(getAor(name, userKey), readIntoData(mPath + name));
         }
         else if (name.prefix(domainCert))
         {
            addDomainCertPEM(getAor(name, domainCert), readIntoData(mPath + name));
         }
         else if (name.prefix(domainKey))
         {
            addDomainPrivateKeyPEM(getAor(name, domainKey), readIntoData(mPath + name));
         }
         else if (name.prefix(rootCert))
         {
            addRootCertPEM(readIntoData(mPath + name));
         }
      }
   }
   closedir( dir );
#endif
}


void
Security::onReadPEM(const Data& name, PEMType type, Data& buffer) const
{
   Data filename = mPath + pemTypePrefixes[type] + name + pem;

   // .dlb. extra copy
   buffer = readIntoData(filename);
}


void
Security::onWritePEM(const Data& name, PEMType type, const Data& buffer) const
{
   Data filename = mPath + pemTypePrefixes[type] + name + pem;

   ofstream str(filename.c_str(), ios::binary);
   str.write(buffer.data(), buffer.size());
}


void
Security::onRemovePEM(const Data& name, PEMType type) const
{
   assert(0);
   // TODO - should delete file 
}


namespace
{
FILE* 
fopenHelper (const char* pathname, const char* option)
{
   FILE* fp = fopen(pathname, option);

   if ( !fp )
   {
      Data msg;
      DataStream strm(msg);

      strm << "fopen(" << pathname << ", " << option << ")" << "failed";

      ErrLog(<< msg);
      throw BaseSecurity::Exception(msg, __FILE__,__LINE__);
   }

   return   fp;
}


void 
clearError ()
{
    while (ERR_get_error())
        ;
}


void 
onReadError (bool do_throw = false)
{
    Data msg;
    while (true)
    {
        const char* file;
        int line;

        unsigned long code = ERR_get_error_line(&file,&line);
        if ( code == 0 )
        {
            break;
        }

        msg.clear();
        DataStream strm(msg);
        char err_str[256];
        ERR_error_string_n(code, err_str, sizeof(err_str));
        strm << err_str << ", file=" << file 
             << ", line= " << line << ", error code=" << code;

        ErrLog(<< msg);
    }
    if (do_throw)
    {
       throw BaseSecurity::Exception(msg, __FILE__,__LINE__);
    }
}


void 
logReadError ()
{
   onReadError(false);
}


void 
throwReadError ()
{
   onReadError(true);
}


struct FileGuard
{
    FileGuard (FILE* fp)
    :   mFp(fp) {}
    ~FileGuard ()
    {
        fclose(mFp);
    }

    FILE* mFp;
};


#if defined(WIN32)

struct FindGuard
{
    FindGuard (HANDLE findHandle)
    :   mHandle(findHandle) {}
    ~FindGuard ()
    {
        FindClose(mHandle);
    }

    HANDLE  mHandle;
};

#else

struct DirGuard
{
    DirGuard (DIR* dir)
    :   mDir(dir) {}
    ~DirGuard ()
    {
        closedir(mDir);
    }

    DIR*  mDir;
};
#endif


void
setPassPhrase(BaseSecurity::PassPhraseMap& passPhrases, 
              const Data& key, 
              const Data& passPhrase)
{
   passPhrases.insert(std::make_pair(key, passPhrase));
}


bool
hasPassPhrase(const BaseSecurity::PassPhraseMap& passPhrases, const Data& key)
{
   BaseSecurity::PassPhraseMap::const_iterator iter = passPhrases.find(key);
   return (iter != passPhrases.end());
}


}  // namespace



void
BaseSecurity::addCertDER (PEMType type, 
                          const Data& key, 
                          const Data& certDER, 
                          bool write) const
{
   assert( !certDER.empty() );

   X509* cert;
   unsigned char* in = (unsigned char*)certDER.data();
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
   X509Map& certs = (type == DomainCert ? mDomainCerts : mUserCerts);

   certs.insert(std::make_pair(key, cert));
   
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

   assert(0); // the code following this has no hope of working 
   
   X509* x = where->second;
   int len = i2d_X509(x, NULL);

   // !kh!
   // Although len == 0 is not an error, I am not sure what quite to do.
   // Asserting for now.
   assert(len != 0);
   if(len < 0)
   {
      ErrLog(<< "Could encode certificate of '" << key << "' to DER form");
      throw BaseSecurity::Exception("Could encode certificate to DER form", __FILE__,__LINE__);
   }
   char*    out = new char[len];
  return   Data(Data::Take, out, len);
}


void 
BaseSecurity::addPrivateKeyPKEY(PEMType type, 
                                const Data& name, 
                                EVP_PKEY* pKey, 
                                bool write) const 
{ 
   PrivateKeyMap& privateKeys = (type == DomainPrivateKey ? 
                                 mDomainPrivateKeys : mUserPrivateKeys);

   privateKeys.insert(std::make_pair(name, pKey));
   
   // figure out a passPhrase to encrypt with 
   char* kstr=NULL;
   int klen=0;
   if (type != DomainPrivateKey)
   {
      PassPhraseMap::const_iterator iter = mUserPassPhrases.find(name);
      if(iter != mUserPassPhrases.end())
      {
         Data passPhrase = iter->second;
         
         kstr = (char*)passPhrase.c_str(); // TODO !cj! mem leak 
         klen = passPhrase.size();
      }
   }

   if (write)
   {
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
      BIO_set_close(in, BIO_NOCLOSE);

      EVP_PKEY* privateKey;
      if (d2i_PKCS8PrivateKey_bio(in, &privateKey, 0, passPhrase) == 0)
      {
         ErrLog(<< "Could not read private key from '" << privateKeyDER << "' using pass phrase '" << passPhrase << "'" );
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
      if (type != DomainPrivateKey)
      {
         PassPhraseMap::const_iterator iter = mUserPassPhrases.find(name);
         if(iter != mUserPassPhrases.end())
         {
            passPhrase = const_cast<char*>(iter->second.c_str());
         }
      }
      BIO_set_close(in, BIO_NOCLOSE);
      
      EVP_PKEY* privateKey;
      if (PEM_read_bio_PrivateKey(in, &privateKey, 0, passPhrase) == 0)
      {
         ErrLog(<< "Could not read private key from '" << endl 
                << privateKeyPEM << "using pass phrase '" << passPhrase <<endl );
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


BaseSecurity::BaseSecurity () :
   mTlsCtx(0),
   mSslCtx(0),
   mRootCerts(0)
{ 
   int ret;
   
   initialize(); 
   
   mRootCerts = X509_STORE_new();
   assert(mRootCerts);

   static char* cipher="RSA+SHA+AES+3DES";

   mTlsCtx = SSL_CTX_new( TLSv1_method() );
   assert(mTlsCtx);
   SSL_CTX_set_cert_store(mTlsCtx, mRootCerts);
   ret = SSL_CTX_set_cipher_list(mTlsCtx,cipher);
   assert(ret);
   
   mSslCtx = SSL_CTX_new( SSLv23_method() );
   assert(mSslCtx);
   SSL_CTX_set_cert_store(mSslCtx, mRootCerts);
   ret = SSL_CTX_set_cipher_list(mSslCtx,cipher);
   assert(ret);
}


BaseSecurity::~BaseSecurity ()
{
   {
      // cleanup certificates
      X509Map::iterator iter = mDomainCerts.begin();
      for (; iter != mDomainCerts.end(); ++iter)
      {
         X509_free(iter->second);
         mDomainCerts.erase(iter);
      }
      iter = mUserCerts.begin();
      for (; iter != mUserCerts.end(); ++iter)
      {
         X509_free(iter->second);
         mUserCerts.erase(iter);
      }
   }
   {
      // cleanup private keys
      PrivateKeyMap::iterator iter = mDomainPrivateKeys.begin();
      for (; iter != mDomainPrivateKeys.end(); ++iter)
      {
         EVP_PKEY_free(iter->second);
         mDomainPrivateKeys.erase(iter);
      }
      iter = mUserPrivateKeys.begin();
      for (; iter != mUserPrivateKeys.end(); ++iter)
      {
         EVP_PKEY_free(iter->second);
         mUserPrivateKeys.erase(iter);
      }
   }
   {
      // cleanup root certs
      X509_STORE_free(mRootCerts);
   }
#if 0 // TODO - mem leak but seg faults
   {
      // cleanup SSL_CTXes
      if (mTlsCtx)
      {
         SSL_CTX_free(mTlsCtx);mTlsCtx=0;
      }
      if (mSslCtx)
      {
         SSL_CTX_free(mSslCtx);mSslCtx=0;
      }
   }
#endif
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


std::vector<BaseSecurity::CertificateInfo>
BaseSecurity::getRootCertDescriptions() const
{
   // !kh!
   // need to be implemented.
   assert(0); // TODO 
   return   std::vector<CertificateInfo>();
}


void
BaseSecurity::addRootCertPEM(const Data& x509PEMEncodedRootCerts)
{
    return;
   assert(0);
#if 0
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
       mRootCerts = X509_STORE_new();

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
   
   ret = X509_NAME_add_entry_by_txt( subject, "O",  MBSTRING_UTF8, 
                                     (unsigned char *) domain.data(), domain.size(), 
                                     -1, 0);
   assert(ret);
   ret = X509_NAME_add_entry_by_txt( subject, "CN", MBSTRING_UTF8, 
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

   const char* p = bodyData.data();
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

   const char* p = bodyData.data();
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

   assert( pKey->type ==  EVP_PKEY_RSA );
   RSA* rsa = EVP_PKEY_get1_RSA(pKey);

   unsigned char result[4096];
   int resultSize = sizeof(result);
   assert( resultSize >= RSA_size(rsa) );

   assert(SHA_DIGEST_LENGTH == 20);
   unsigned char hashRes[SHA_DIGEST_LENGTH];
   unsigned int hashResLen=SHA_DIGEST_LENGTH;

   SHA_CTX sha;
   SHA1_Init( &sha );
   SHA1_Update(&sha, in.data() , in.size() );
   SHA1_Final( hashRes, &sha );

   DebugLog( << "hash of string is 0x" <<  Data(hashRes,sizeof(hashRes)).hex() );

#if 1
   int r = RSA_sign(NID_sha1, hashRes, hashResLen,
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
   Security::dumpAsn("identity-in-hash", Data(hashRes, hashResLen) );
   Security::dumpAsn("identity-in-rsa",res);
   Security::dumpAsn("identity-in-base64",enc);

   return enc;
}


bool
BaseSecurity::checkIdentity( const Data& signerDomain, const Data& in, const Data& sigBase64 ) const
{
   if (mDomainCerts.count(signerDomain) == 0)
   {
      ErrLog( << "No public key for " << signerDomain );
      throw Exception("Missing public key when verifying identity",__FILE__,__LINE__);
   }
   X509* cert = mDomainCerts[signerDomain];

   DebugLog( << "Check identity for " << in );
   DebugLog( << " base64 data is " << sigBase64 );

   Data sig = sigBase64.base64decode();
   DebugLog( << "decoded sig is 0x"<< sig.hex() );

   assert(SHA_DIGEST_LENGTH == 20);
   unsigned char hashRes[SHA_DIGEST_LENGTH];
   unsigned int hashResLen=SHA_DIGEST_LENGTH;

   SHA_CTX sha;
   SHA1_Init( &sha );
   SHA1_Update(&sha, in.data() , in.size() );
   SHA1_Final( hashRes, &sha );
   Data computedHash(hashRes, hashResLen);

   DebugLog( << "hash of string is 0x" <<  Data(hashRes,sizeof(hashRes)).hex() );

   EVP_PKEY* pKey = X509_get_pubkey( cert );
   assert( pKey );

   assert( pKey->type ==  EVP_PKEY_RSA );
   RSA* rsa = EVP_PKEY_get1_RSA(pKey);

#if 1
   int ret = RSA_verify(NID_sha1, hashRes, hashResLen,
                        (unsigned char*)sig.data(), sig.size(),
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

   dumpAsn("identity-out-msg", in );
   dumpAsn("identity-out-base64", sigBase64 );
   dumpAsn("identity-out-sig", sig );
   dumpAsn("identity-out-hash", computedHash );

   return ret;
}


void
BaseSecurity::checkAndSetIdentity( const SipMessage& msg ) const
{
   auto_ptr<SecurityAttributes> sec(new SecurityAttributes);

   try
   {
      if (checkIdentity(msg.header(h_From).uri().host(),
                        msg.getCanonicalIdentityString(),
                        msg.header(h_Identity).value()))
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

   assert( mRootCerts );

   switch (type)
   {
      case NID_pkcs7_signedAndEnveloped:
      {
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

            return 0;
         }
      }
      break;

      default:
         ErrLog(<< "Got PKCS7 data that could not be handled type=" << type );
         throw Exception("Unsupported PKCS7 data type", __FILE__, __LINE__);
   }

   BIO_flush(out);
   char* outBuf=0;
   long size = BIO_get_mem_data(out,&outBuf);
   assert( size >= 0 );

   Data outData(outBuf,size);
   DebugLog( << "uncoded body is <" << outData.escaped() << ">" );

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

   vector<Contents*>::const_iterator i = multi->parts().begin();
   Contents* first = *i;
   ++i;
   assert( i != multi->parts().end() );
   Contents* second = *i;
   Pkcs7SignedContents* sig = dynamic_cast<Pkcs7SignedContents*>( second );

   if ( !sig )
   {
      ErrLog( << "Don't know how to deal with signature type " );
      throw Exception("Invalid contents passed to checkSignature", __FILE__, __LINE__);
   }

   int flags=0;
   flags |= PKCS7_BINARY;

   assert( second );
   assert( first );

   //CerrLog( << "message to sign is " << *first );

   Data bodyData;
   DataStream strm( bodyData );
   first->encodeHeaders( strm );
   first->encode( strm );
   strm.flush();
   //CerrLog( << "encoded version to sign is " << bodyData );

   // Data textData = first->getBodyData();
   Data textData = bodyData;
   Data sigData = sig->getBodyData();

   Security::dumpAsn( "resip-asn-uncode-signed-text", textData );
   Security::dumpAsn( "resip-asn-uncode-signed-sig", sigData );

   BIO* in = BIO_new_mem_buf( (void*)sigData.data(),sigData.size());
   assert(in);
   InfoLog( << "ceated in BIO");

   BIO* out = BIO_new(BIO_s_mem());
   assert(out);
   InfoLog( << "created out BIO" );

   DebugLog( << "verify <"    << textData.escaped() << ">" );
   DebugLog( << "signature <" << sigData.escaped() << ">" );

   BIO* pkcs7Bio = BIO_new_mem_buf( (void*) textData.data(),textData.size());
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
      assert(0);
      // need to add back in code that adds all certs when sender unkonwn
   }
   else
   {
      if (mUserCerts.count( *signedBy))
      {
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
               catch (ParseBuffer::Exception& e)
               {
               }
            }
         }

         sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);
      }
   }
   else
   {
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

   assert( mRootCerts );

   switch (type)
   {
      case NID_pkcs7_signed:
      {
         if ( PKCS7_verify(pkcs7, certs, mRootCerts, pkcs7Bio, out, flags ) != 1 )
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
         ErrLog(<< "Got PKCS7 data that could not be handled type=" << type );
         return 0;
   }

   BIO_flush(out);
   char* outBuf=0;
   long size = BIO_get_mem_data(out,&outBuf);
   assert( size >= 0 );

   Data outData(outBuf,size);
   DebugLog( << "uncoded body is <" << outData.escaped() << ">" );

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


void
BaseSecurity::dumpAsn( char* name, Data data)
{
#if 0 // !CJ! TODO turn off
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
