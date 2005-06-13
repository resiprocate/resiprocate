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
#include "resiprocate/Transport.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Random.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Timer.hxx"
#include "resiprocate/os/ParseBuffer.hxx"


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

static void
dumpAsn( char* name, Data data)
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


Security::Exception::Exception(const Data& msg, const Data& file, const int line) :
   BaseException(msg,file,line)
{
}

Security::Security( bool tlsServer, bool useTls )
{
    initialize(tlsServer, useTls ? SecurityTypes::TLSv1 : SecurityTypes::SSLv23);
}

Security::Security( bool tlsServer, SecurityTypes::SSLType mode)
{
    initialize(tlsServer, mode);
}

void
Security::initialize( bool tlsServer, SecurityTypes::SSLType mode )
{
   mTlsServer = tlsServer;

   assert( (mode & SecurityTypes::SSLv23) ^ (mode & SecurityTypes::TLSv1) );

   mSSLMode = mode;

   privateKey = 0;
   publicCert = 0;
   privateIdentityKey = 0;
   publicIdentityCert = 0;
   certAuthorities = 0;
   ctxTls = 0;

   // !cj! check-lock-check TODO - this code is broken - use pthreadOnce
   static bool initDone = false;
   if (!initDone)
   {
      static Mutex once;
      Lock lock(once);
      if (!initDone)
      {
         initDone = true;
         initialize();
      }
   }
}


void
Security::initialize()
{
   DebugLog( << "Setting up SSL library" );

   SSL_library_init();
   SSL_load_error_strings();
   OpenSSL_add_ssl_algorithms();
   Random::initialize();
   Timer::getTimeMs(); // initalize time offsets
}


SSL_CTX*
Security::getTlsCtx(bool isServer)
{
   DebugLog(<<"Security::getTlsCtx() this="<<(void*)this);
   if ( ctxTls )
   {
      return ctxTls;
   }

   if ( mSSLMode & SecurityTypes::TLSv1 )
   {
      InfoLog( << "Setting up to be a TLS v1 server" );

      if (1)
      {
         ctxTls=SSL_CTX_new( TLSv1_method() );
      }
      else
      { // why would we do this?
         if ( isServer )
         {
            ctxTls=SSL_CTX_new( TLSv1_server_method() );
         }
         else
         {
            ctxTls=SSL_CTX_new( TLSv1_client_method() );
         }
      }
   }
   else if ( mSSLMode & SecurityTypes::SSLv23 )
   {
      ErrLog( << "Warning - using SSL v2 v3 instead of TLS" );
      ctxTls=SSL_CTX_new(  SSLv23_method() );
   }
   else
   {
      throw Exception("Unknown SSL protocol mode requested",__FILE__,__LINE__);
   }
   assert( ctxTls );

   if ( isServer )
   {
      DebugLog( << "Setting up as TLS server" );
      assert( publicCert );
      assert( privateKey );
   }
   else
   {
      DebugLog( << "Setting up as TLS client" );
   }

#if 0
   if ( mTlsServer )
   {
      if ( publicCert )
      {
         if(!SSL_CTX_use_certificate(ctxTls, publicCert))
         throw Exception("SSL_CTX_use_certificate failed",
         __FILE__,__LINE__);
      }
      if (privateKey)
      {
         if (!SSL_CTX_use_PrivateKey(ctxTls,privateKey))
         throw Exception("SSL_CTX_use_PrivateKey failed.",
         __FILE__,__LINE__);
      }
   }
#endif

   assert( certAuthorities );
   SSL_CTX_set_cert_store(ctxTls, certAuthorities);

   // set up the cipher
   char* cipher="RSA+SHA+AES+3DES";
   int ret = SSL_CTX_set_cipher_list(ctxTls,cipher);
   if ( ret == 0 )
   {
      ErrLog( << "Could not set any TLS ciphers");
   }

   DebugLog( << "Done setting up TLS client" );

   return ctxTls;
}


