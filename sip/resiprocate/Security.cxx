#if defined(USE_SSL)

#include <sys/types.h>

#include <openssl/e_os2.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/pkcs7.h>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>

#ifndef WIN32
#include <dirent.h>
#endif

#include "sip2/util/Socket.hxx"

#include "sip2/sipstack/SipStack.hxx"
#include "sip2/sipstack/Security.hxx"
#include "sip2/sipstack/Contents.hxx"
#include "sip2/sipstack/Pkcs7Contents.hxx"
#include "sip2/sipstack/PlainContents.hxx"
#include "sip2/util/Random.hxx"
#include "sip2/util/DataStream.hxx"
#include "sip2/util/Logger.hxx"

using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::SIP


TlsConnection::TlsConnection( Security* security, Socket fd, bool server )
{
   ssl = NULL;
   
   assert( security );
   SSL_CTX* ctx = security->getTlsCtx();
   assert(ctx);
   
   ssl = SSL_new(ctx);
   assert(ssl);
   
   bio = BIO_new_socket(fd,0/*close flag*/);
   assert( bio );
   
   SSL_set_bio( ssl, bio, bio );

   int ok=0;
   if (server)
   {
      ok = SSL_accept(ssl);
   }
   else
   {
      ok = SSL_connect(ssl);
   }
   if ( ok != 1 )
   {
      int err = SSL_get_error(ssl,ok);
      char buf[256];
      ERR_error_string_n(err,buf,sizeof(buf));
      
      ErrLog( << "ssl connection failed with server=" << server << " err=" << err << " " << buf );
      
      bio = NULL;
   }
}

      
int 
TlsConnection::read( const void* buf, const int count )
{
   assert( ssl );
   assert( buf );
   int ret;

   if (!bio)
   {
      DebugLog( << "Got TLS read bad bio  " );
     return 0;
   }
      
   ret = SSL_read(ssl,(char*)buf,count);
   if (ret < 0 )
   {
      int err = SSL_get_error(ssl,ret);
      switch (err)
      {
         case SSL_ERROR_WANT_READ:
         case SSL_ERROR_WANT_WRITE:
         case SSL_ERROR_NONE:
         {
            DebugLog( << "Got TLS read got condition of " << err  );
            return 0;
         }
         break;
         default:
         {
            char buf[256];
            ERR_error_string_n(err,buf,sizeof(buf));
            ErrLog( << "Got TLS read error " << err  << " " << buf  );
            return 0;
         }
         break;
      }
   }

   return ret;
}


int 
TlsConnection::write( const void* buf, const int count )
{
   assert( ssl );
   assert( buf );
   int ret;
 
   if (!bio)
   {
      DebugLog( << "Got TLS write bad bio "  );
     return 0;
   }
        
   ret = SSL_write(ssl,(const char*)buf,count);
   if (ret < 0 )
   {
      int err = SSL_get_error(ssl,ret);
      switch (err)
      {
         case SSL_ERROR_WANT_READ:
         case SSL_ERROR_WANT_WRITE:
         case SSL_ERROR_NONE:
         {
            DebugLog( << "Got TLS write got codition of " << err  );
            return 0;
         }
         break;
         default:
         {
            ErrLog( << "Got TLS write error " << err  );
            return 0;
         }
         break;
      }
   }

   return ret;
}


bool 
TlsConnection::hasDataToRead() // has data that can be read 
{
   int p = SSL_pending(ssl);
   return (p>0);
}


bool 
TlsConnection::isGood() // has data that can be read 
{
   return (bio!=0);
}


