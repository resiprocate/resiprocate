#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

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

#if defined(USE_SSL)

#if !defined(WIN32)
#include <unistd.h>
#include <pthread.h>
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

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


Security::Exception::Exception(const Data& msg, const Data& file, const int line) :
   BaseException(msg,file,line)
{
}

Security::Security( bool tlsServer, bool useTls )
{
   privateKey = NULL;
   publicCert = NULL;
   certAuthorities = NULL;
   ctxTls = NULL;
   mTlsServer = tlsServer;
   mUseTls = useTls;
      
#ifdef WIN32
   // !jf! not threadsafe
   static bool initDone=false;
   if (!initDone)
   {
      initialize();
      initDone = true;
   }
#else
   // for thread safety
   pthread_once_t once_control = PTHREAD_ONCE_INIT;
   pthread_once(&once_control, Security::initialize);
#endif
}

void
Security::initialize()
{
   DebugLog( << "Setting up SSL library" );
      
   SSL_library_init();
   SSL_load_error_strings();
   OpenSSL_add_ssl_algorithms();
   Random::initialize();
}


SSL_CTX* 
Security::getTlsCtx(bool isServer)
{
   if ( ctxTls )
   {
      return ctxTls;
   }
   
   if ( mUseTls )
   {
      InfoLog( << "Setting up to be a TLS v1 server" );

# if 1
    ctxTls=SSL_CTX_new( TLSv1_method() ); 
#else
      if ( isServer )
      {
         ctxTls=SSL_CTX_new( TLSv1_server_method() );
      }
      else
      {
         ctxTls=SSL_CTX_new( TLSv1_client_method() );
      }
#endif
   }
   else
   {
      ErrLog( << "Warning - using SSL v2 v3 instead of TLS" );
      ctxTls=SSL_CTX_new(  SSLv23_method() );
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
   
   int ok;
   
   if ( mTlsServer )
   {
      if ( publicCert )
      {
         ok = SSL_CTX_use_certificate(ctxTls, publicCert);
         assert( ok == 1);
      }
      
      if (privateKey)
      {
         ok = SSL_CTX_use_PrivateKey(ctxTls,privateKey);
         assert( ok == 1);
      }
   }
   
   assert( certAuthorities );
   SSL_CTX_set_cert_store(ctxTls, certAuthorities);

#if 1
   // set up the cipher
   char* cipher="RSA+SHA+AES+3DES";
   int ret = SSL_CTX_set_cipher_list(ctxTls,cipher);
   if ( ret == 0 )
   {
      ErrLog( << "Could not set any TLS ciphers");
   }
#endif

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
            ErrLog( << "Environment variobal HOME is not set" );
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

#if 0
   if (ok)
   {
      getTlsCtx();
      getSmimeCtx();
   }
#endif
   
#ifdef WIN32
   Data pubKeyDir("public_keys\\");
#else
   Data pubKeyDir("public_keys/");
#endif

   ok = loadPublicCert( getPath(dirPath,pubKeyDir) ) ? ok : false;
   
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
      Data err( "Could not read my public cert from " );
      err += filePath;
      throw Exception(err, __FILE__,__LINE__);
      return false;
   }
   
   publicCert = PEM_read_X509(fp,NULL,NULL,NULL);
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
      
      Data err( "Error reading contents of my public cert file " );
      err += filePath;
      throw Exception(err, __FILE__,__LINE__);
      
      return false;
   }
   
   InfoLog( << "Loaded my public cert from " << filePath );
   
   return true;
}


