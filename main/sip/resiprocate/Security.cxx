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
  ifstream is;
  is.open(filename.c_str(), ios::binary );
  assert(is.is_open());

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
         if (name.prefix(userCert))
         {
            addUserCertPEM(getAor(name, userCert), readIntoData(name));
         }
         else if (name.prefix(userKey))
         {
            addUserPrivateKeyPEM(getAor(name, userKey), readIntoData(name));
         }
         else if (name.prefix(domainCert))
         {
            addDomainPrivateKeyPEM(getAor(name, domainCert), readIntoData(name));
         }
         else if (name.prefix(domainKey))
         {
            addDomainPrivateKeyPEM(getAor(name, domainKey), readIntoData(name));
         }
         else if (name.prefix(rootCert))
         {
            addRootCertPEM(readIntoData(name));
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
}



namespace
{

FILE* fopenHelper (const char* pathname, const char* option)
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

void clearError ()
{
    while (ERR_get_error())
        ;
}
void onReadError (bool do_throw = false)
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
        strm << err_str << ", file=" << file << ", line= " << line << ", error code=" << code;

        ErrLog(<< msg);
    }
    if(do_throw)
        throw BaseSecurity::Exception(msg, __FILE__,__LINE__);
}
void logReadError ()
{
   onReadError(false);
}
void throwReadError ()
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

// ------------------------------------------------------------
// OpenSSL certificate loading from PEM strings
//

// derived from openssl/crypto/x509/by_file.c : X509_load_cert_crl_file
//
int
pemstring_cert_crl(X509_LOOKUP *lu, const char *certPem)
{
	BIO* in = BIO_new_mem_buf(const_cast<char*>(certPem), -1);

	if(!in) {
		X509err(X509_F_X509_LOAD_CERT_CRL_FILE,ERR_R_SYS_LIB);
		return 0;
	}
	BIO_set_close(in, BIO_NOCLOSE);
	STACK_OF(X509_INFO)* inf = PEM_X509_INFO_read_bio(in, NULL, NULL, NULL);
	BIO_free(in);
	if(!inf) {
		X509err(X509_F_X509_LOAD_CERT_CRL_FILE,ERR_R_PEM_LIB);
		return 0;
	}

	int count = 0;
	for(int i = 0; i < sk_X509_INFO_num(inf); i++) {
		X509_INFO* itmp = sk_X509_INFO_value(inf, i);
		if(itmp->x509) {
			X509_STORE_add_cert(lu->store_ctx, itmp->x509);
			count++;
		} else if(itmp->crl) {
			X509_STORE_add_crl(lu->store_ctx, itmp->crl);
			count++;
		}
	}
	sk_X509_INFO_pop_free(inf, X509_INFO_free);
	return count;
}

int
pemstring_ctrl(X509_LOOKUP *lu, int cmd, const char *argp, long argl, char **ret)
{
    int ok = 0;

    switch (cmd)
    {
	case X509_L_FILE_LOAD:
	    ok = (pemstring_cert_crl(lu,argp) != 0);
	    break;
    }
    return ok;
}
// ----------- END of OpenSSL support section ---------------

void configSslCtx (SSL_CTX* ctx, X509_STORE* certStore)
{
   // associate root certs to mTlsCtx
   int ret;
   SSL_CTX_set_cert_store(ctx, certStore);
   //assert(ret);

   // set up the cipher
   static char* cipher="RSA+SHA+AES+3DES";
   ret = SSL_CTX_set_cipher_list(ctx,cipher);
   assert(ret);
}


void
setPassPhrase(BaseSecurity::PassPhraseMap& passPhrases, const Data& key, const Data& passPhrase)
{
   BaseSecurity::PassPhraseMap::iterator iter = passPhrases.find(key);
   if (iter == passPhrases.end())
      passPhrases.insert(std::make_pair(key, passPhrase));
}
bool
hasPassPhrase(const BaseSecurity::PassPhraseMap& passPhrases, const Data& key)
{
   BaseSecurity::PassPhraseMap::const_iterator iter = passPhrases.find(key);
   if (iter == passPhrases.end())
      return   false;
   else
      return   true;
}


}  // namespace