Data 
TlsConnection::peerName()
{
   assert(ssl);
   Data ret = Data::Empty;

   if (!bio)
   {
      DebugLog( << "bad bio" );
      return Data::Empty;
   }
      
#ifdef WIN32
   assert(0);
#else
   ErrLog("request peer certificate" );
   X509* cert = SSL_get_peer_certificate(ssl);
   if ( !cert )
   {
      ErrLog("No peer certifiace in TLS connection" );
      return ret;
   }
   ErrLog("Got peer certificate" );

   X509_NAME* subject = X509_get_subject_name(cert);
   assert(subject);
   
   int i =-1;
   while( true )
   {
      i = X509_NAME_get_index_by_NID(subject, NID_commonName,i);
      if ( i == -1 )
      {
         break;
      }
      assert( i != -1 );
      X509_NAME_ENTRY* entry = X509_NAME_get_entry(subject,i);
      assert( entry );
      
      ASN1_STRING*	s = X509_NAME_ENTRY_get_data(entry);
      assert( s );
      
      int t = M_ASN1_STRING_type(s);
      int l = M_ASN1_STRING_length(s);
      unsigned char* d = M_ASN1_STRING_data(s);
      
      ErrLog( << "got string type=" << t << " len="<<l );
      ErrLog( << "data=<" << d << ">" );

      ret = Data(d);
   }
   
   STACK* sk = X509_get1_email(cert);
   if (sk)
   {
      ErrLog( << "Got an email" );
      
      for( int i=0; i<sk_num(sk); i++ )
      {
         char* v = sk_value(sk,i);
         ErrLog( << "Got an email value of " << v );

         ret = Data(v);
      }
   }
   
   int numExt = X509_get_ext_count(cert);
   ErrLog("Got peer certificate with " << numExt << " extentions" );

   for ( int i=0; i<numExt; i++ )
   {
      X509_EXTENSION* ext = X509_get_ext(cert,i);
      assert( ext );
      
      const char* str = OBJ_nid2sn(OBJ_obj2nid(X509_EXTENSION_get_object(ext)));
      assert(str);
      
      ErrLog("Got certificate extention" << str );
   }
   
   X509_free(cert); cert=NULL;
#endif
   return ret;
}


Security::Security()
{
   privateKey = NULL;
   publicCert = NULL;
   certAuthorities = NULL;
   ctx = NULL;
   
   static bool initDone=false;
   if ( !initDone )
   {
      initDone = true;
      
      SSL_library_init();
      
      SSL_load_error_strings();
            
      //OpenSSL_add_all_algorithms();
      OpenSSL_add_all_ciphers();
      OpenSSL_add_all_digests();

      ERR_load_crypto_strings();

      Random::initialize();
   }
}


SSL_CTX* 
Security::getTlsCtx()
{
   if ( ctx )
   {
      return ctx;
   }
   
   ctx=SSL_CTX_new( TLSv1_method() );
   assert( ctx );
   
   int ok;
   assert( publicCert );
   ok = SSL_CTX_use_certificate(ctx, publicCert);
   assert( ok == 1);
   
   assert( privateKey );
   ok = SSL_CTX_use_PrivateKey(ctx,privateKey);
   assert( ok == 1);
   
   assert( certAuthorities );
   SSL_CTX_set_cert_store(ctx, certAuthorities);

   return ctx;
}


Security::~Security()
{
}
  

Data 
Security::getPath( const Data& dirPath, const Data& file )
{
   Data path = dirPath;
   
   if ( path.empty() )
   {
#ifdef WIN32
  //      assert(0); 
	   // !cj! TODO need to fix
	   path = "C:\\.";
#else
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
            ErrLog( << "Environment variobal HOME is not set" );
            path = "/etc/sip";
         }
      }
#endif
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
Security::loadAllCerts( const Data& password, const Data&  dirPath )
{
   bool ok = true;
   ok = loadRootCerts( getPath( dirPath, Data("root.pem")) ) ? ok : false;
   ok = loadMyPublicCert( getPath( dirPath, Data("id.pem")) ) ? ok : false;
   ok = loadMyPrivateKey( password, getPath(dirPath,Data("id_key.pem") )) ? ok : false;

   if (ok)
   {
      getTlsCtx();
   }
   
   ok = loadPublicCert( getPath( dirPath, Data("public_keys/")) ) ? ok : false;
   
   return ok;
}
     

bool 
Security::loadMyPublicCert( const Data&  filePath )
{
   assert( !filePath.empty() );
   
   FILE* fp = fopen(filePath.c_str(),"r");
   if ( !fp )
   {
      ErrLog( << "Could not read public cert from " << filePath );
      return false;
   }
   
   publicCert = PEM_read_X509(fp,NULL,NULL,NULL);
   if (!publicCert)
   {
      ErrLog( << "Error reading contents of public cert file " << filePath );
      return false;
   }
   
   InfoLog( << "Loaded public cert from " << filePath );
   
   return true;
}