Security::~Security()
{
   // TODO !cj! shoudl clean up contexts
}


Data
Security::getPath( const Data& dirPath, const Data& file )
{
   Data path = dirPath;

   if ( path.empty() )
   {
      char* v = getenv("SIP");
      if (v)
      {
         path = Data(v);
      }
      else
      {
         v = getenv("HOME");
         if ( v )
         {
            path = Data(v);
            path += Data("/.sip");
         }
         else
         {
#ifdef WIN32
            path = "C:\\certs";
#else
            ErrLog( << "Environment variable HOME is not set" );
            path = "/etc/sip";
#endif
         }
      }
   }

#ifdef WIN32
   path += Data("\\");
#else
   path += Data("/");
#endif

   assert( !file.empty() );
   path += file;
   DebugLog( << "Using file path " << path );

   return path;
}


bool
Security::loadAllCerts( const Data& password, const Data& dirPath )
{
   bool ok = true;
   ok = loadRootCerts( getPath( dirPath, Data("root.pem")) ) ? ok : false;

   ok = loadMyPublicCert( getPath( dirPath, Data("id.pem")) ) ? ok : false;
   ok = loadMyPrivateKey( password, getPath(dirPath,Data("id_key.pem") )) ? ok : false;

   ok = loadMyPublicIdentityCert( getPath( dirPath, Data("identity.pem")) ) ? ok : false;
   ok = loadMyPrivateIdentityKey( password, getPath(dirPath,Data("identity_key.pem") )) ? ok : false;

#if 0
   if (ok)
   {
      getTlsCtx();
      getSmimeCtx();
   }
#endif

   Data pubKeyDir("public_keys");
   pubKeyDir += Symbols::pathSep;

   ok = loadPublicCert(getPath(dirPath,pubKeyDir)) ? ok : false;

   return ok;
}


bool
Security::loadMyPublicCert( const Data&  filePath )
{
   assert( !filePath.empty() );

   if (publicCert)
   {
      return true;
   }

   FILE* fp = fopen(filePath.c_str(),"rb");
   if ( !fp )
   {
      ErrLog(<< "Could not read my public cert from " << filePath );
      throw Exception("Could not read public certificate", __FILE__,__LINE__);
      return false;
   }

   publicCert = PEM_read_X509(fp,0,0,0);
   if (!publicCert)
   {
      ErrLog( << "Error reading contents of my public cert file " << filePath );

      while (true)
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
         DebugLog( << "Error code = " << code << " file=" << file << " line=" << line );
      }

      ErrLog (<< "Error reading contents of my public cert file " << filePath );
      throw Exception("Error reading contents of public cert file", __FILE__,__LINE__);

      return false;
   }

   InfoLog( << "Loaded my public cert from " << filePath );
   return true;
}


bool
Security::loadMyPublicIdentityCert( const Data&  filePath )
{
   assert( !filePath.empty() );

   if (publicIdentityCert)
   {
      return true;
   }

   FILE* fp = fopen(filePath.c_str(),"rb");
   if ( !fp )
   {
      ErrLog(<< "Could not read my public identity cert from " << filePath );
      throw Exception("Could not read public identity certificate", __FILE__,__LINE__);
      return false;
   }

   publicIdentityCert = PEM_read_X509(fp,0,0,0);
   if (!publicIdentityCert)
   {
      ErrLog( << "Error reading contents of my public identity cert file " << filePath );

      while (true)
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
         DebugLog( << "Error code = " << code << " file=" << file << " line=" << line );
      }

      ErrLog (<< "Error reading contents of my public identity cert file " << filePath );
      throw Exception("Error reading contents of public identity cert file", __FILE__,__LINE__);

      return false;
   }

   InfoLog( << "Loaded my public identity cert from " << filePath );
   return true;
}