bool 
Security::loadRootCerts(  const Data& filePath )
{ 
   assert( !filePath.empty() );
   
   if (certAuthorities)
   {
      return true;
   }
   
   certAuthorities = X509_STORE_new();
   assert( certAuthorities );
   
   if ( X509_STORE_load_locations(certAuthorities,filePath.c_str(),NULL) != 1 )
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
         DebugLog( << "Error code = " << code << " file=" << file << " line=" << line );
      }
      
      Data err( "Error reading contents of root cert file  " );
      err += filePath;
      throw Exception(err, __FILE__,__LINE__);
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
      Data err( "Error reading public cert directory " );
      err += filePath;
      throw Exception(err, __FILE__,__LINE__);
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
            Data err( "Could not read other persons public key from " );
            err += path;
            throw Exception(err, __FILE__,__LINE__);
         }
         else
         {
            X509* cert = PEM_read_X509(fp,NULL,NULL,NULL);
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
      

               Data err( "Error reading contents of other persons public key file " );
               err += path;
               throw Exception(err, __FILE__,__LINE__);
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
            Data err( "Bizarre problem reading public certificate direcotyr" );
            err += filePath;
            throw Exception(err, __FILE__,__LINE__);
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

   struct dirent * d = NULL;
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

      X509* cert = PEM_read_X509(fp,NULL,NULL,NULL);
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

      DebugLog( << "Loaded public key from " << name );         
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

      Data err( "Could not read private key from " );
      err += filePath;
      throw Exception(err, __FILE__,__LINE__);
	  
      return false;
   }
   
   DebugLog( << "password is " << password );
   
   privateKey = PEM_read_PrivateKey(fp,NULL,NULL,(void*)password.c_str());
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
      

      Data err( "Error reading contents of private key file " );
      err += filePath;
      throw Exception(err, __FILE__,__LINE__);

      return false;
   }
   
   InfoLog( << "Loaded private key from << " << filePath );
   
   assert( privateKey );
   return true;
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
   body->header(h_ContentTransferEncoding).value() = "binary";
   multi->parts().push_back( body );

   // compute the signature 
   int flags = 0;
   flags |= PKCS7_BINARY;
   flags |= PKCS7_DETACHED;
#if 0
   flags |= PKCS7_NOCERTS; // should remove 
#endif

   Data bodyData = bodyIn->getBodyData();
   const char* p = bodyData.data();
   int s = bodyData.size();
   BIO* in;
   DebugLog( << "sign <" << bodyData.data() << ">" );
   in = BIO_new_mem_buf( (void*)p,s);
   assert(in);
   DebugLog( << "ceated in BIO");
    
   BIO* out;
   out = BIO_new(BIO_s_mem());
   assert(out);
   DebugLog( << "created out BIO" );
     
   STACK_OF(X509)* chain=NULL;
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
      return NULL;
   }
   DebugLog( << "created PKCS7 signature object " );

   i2d_PKCS7_bio(out,pkcs7);
   BIO_flush(out);
   
   char* outBuf=NULL;
   long size = BIO_get_mem_data(out,&outBuf);
   assert( size > 0 );
   
   Data outData(outBuf,size);
  
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
   
   Data bodyData;
   oDataStream strm(bodyData);

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
     
   STACK_OF(X509)* chain=NULL;
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
      return NULL;
   }
   DebugLog( << "created PKCS7 sign object " );

   i2d_PKCS7_bio(out,pkcs7);

   BIO_flush(out);
   
   char* outBuf=NULL;
   long size = BIO_get_mem_data(out,&outBuf);
   assert( size > 0 );
   
   Data outData(outBuf,size);
  
   InfoLog( << "Signed body size is <" << outData.size() << ">" );
   //InfoLog( << "Signed body is <" << outData.escaped() << ">" );

   Pkcs7Contents* outBody = new Pkcs7Contents( outData );
   assert( outBody );

   // !cj! change these from using unkonw paramters types 
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
   
   Data bodyData;
   oDataStream strm(bodyData);

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
   X509* cert = NULL;
 
   // cert = publicKeys[recipCertName]; 
   MapConstIterator i = publicKeys.find(recipCertName);
   if (i != publicKeys.end())
   {
      cert = i->second;
   }
   else
   {
      ErrLog( << "Do not have a public key for " << recipCertName );      
      return NULL;
   }
   assert(cert);
      
   STACK_OF(X509) *certs;
   certs = sk_X509_new_null();
   assert(certs);
   assert( cert );
   sk_X509_push(certs, cert);
   
   EVP_CIPHER* cipher =   (EVP_CIPHER*) EVP_des_ede3_cbc();