void
BaseSecurity::addCertDER (PEMType type, const Data& key, const Data& certDER, bool write)
{
   assert( !certDER.empty() );
   X509Map& certs = (type == DomainCert ? mDomainCerts : mUserCerts);

   X509Map::iterator where = certs.find(key);
   if (where == certs.end())
   {
      X509* cert;
      unsigned char* in = reinterpret_cast<unsigned char*>(const_cast<char*>(certDER.data()));
      if (d2i_X509(&cert,&in,certDER.size()) == 0)
      {
         ErrLog(<< "Could not read DER certificate from " << certDER );
         throw BaseSecurity::Exception("Could not read DER certificate ", __FILE__,__LINE__);
      }
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
            Data  buf(Data::Take, p, len);

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
}
void
BaseSecurity::addCertPEM (PEMType type, const Data& key, const Data& certPEM, bool write)
{
   assert( !certPEM.empty() );
   X509Map& certs = (type == DomainCert ? mDomainCerts : mUserCerts);

   X509Map::iterator where = certs.find(key);
   if (where == certs.end())
   {
      if (write)
         this->onWritePEM(key, type, certPEM);

      BIO* in = BIO_new_mem_buf(const_cast<char*>(certPEM.c_str()), -1);

      if ( !in )
     {
         ErrLog(<< "Could not create BIO buffer from '" << certPEM << "'");
         throw Exception("Could not create BIO buffer", __FILE__,__LINE__);
      }

      try
      {
         BIO_set_close(in, BIO_NOCLOSE);

         X509* cert = PEM_read_bio_X509(in,0,0,0);
         certs.insert(std::make_pair(key, cert));
      }
      catch(...)
      {
         BIO_free(in);
         throw;
      }
      BIO_free(in);
   }
}
bool
BaseSecurity::hasCert (PEMType type, const Data& key, bool read) const
{
   assert( !key.empty() );
   X509Map& certs = (type == DomainCert ? mDomainCerts : mUserCerts);

   X509Map::iterator where = certs.find(key);
   if (where == certs.end())
   {
      if (read)
      {
         Data certPEM;
         try
         {
            onReadPEM(key, type, certPEM);
            BaseSecurity*  mutable_this = const_cast<BaseSecurity*>(this);
            mutable_this->addCertPEM(type, key, certPEM, false);
            return   true;
         }
         catch (...)
         {
            return   false;
         }
      }
      else
      {
         return   false;
      }
   }
   return   true;
}
bool
BaseSecurity::removeCert (PEMType type, const Data& key, bool remove)
{
   assert( !key.empty() );
   X509Map& certs = (type == DomainCert ? mDomainCerts : mUserCerts);

   X509Map::iterator iter = certs.find(key);
   if (iter != certs.end())
   {
      X509_free(iter->second);
      certs.erase(iter);

      if (remove)
         onRemovePEM(key, type);

      return   true;
   }
   return   false;
}
Data
BaseSecurity::getCertDER (PEMType type, const Data& key, bool read) const
{
   assert( !key.empty() );

   if (hasCert(type, key, read) == false)
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
BaseSecurity::addPrivateKeyDER(
   PEMType type,
   const Data& key,
   const Data& privateKeyDER,
   bool write)
{
   assert( !key.empty() );
   assert( !privateKeyDER.empty() );

   PrivateKeyMap& privateKeys = (type == DomainPrivateKey ? mDomainPrivateKeys : mUserPrivateKeys);

   PrivateKeyMap::iterator where = privateKeys.find(key);
   if (where == privateKeys.end())
   {
      if (write)
      {
         onWritePEM(key, type, privateKeyDER);
      }
      BIO* in = BIO_new_mem_buf(const_cast<char*>(privateKeyDER.c_str()), -1);

      if ( !in )
      {
         ErrLog(<< "Could create BIO buffer from '" << privateKeyDER << "'");
         throw Exception("Could not create BIO buffer", __FILE__,__LINE__);
      }

      try
      {
         char* p = 0;
         if (type != DomainPrivateKey)
         {
            PassPhraseMap::const_iterator iter = mUserPassPhrases.find(key);
            if(iter != mUserPassPhrases.end())
            {
               p = const_cast<char*>(iter->second.c_str());
            }
         }

         BIO_set_close(in, BIO_NOCLOSE);

         EVP_PKEY* privateKey;
         if (d2i_PKCS8PrivateKey_bio(in, &privateKey, 0, p) == 0)
         {
            ErrLog(<< "Could not read private key from '" << privateKeyDER << "' using pass phrase '" << p << "'" );
            throw Exception("Could not read private key ", __FILE__,__LINE__);
         }

         privateKeys.insert(std::make_pair(key, privateKey));
      }
      catch(...)
      {
         BIO_free(in);
         throw;
      }
      BIO_free(in);
   }
}

void
BaseSecurity::addPrivateKeyPEM(
   PEMType type,
   const Data& key,
   const Data& privateKeyPEM,
   bool write)
{
   assert( !key.empty() );
   assert( !privateKeyPEM.empty() );

   PrivateKeyMap& privateKeys = (type == DomainPrivateKey ? mDomainPrivateKeys : mUserPrivateKeys);

   PrivateKeyMap::iterator where = privateKeys.find(key);
   if (where == privateKeys.end())
   {
      if (write)
      {
         onWritePEM(key, type, privateKeyPEM);
      }
      BIO* in = BIO_new_mem_buf(const_cast<char*>(privateKeyPEM.c_str()), -1);

      if ( !in )
      {
         ErrLog(<< "Could create BIO buffer from '" << privateKeyPEM << "'");
         throw Exception("Could not create BIO buffer", __FILE__,__LINE__);
      }

      try
      {
         char* p = 0;
         if (type != DomainPrivateKey)
         {
            PassPhraseMap::const_iterator iter = mUserPassPhrases.find(key);
            if(iter != mUserPassPhrases.end())
            {
               p = const_cast<char*>(iter->second.c_str());
            }
         }

         BIO_set_close(in, BIO_NOCLOSE);

         EVP_PKEY* privateKey;
         if (PEM_read_bio_PrivateKey(in, &privateKey, 0, p) == 0)
         {
            ErrLog(<< "Could not read private key from '" << privateKeyPEM << "' using pass phrase '" << p << "'" );
            throw Exception("Could not read private key ", __FILE__,__LINE__);
         }

         privateKeys.insert(std::make_pair(key, privateKey));
      }
      catch(...)
      {
         BIO_free(in);
         throw;
      }
      BIO_free(in);
   }
}
bool
BaseSecurity::hasPrivateKey(
   PEMType type,
   const Data& key,
   bool read) const
{
   assert( !key.empty() );

   PrivateKeyMap& privateKeys = (type == DomainPrivateKey ? mDomainPrivateKeys : mUserPrivateKeys);

   PrivateKeyMap::const_iterator where = privateKeys.find(key);
   if (where == privateKeys.end())
   {
      if (read)
      {
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
      return   false;
   }
   else
   {
      return   true;
   }
}
Data
BaseSecurity::getPrivateKeyPEM(
   PEMType type,
   const Data& key,
   bool read) const
{
   assert( !key.empty() );

   if (hasPrivateKey(type, key, true) == false)
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

   // !kh!
   // creates a read/write BIO buffer.
   BIO *out = BIO_new(BIO_s_mem());
   assert(out);
   EVP_PKEY* pk = where->second;
   assert(pk);

   // write pk to out using key phrase p, with no cipher.
   int ret = PEM_write_bio_PrivateKey(out, pk, 0, 0, 0, 0, p);
   assert(ret == 1);

   // get content in BIO buffer to our buffer.
   // hand our buffer to a Data object.
   BIO_flush(out);
   char* buf = 0;
   int len = BIO_get_mem_data(out, &buf);
   return   Data(Data::Take, buf, len);
}
Data
BaseSecurity::getPrivateKeyDER(
   PEMType type,
   const Data& key,
   bool read) const
{
   assert( !key.empty() );

   if (hasPrivateKey(type, key, true) == false)
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
   return   Data(Data::Take, buf, len);
}
bool
BaseSecurity::removePrivateKey(PEMType type, const Data& key, bool remove)
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
      return   true;
   }
   return   false;
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
   {
      // cleanup SSL_CTXes
      SSL_CTX_free(mTlsCtx);
      SSL_CTX_free(mSslCtx);
   }
}
void
BaseSecurity::initialize ()
{
   DebugLog( << "Setting up SSL library" );

   SSL_library_init();
   SSL_load_error_strings();
   OpenSSL_add_all_algorithms();
   //OpenSSL_add_ssl_algorithms();
   Random::initialize();
   Timer::getTimeMs(); // initalize time offsets

   // make sure that necessary algorithms exist:
   assert(EVP_des_ede3_cbc());
}

std::vector<BaseSecurity::CertificateInfo>
BaseSecurity::getRootCertDescriptions() const
{
   // !kh!
   // need to be implemented.
   assert(0);
   return   std::vector<CertificateInfo>();
}
void
BaseSecurity::addRootCertPEM(const Data& x509PEMEncodedRootCerts)
{
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
   return   hasCert(DomainCert, domainName, true);
}

bool
BaseSecurity::removeDomainCert(const Data& domainName)
{
   return   removeCert(DomainCert, domainName, true);
}

Data
BaseSecurity::getDomainCertDER(const Data& domainName) const
{
   return   getCertDER(DomainCert, domainName, true);
}

void
BaseSecurity::addDomainPrivateKeyPEM(const Data& domainName, const Data& privateKeyPEM)
{
   addPrivateKeyPEM(DomainPrivateKey, domainName, privateKeyPEM, true);
}
bool
BaseSecurity::hasDomainPrivateKey(const Data& domainName) const
{
   return   hasPrivateKey(DomainPrivateKey, domainName, true);
}
bool
BaseSecurity::removeDomainPrivateKey(const Data& domainName)
{
   return   removePrivateKey(DomainPrivateKey, domainName, true);
}
Data
BaseSecurity::getDomainPrivateKeyPEM(const Data& domainName) const
{
   return   getPrivateKeyPEM(DomainPrivateKey, domainName, true);
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
   return   hasCert(UserCert, aor, true);
}
bool
BaseSecurity::removeUserCert(const Data& aor)
{
   return   removeCert(UserCert, aor, true);
}
Data
BaseSecurity::getUserCertDER(const Data& aor) const
{
   return   getCertDER(UserCert, aor, true);
}

void
BaseSecurity::setUserPassPhrase(const Data& aor, const Data& passPhrase)
{
   assert(aor.empty());

   PassPhraseMap::iterator iter = mUserPassPhrases.find(aor);
   if(iter == mUserPassPhrases.end())
      mUserPassPhrases.insert(std::make_pair(aor, passPhrase));
}
bool
BaseSecurity::hasUserPassPhrase(const Data& aor) const
{
   assert(aor.empty());

   PassPhraseMap::const_iterator iter = mUserPassPhrases.find(aor);
   if(iter == mUserPassPhrases.end())
      return   false;
   else
      return   true;
}
bool
BaseSecurity::removeUserPassPhrase(const Data& aor)
{
   assert(aor.empty());

   PassPhraseMap::iterator iter = mUserPassPhrases.find(aor);
   if(iter == mUserPassPhrases.end())
   {
      return   false;
   }
   else
   {
      mUserPassPhrases.erase(iter);
      return   true;
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
   return   hasPrivateKey(UserPrivateKey, aor, true);
}
bool
BaseSecurity::removeUserPrivateKey(const Data& aor)
{
   return   removePrivateKey(UserPrivateKey, aor, true);
}
Data
BaseSecurity::getUserPrivateKeyPEM(const Data& aor) const
{
   return   getPrivateKeyPEM(UserPrivateKey, aor, true);
}
Data
BaseSecurity::getUserPrivateKeyDER(const Data& aor) const
{
   return   getPrivateKeyDER(UserPrivateKey, aor, true);
}

void
BaseSecurity::generateUserCert (const Data& aor, const Data& passPhrase)
{
   assert(0);
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
#if 0 // TODO !cj!
   flags |= PKCS7_NOCERTS; // should remove
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

   BIO* out = BIO_new(BIO_s_mem());
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
   // !jf! this is a really crappy parser - shoudl do proper mime stuff
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

   list<Contents*>::const_iterator i = multi->parts().begin();
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
      *sigStat = isBad;
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
               *sigStat = isBad;
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
               *sigStat = notTrusted;
            }
            else
            {
               if (false) // !jf! TODO look for this cert in store
               {
                  DebugLog( << "Signature is trusted" );
                  *sigStat = trusted;
               }
               else
               {
                  DebugLog( << "Signature is caTrusted" );
                  *sigStat = caTrusted;
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
   if (mTlsCtx == 0)
   {
      mTlsCtx = SSL_CTX_new( TLSv1_method() );
      assert(mTlsCtx);

      // make sure we have root certs
      assert(mRootCerts);
      configSslCtx(mTlsCtx, mRootCerts);
   }

   return   mTlsCtx;
}
SSL_CTX*
BaseSecurity::getSslCtx ()
{
   if (mSslCtx == 0)
   {
      mSslCtx = SSL_CTX_new( SSLv23_method() );
      assert(mSslCtx);

      // make sure we have root certs
      assert(mRootCerts);
      configSslCtx(mSslCtx, mRootCerts);
   }

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