bool 
Security::loadRootCerts(  const Data& filePath )
{ 
   assert( !filePath.empty() );
   
   certAuthorities = X509_STORE_new();
   assert( certAuthorities );
   
   if ( X509_STORE_load_locations(certAuthorities,filePath.c_str(),NULL) != 1 )
   {  
      ErrLog( << "Error reading contents of root cert file " << filePath );
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
   assert(0);
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
      
      FILE* fp = fopen(path.c_str(),"r");
      if ( !fp )
      {
         ErrLog( << "Could not read public key from " << path );
         continue;
      }
   
      X509* cert = PEM_read_X509(fp,NULL,NULL,NULL);
      if (!cert)
      {
         ErrLog( << "Error reading contents of public key file " << path );
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
   
   FILE* fp = fopen(filePath.c_str(),"r");
   if ( !fp )
   {
      ErrLog( << "Could not read private key from " << filePath );
      return false;
   }
   
   //DebugLog( "password is " << password );
   
   privateKey = PEM_read_PrivateKey(fp,NULL,NULL,(void*)password.c_str());
   if (!privateKey)
   {
      ErrLog( << "Error reading contents of private key file " << filePath );

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
         DebugLog( << "Error code = " << code << " file=" << file << " line=" << line );
      }
      
      return false;
   }
   
   InfoLog( << "Loaded private key from << " << filePath );
   
   assert( privateKey );
   return true;
}



Pkcs7Contents* 
Security::sign( Contents* bodyIn )
{
   assert( bodyIn );

   int flags = 0;
   flags |= PKCS7_BINARY;
   
   Data bodyData;
   oDataStream strm(bodyData);
#if 1
   strm << "Content-Type: " << bodyIn->getType() << Symbols::CRLF;
   strm << Symbols::CRLF;
#endif
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

#if 0
   if ( SMIME_write_PKCS7(out,pkcs7,in,0) != 1 )
   {
      ErrLog( << "Error doind S/MIME write of signed object" );
      return NULL;
   }
   DebugLog( << "created SMIME write object" );
#else
   i2d_PKCS7_bio(out,pkcs7);
#endif
   BIO_flush(out);
   
   char* outBuf=NULL;
   long size = BIO_get_mem_data(out,&outBuf);
   assert( size > 0 );
   
   Data outData(outBuf,size);
  
   InfoLog( << "Signed body size is <" << outData.size() << ">" );
   //InfoLog( << "Signed body is <" << outData.escaped() << ">" );

   Pkcs7Contents* outBody = new Pkcs7Contents( outData );
   assert( outBody );

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
#if 1
   strm << "Content-Type: " << bodyIn->getType() << Symbols::CRLF;
   strm << Symbols::CRLF;
#endif
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

#if 0
   if ( SMIME_write_PKCS7(out,pkcs7,in,0) != 1 )
   {
      ErrLog( << "Error doind S/MIME write of signed object" );
      return NULL;
   }
   DebugLog( << "created SMIME write object" );
#else
   i2d_PKCS7_bio(out,pkcs7);
#endif
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

   return outBody;
   
}


Contents* 
Security::uncode( Pkcs7Contents* sBody, Data* signedBy, SignatureStatus* sigStat, bool* encrypted )
{
   SignatureStatus localSigStat;
   if (!sigStat) sigStat=&localSigStat;
   bool localEncyped;
   if (!encrypted) encrypted=&localEncyped;
   Data localSignedby;
   if (!signedBy) signedBy=&localSignedby;

   *encrypted = false;
   *sigStat = none;
   *signedBy = Data::Empty;
   
   InfoLog( << "Calling first layer of Security:uncode");
   Contents* outBody = uncodeSingle( sBody, true, signedBy, sigStat, encrypted );
   if ( (!outBody) && (*sigStat==isBad ) )
   {
      InfoLog( << "Retry first layer of Security:uncode");
      outBody = uncodeSingle( sBody, false, signedBy, sigStat, encrypted ); 
   }
   
   Pkcs7Contents* recuriveBody = dynamic_cast<Pkcs7Contents*>( outBody );
   if ( recuriveBody )
   {
      InfoLog( << "Calling Second layer of Security:uncode");

      outBody = uncodeSingle( recuriveBody, true, signedBy, sigStat, encrypted );
      if ( (!outBody) && (*sigStat==isBad ) )
      {
         InfoLog( << "Retry Second layer of Security:uncode");
         outBody = uncodeSingle( recuriveBody, false, signedBy, sigStat, encrypted ); 
      }
      
      delete recuriveBody;
   }
   
   return outBody;
}


Contents* 
Security::uncodeSingle( Pkcs7Contents* sBody, bool verifySig,  
                        Data* signedBy, SignatureStatus* sigStatus, bool* encrypted )
{
   int flags=0;
   flags |= PKCS7_BINARY;
   
   // for now, assume that this is only a singed message
   assert( sBody );
   
   //const char* p = sBody->text().data();
   const char* p = sBody->text().c_str();
   int   s = sBody->text().size();
   //InfoLog( << "uncode body = " << sBody->text().escaped() );
   InfoLog( << "uncode body size = " << s );
   
   BIO* in;
   in = BIO_new_mem_buf( (void*)p,s);
   assert(in);
   InfoLog( << "ceated in BIO");
    
   BIO* out;
   out = BIO_new(BIO_s_mem());
   assert(out);
   InfoLog( << "created out BIO" );

#if 0
   BIO* pkcs7Bio=NULL;
   PKCS7* pkcs7 = SMIME_read_PKCS7(in,&pkcs7Bio);
   if ( !pkcs7 )
   {
      ErrLog( << "Problems doing SMIME_read_PKCS7" );
      return NULL;
   }
   if ( pkcs7Bio )
   {
      ErrLog( << "Can not deal with mutlipart mime version stuff " );
      return NULL;
   }  
#else
   BIO* pkcs7Bio=NULL;
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
#endif
   
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
         InfoLog("email field of signing cert is <" << e << ">" );
         if ( signedBy)
         {
            *signedBy = Data(e);
         }
      }
   }

   STACK_OF(X509)* certs;
   certs = sk_X509_new_null();
   assert( certs );
   
   if ( !verifySig )
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
         if ( encrypted )
         {
            *encrypted = true;
         }
      }
      break;
      
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
            
            return NULL;
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
         ErrLog("Got PKCS7 data that could not be handled type=" << type );
         return NULL;
   }
      
   BIO_flush(out);
   char* outBuf=NULL;
   long size = BIO_get_mem_data(out,&outBuf);
   assert( size >= 0 );
   
   Data outData(outBuf,size);
   
   DebugLog( << "uncodec body is <" << outData << ">" );

   // parse out the header information and form new body.
   // !jf! this is a really crappy parser - shoudl do proper mime stuff
   ParseBuffer pb( outData.data(), outData.size() );

   //const char* start = pb.position();
   
   pb.skipToChar(Symbols::COLON[0]);
   pb.skipChar();
   
   Mime mime;
   mime.parse(pb);

   //InfoLog( << "location after mime parese  <" << Data(start,pb.position()-start) << ">" );

   const char* anchor = pb.skipToChars(Symbols::CRLFCRLF);
   if ( anchor )
   {
      anchor += 4;
   }
      
   //InfoLog( << "location after CRLF parese  <" << Data(start,pb.position()-start) << ">" );

   if ( Contents::getFactoryMap().find(mime) == Contents::getFactoryMap().end())
   {
      ErrLog( << "Don't know how to deal with MIME type " << mime );
      return NULL;
   }
   pb.skipToEnd();

   // InfoLog( << "uncodec body is <" << outData << ">" );
   //InfoLog( << "uncodec data is <" << Data(anchor,pb.position()-anchor) << ">" );
   //InfoLog( << "uncodec data szie is <" << pb.position()-anchor << ">" );

   Data tmp;
   pb.data(tmp, anchor);
   Contents* ret = Contents::createContents(mime, tmp);
   assert( ret );
   
   //InfoLog( << "uncode return type is " << ret->getType() );
 
 
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