//   const EVP_CIPHER* cipher = EVP_des_ede3_cbc(); // !jf! - should really use
//   this one

   //const EVP_CIPHER* cipher = EVP_aes_128_cbc();
   //const EVP_CIPHER* cipher = EVP_enc_null();
   assert( cipher );
   
   PKCS7* pkcs7 = PKCS7_encrypt( certs, in, cipher, flags);
   if ( !pkcs7 )
   {
      ErrLog( << "Error creating PKCS7 encrypt object" );
      return NULL;
   }
   DebugLog( << "created PKCS7 encrypt object " );

   i2d_PKCS7_bio(out,pkcs7);

   BIO_flush(out);
   
   char* outBuf=NULL;
   long size = BIO_get_mem_data(out,&outBuf);
   assert( size > 0 );
   
   Data outData(outBuf,size);
  
   InfoLog( << Data("Encrypted body size is ") << outData.size() );
   InfoLog( << Data("Encrypted body is <") << outData.escaped() << ">" );
   InfoLog( << Data("Encrypted body is <") << outData.size() << ">" );

   Pkcs7Contents* outBody = new Pkcs7Contents( outData );
   assert( outBody );

   // !cj! change these from using unkonw paramters types 
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
      return NULL;
   }
   
   list<Contents*>::const_iterator i = multi->parts().begin();
   Contents* first = *i;
   
   i++;
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
      
   Data textData = first->getBodyData();
   Data sigData = sig->getBodyData();
      
   BIO* in = BIO_new_mem_buf( (void*)sigData.c_str(),sigData.size());
   assert(in);
   InfoLog( << "ceated in BIO");
    
   BIO* out = BIO_new(BIO_s_mem());
   assert(out);
   InfoLog( << "created out BIO" );

   DebugLog( << "verify <"    << textData.escaped() << ">" );
   DebugLog( << "signature <" << sigData.escaped() << ">" );
   //DebugLog( << "signature text <" << sig->text().escaped() << ">" );
      
   BIO* pkcs7Bio = BIO_new_mem_buf( (void*) textData.c_str(),textData.size());
   assert(pkcs7Bio);
   InfoLog( << "ceated pkcs BIO");
    
   PKCS7* pkcs7 = d2i_PKCS7_bio(in, NULL);
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
         InfoLog( << "Error code = " << code << " file=" << file << " line=" << line );
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

   STACK_OF(X509)* signers = PKCS7_get0_signers(pkcs7, NULL/*certs*/, 0/*flags*/ );
   for (int i=0; i<sk_X509_num(signers); i++)
   {
      X509* x = sk_X509_value(signers,i);

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
   }

   STACK_OF(X509)* certs;
   certs = sk_X509_new_null();
   assert( certs );
   
   //      if ( !verifySig )
   {
      flags |= PKCS7_NOVERIFY;
   }
   
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
               *sigStatus = notTrusted;
            }
            else
            {
               if (false) // !jf! TODO look for this cert in store
               {
                  *sigStatus = trusted;
               }
               else
               {
                  *sigStatus = caTrusted;
               }
            }
         }
      }
      break;
      
      default:
         ErrLog(<< "Got PKCS7 data that could not be handled type=" << type );
         return NULL;
   }
      
   BIO_flush(out);
   char* outBuf=NULL;
   long size = BIO_get_mem_data(out,&outBuf);
   assert( size >= 0 );
      
   Data outData(outBuf,size);
      
   DebugLog( << "uncodec body is <" << outData << ">" );
      
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
   //DebugLog( << "uncode body = <" << text.escaped() << ">" );
   //DebugLog( << "uncode body size = " << text.size() );
   BIO* in = BIO_new_mem_buf( (void*)text.c_str(),text.size());
   assert(in);
   InfoLog( << "ceated in BIO");
    
   BIO* out;
   out = BIO_new(BIO_s_mem());
   assert(out);
   InfoLog( << "created out BIO" );

   PKCS7* pkcs7 = d2i_PKCS7_bio(in, NULL);
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
           
      return NULL;
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
   
   //if ( !verifySig )
   {
      flags |= PKCS7_NOVERIFY;
   }
   
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
            return NULL;
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
           
            return NULL;
         }
      }
      break;
      
      default:
         ErrLog(<< "Got PKCS7 data that could not be handled type=" << type );
         return NULL;
   }
      
   BIO_flush(out);
   char* outBuf=NULL;
   long size = BIO_get_mem_data(out,&outBuf);
   assert( size >= 0 );
   
   Data outData(outBuf,size);
   
   DebugLog( << "uncoded body is <" << outData << ">" );

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