bool
Security::loadRootCerts(  const Data& filePath )
{
   assert( !filePath.empty() );

   if (certAuthorities == 0)
       certAuthorities = X509_STORE_new();
   assert( certAuthorities );

   if ( X509_STORE_load_locations(certAuthorities,filePath.c_str(),0) != 1 )
   {
      ErrLog( << "Error reading contents of root cert file " << filePath );

      while (true)
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
         ErrLog( << "Error code = " << code << " file=" << file << " line=" << line );
      }

      throw Exception("Error reading contents of root certificate file", __FILE__,__LINE__);
      return false;
   }

   InfoLog( << "Loaded public CAs from " << filePath );

   return true;
}


bool
Security::loadPublicCert(  const Data& filePath )
{
   assert( !filePath.empty() );

#ifdef WIN32
   WIN32_FIND_DATA FileData;
   HANDLE hSearch;
   Data searchPath = filePath + Data("*");
   hSearch = FindFirstFile( searchPath.c_str(), &FileData);
   if (hSearch == INVALID_HANDLE_VALUE)
   {
      ErrLog (<< "Error reading public certificate directory: " << filePath);
      throw Exception("Error reading public cert directory", __FILE__,__LINE__);
      return false;
   }

   bool done = false;
   while (!done)
   {
      Data name( FileData.cFileName);
      Data path = filePath;
      //path += "\\";
      path += name;

      if ( strchr( name.c_str(), '@' ))
      {
         FILE* fp = fopen(path.c_str(),"rb");
         if ( !fp )
         {
            ErrLog( << "Could not read public key from " << path );
            throw Exception("Could not read other person's public key", __FILE__,__LINE__);
         }
         else
         {
            X509* cert = PEM_read_X509(fp,0,0,0);
            if (!cert)
            {
               ErrLog( << "Error reading contents of public key file " << path );

               while (true)
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
                  DebugLog( << "Error code = " << code << " file=" << file << " line=" << line );
               }
               throw Exception("Error reading contents of other persons public key file ", __FILE__,__LINE__);
            }
            else
            {
               publicKeys[name] = cert;
            }
         }

      }

      if (!FindNextFile(hSearch, &FileData))
      {
         if (GetLastError() == ERROR_NO_MORE_FILES)
         {
            done = true;
         }
         else
         {
            throw Exception("Bizarre problem reading public certificate directory", __FILE__,__LINE__);
            return false;
         }
      }
   }
   FindClose(hSearch);

   return true;
#else
   DIR* dir = opendir( filePath.c_str() );

   if (!dir )
   {
      ErrLog( << "Error reading public key directory  " << filePath );
      return false;
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
      Data path = filePath;
      path += name;

      if ( !strchr( name.c_str(), '@' ))
      {
         DebugLog( << "skipping file " << path );
         continue;
      }

      FILE* fp = fopen(path.c_str(),"rb");
      if ( !fp )
      {
         ErrLog( << "Could not read public key from " << path );
         continue;
      }

      X509* cert = PEM_read_X509(fp,0,0,0);
      if (!cert)
      {
         ErrLog( << "Error reading contents of public key file " << path );
         while (true)
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
            DebugLog( << "Error code = " << code << " file=" << file << " line=" << line );
         }

         continue;
      }

      publicKeys[name] = cert;

      InfoLog( << "Loaded public key for " << name );
   }

   closedir( dir );
#endif

   return true;
}


bool
Security::savePublicCert( const Data& certName,  const Data& filePath )
{
   assert(0);
   return false;
}


bool
Security::loadMyPrivateKey( const Data& password, const Data&  filePath )
{
   assert( !filePath.empty() );

   if ( privateKey )
   {
      return true;
   }

   FILE* fp = fopen(filePath.c_str(),"rb");
   if ( !fp )
   {
      ErrLog( << "Could not read private key from " << filePath );
      throw Exception("Could not read private key", __FILE__,__LINE__);
      return false;
   }

   //DebugLog( << "password is " << password );

   privateKey = PEM_read_PrivateKey(fp,0,0,(void*)password.c_str());
   if (!privateKey)
   {
      ErrLog( << "Error reading contents of private key file " << filePath );
      while (true)
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
         DebugLog( << "Error code = " << code << " file=" << file << " line=" << line );
      }
      throw Exception("Error reading contents of private key file", __FILE__,__LINE__);
      return false;
   }

   InfoLog( << "Loaded private key from " << filePath );

   assert( privateKey );
   return true;
}


bool
Security::loadMyPrivateIdentityKey( const Data& password, const Data&  filePath )
{
   assert( !filePath.empty() );

   if ( privateIdentityKey )
   {
      return true;
   }

   FILE* fp = fopen(filePath.c_str(),"rb");
   if ( !fp )
   {
      ErrLog( << "Could not read private identiy key from " << filePath );
      throw Exception("Could not read private  identiy key", __FILE__,__LINE__);
      return false;
   }

   //DebugLog( << "password is " << password );

   privateIdentityKey = PEM_read_PrivateKey(fp,0,0,(void*)password.c_str());
   if (!privateKey)
   {
      ErrLog( << "Error reading contents of private  identiy key file " << filePath );
      while (true)
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
         DebugLog( << "Error code = " << code << " file=" << file << " line=" << line );
      }
      throw Exception("Error reading contents of private  identiy key file", __FILE__,__LINE__);
      return false;
   }

   InfoLog( << "Loaded private  identiy key from " << filePath );

   assert( privateIdentityKey );
   return true;
}


// ------------------------------------------------------------
// OpenSSL certificate loading from PEM strings
//

// derived from openssl/crypto/x509/by_file.c : X509_load_cert_crl_file
//
static int
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

static int
pemstring_ctrl(X509_LOOKUP *lu, int cmd, const char *argp, long argl,
	     char **ret)
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

bool
Security::setMyPublicCert( const Data&  certPem )
{
   assert( !certPem.empty() );

   if (publicCert)
   {
      return true;
   }

   BIO* in = BIO_new_mem_buf(const_cast<char*>(certPem.c_str()), -1);

   if ( !in )
   {
      ErrLog(<< "Could not read my public cert from " << certPem );
      throw Exception("Could not read public certificate", __FILE__,__LINE__);
      return false;
   }
   BIO_set_close(in, BIO_NOCLOSE);

   publicCert = PEM_read_bio_X509(in,0,0,0);
   BIO_free(in);

   if (!publicCert)
   {
      ErrLog( << "Error reading my public cert from " << certPem );
      throw Exception("Error reading public cert", __FILE__,__LINE__);

      return false;
   }

   return true;
}

bool
Security::setMyPrivateKey( const Data& password, const Data& keyPem )
{
   assert( !keyPem.empty() );

   if ( privateKey )
   {
      return true;
   }

   BIO* in = BIO_new_mem_buf(const_cast<char*>(keyPem.c_str()), -1);

   if ( !in )
   {
      ErrLog(<< "Could not read my private key from " << keyPem );
      throw Exception("Could not read private key", __FILE__,__LINE__);
      return false;
   }
   BIO_set_close(in, BIO_NOCLOSE);

   DebugLog( << "password is " << password );

   privateKey = PEM_read_bio_PrivateKey(in,0,0,(void*)password.c_str());
   BIO_free(in);

   if (!privateKey)
   {
      ErrLog( << "Error reading my private key from " << keyPem );
      throw Exception("Could not read private key", __FILE__,__LINE__);
      return false;
   }

   return true;
}

bool
Security::setRootCerts(  const Data& certPem )
{
   assert( !certPem.empty() );

   if (certAuthorities == 0)
       certAuthorities = X509_STORE_new();

   assert( certAuthorities );

   static X509_LOOKUP_METHOD x509_pemstring_lookup=
	{
	"Load cert from PEM string into cache",
	NULL,		/* new */
	NULL,		/* free */
	NULL, 		/* init */
	NULL,		/* shutdown */
	pemstring_ctrl,	/* ctrl */
	NULL,		/* get_by_subject */
	NULL,		/* get_by_issuer_serial */
	NULL,		/* get_by_fingerprint */
	NULL,		/* get_by_alias */
	};

   X509_LOOKUP* lookup = X509_STORE_add_lookup(certAuthorities, &x509_pemstring_lookup);
   if (lookup == NULL)
       return false;		// cleanup store?

   X509_LOOKUP_ctrl(lookup, X509_L_FILE_LOAD, certPem.c_str(), 0, 0);

   return true;
}


bool
Security::checkIdentity( const Data& in, const Data& sigBase64 )
{
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

   EVP_PKEY* pKey = X509_get_pubkey( publicIdentityCert );
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


Data
Security::computeIdentity( const Data& in )
{
   DebugLog( << "Compute identity for " << in );

   EVP_PKEY* pKey = privateIdentityKey;
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

   dumpAsn("identity-in", in );
   dumpAsn("identity-in-hash", Data(hashRes, hashResLen) );
   dumpAsn("identity-in-rsa",res);
   dumpAsn("identity-in-base64",enc);

   return enc;
}


Contents*
Security::sign( Contents* bodyIn )
{
   return multipartSign( bodyIn );
}


MultipartSignedContents*
Security::multipartSign( Contents* bodyIn )
{
   DebugLog( << "Doing multipartSign" );
   assert( bodyIn );

   // form the multipart
   MultipartSignedContents* multi = new MultipartSignedContents;
   multi->header(h_ContentType).param( p_micalg ) = "sha1";
   multi->header(h_ContentType).param( p_protocol ) = "application/pkcs7-signature";
   multi->header(h_ContentType).type() = "multipart";
   multi->header(h_ContentType).subType() = "signed";

   // add the main body to it
   Contents* body =  bodyIn->clone();
   assert( body );

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
   dumpAsn("resip-sign-out-data",bodyData);

   const char* p = bodyData.data();
   int s = bodyData.size();
   BIO* in;
   in = BIO_new_mem_buf( (void*)p,s);
   assert(in);
   DebugLog( << "ceated in BIO");

   BIO* out;
   out = BIO_new(BIO_s_mem());
   assert(out);
   DebugLog( << "created out BIO" );

   STACK_OF(X509)* chain=0;
   chain = sk_X509_new_null();
   assert(chain);

   DebugLog( << "checking" );
   assert( publicCert );
   assert( privateKey );

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
   dumpAsn("resip-sign-out-sig",outData);

   Pkcs7Contents* sigBody = new Pkcs7Contents( outData );
   assert( sigBody );

   sigBody->header(h_ContentType).type() = "application";
   sigBody->header(h_ContentType).subType() = "pkcs7-signature";
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
Security::pkcs7Sign( Contents* bodyIn )
{
   assert( bodyIn );

   int flags = 0;
   flags |= PKCS7_BINARY;
#if 0 // TODO !cj!
   flags |= PKCS7_NOCERTS; // should remove
#endif

   Data bodyData;
   DataStream strm(bodyData);
   bodyIn->encodeHeaders(strm);
   bodyIn->encode( strm );
   strm.flush();

   DebugLog( << "body data to sign is <" << bodyData << ">" );

   const char* p = bodyData.data();
   int s = bodyData.size();

   BIO* in;
   in = BIO_new_mem_buf( (void*)p,s);
   assert(in);
   DebugLog( << "ceated in BIO");

   BIO* out;
   out = BIO_new(BIO_s_mem());
   assert(out);
   DebugLog( << "created out BIO" );

   STACK_OF(X509)* chain=0;
   chain = sk_X509_new_null();
   assert(chain);

   DebugLog( << "checking" );
   assert( publicCert );
   assert( privateKey );

   int i = X509_check_private_key(publicCert, privateKey);
   DebugLog( << "checked cert and key ret=" << i  );

   PKCS7* pkcs7 = PKCS7_sign( publicCert, privateKey, chain, in, flags);
   if ( !pkcs7 )
   {
      ErrLog( << "Error creating PKCS7 signing object" );
      return 0;
   }
   DebugLog( << "created PKCS7 sign object " );

   i2d_PKCS7_bio(out,pkcs7);

   BIO_flush(out);

   char* outBuf=0;
   long size = BIO_get_mem_data(out,&outBuf);
   assert( size > 0 );

   Data outData(outBuf,size);

   InfoLog( << "Signed body size is <" << outData.size() << ">" );
   //InfoLog( << "Signed body is <" << outData.escaped() << ">" );

   Pkcs7Contents* outBody = new Pkcs7Contents( outData );
   assert( outBody );

   outBody->header(h_ContentType).type() = "application";
   outBody->header(h_ContentType).subType() = "pkcs7-mime";
   outBody->header(h_ContentType).param( p_smimeType ) = "signed-data";
   outBody->header(h_ContentType).param( p_name ) = "smime.p7s";
   outBody->header(h_ContentDisposition).param( p_handling ) = "required";
   outBody->header(h_ContentDisposition).param( p_filename ) = "smime.p7s";
   outBody->header(h_ContentDisposition).value() =  "attachment" ;

   return outBody;
}


bool
Security::haveCert()
{
   if ( !privateKey ) return false;
   if ( !publicCert ) return false;

   return true;
}


bool
Security::havePublicKey( const Data& recipCertName )
{
   DebugLog( <<"looking for public key for " << recipCertName );

   MapConstIterator i = publicKeys.find(recipCertName);
   if (i != publicKeys.end())
   {
      return true;
   }

   return false;
}


Pkcs7Contents*
Security::encrypt( Contents* bodyIn, const Data& recipCertName )
{
   assert( bodyIn );

   int flags = 0 ;
   flags |= PKCS7_BINARY;
#if 0 // TODO !cj!
   flags |= PKCS7_NOCERTS; // should remove
#endif

   Data bodyData;
   DataStream strm(bodyData);
   bodyIn->encodeHeaders(strm);
   bodyIn->encode( strm );
   strm.flush();

   InfoLog( << "body data to encrypt is <" << bodyData.escaped() << ">" );

   const char* p = bodyData.data();
   int s = bodyData.size();

   BIO* in;
   in = BIO_new_mem_buf( (void*)p,s);
   assert(in);
   DebugLog( << "ceated in BIO");

   BIO* out;
   out = BIO_new(BIO_s_mem());
   assert(out);
   DebugLog( << "created out BIO" );

   InfoLog( << "target cert name is " << recipCertName );
   X509* cert = 0;

   // cert = publicKeys[recipCertName];
   MapConstIterator i = publicKeys.find(recipCertName);
   if (i != publicKeys.end())
   {
      cert = i->second;
   }
   else
   {
      ErrLog( << "Do not have a public key for " << recipCertName );
      return 0;
   }
   assert(cert);

   STACK_OF(X509) *certs;
   certs = sk_X509_new_null();
   assert(certs);
   assert( cert );
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

   InfoLog( << Data("Encrypted body size is ") << outData.size() );
   InfoLog( << Data("Encrypted body is <") << outData.escaped() << ">" );

   dumpAsn("resip-encrpt-out",outData);

   Pkcs7Contents* outBody = new Pkcs7Contents( outData );
   assert( outBody );

   outBody->header(h_ContentType).type() = "application";
   outBody->header(h_ContentType).subType() = "pkcs7-mime";
   outBody->header(h_ContentType).param( p_smimeType ) = "enveloped-data";
   outBody->header(h_ContentType).param( p_name ) = "smime.p7m";
   outBody->header(h_ContentDisposition).param( p_handling ) = "required";
   outBody->header(h_ContentDisposition).param( p_filename ) = "smime.p7";
   outBody->header(h_ContentDisposition).value() =  "attachment" ;

   return outBody;
}


Contents*
Security::uncodeSigned( MultipartSignedContents* multi,
                        Data* signedBy,
                        SignatureStatus* sigStatus )
{
   if ( multi->parts().size() != 2 )
   {
      return 0;
   }

   list<Contents*>::const_iterator i = multi->parts().begin();
   Contents* first = *i;
   ++i;
   assert( i != multi->parts().end() );
   Contents* second = *i;
   Pkcs7SignedContents* sig = dynamic_cast<Pkcs7SignedContents*>( second );

   if ( !sig )
   {
      ErrLog( << "Don't know how to deal with signature type" );
      return first;
   }

   int flags=0;
   flags |= PKCS7_BINARY;

   assert( second );
   assert( first );

   CerrLog( << "message to sign is " << *first );
   //CerrLog( << "first is of type" << typename(*first) );

   Data bodyData;
   DataStream strm( bodyData );
   first->encodeHeaders( strm );
   first->encode( strm );
   strm.flush();
   CerrLog( << "encoded version to sign is " << bodyData );

   // Data textData = first->getBodyData();
   Data textData = bodyData;
   Data sigData = sig->getBodyData();

   dumpAsn( "resip-asn-uncode-signed-text", textData );
   dumpAsn( "resip-asn-uncode-signed-sig", sigData );

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
#if 1
   // add all the public certs to the stack
   //  !cj! TODO - should be just the people names that match who this msg was from
   MapConstIterator index = publicKeys.begin();
   while ( index != publicKeys.end())
   {
      InfoLog( << "Added a public cert for " << index->first  );
      X509* cert = index->second;
      sk_X509_push(certs, cert);
      index++;
   }
#endif

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

#if 1
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
#else
         STACK* emails = X509_get1_email(x);

         for ( int j=0; j<sk_num(emails); j++)
         {
            char* e = sk_value(emails,j);
            InfoLog(<< "email field of signing cert is <" << e << ">" );
            if ( signedBy)
            {

               *signedBy = Data(e);
            }
         }
#endif
      }
   }
   else
   {
      InfoLog(<< "No signers of this messages" );
   }

#if 0
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

   assert( certAuthorities );

   switch (type)
   {
      case NID_pkcs7_signed:
      {
         if ( PKCS7_verify(pkcs7, certs, certAuthorities, pkcs7Bio, out, flags ) != 1 )
         {
            ErrLog( << "Problems doing PKCS7_verify" );

            if ( sigStatus )
            {
               *sigStatus = isBad;
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
         if ( sigStatus )
         {
            if ( flags & PKCS7_NOVERIFY )
            {
               DebugLog( << "Signature is notTrusted" );
               *sigStatus = notTrusted;
            }
            else
            {
               if (false) // !jf! TODO look for this cert in store
               {
                  DebugLog( << "Signature is trusted" );
                  *sigStatus = trusted;
               }
               else
               {
                  DebugLog( << "Signature is caTrusted" );
                  *sigStatus = caTrusted;
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


Contents*
Security::decrypt( Pkcs7Contents* sBody )
{
   int flags=0;
   flags |= PKCS7_BINARY;

   // for now, assume that this is only a singed message
   assert( sBody );

   Data text = sBody->getBodyData();
   DebugLog( << "uncode body = <" << text.escaped() << ">" );
   DebugLog( << "uncode body size = " << text.size() );

   dumpAsn("resip-asn-decrypt", text );

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

   STACK_OF(X509)* certs;
   certs = sk_X509_new_null();
   assert( certs );

   //   flags |= PKCS7_NOVERIFY;

   assert( certAuthorities );

   switch (type)
   {
      case NID_pkcs7_signedAndEnveloped:
      {
         assert(0);
      }
      break;

      case NID_pkcs7_enveloped:
      {
         if ( (!privateKey) || (!publicCert) )

         {
            InfoLog( << "Don't have a private certifact to user for  PKCS7_decrypt" );
            return 0;
         }

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
         return 0;
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
