#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/TlsConnection.hxx"
#include "resiprocate/Security.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Socket.hxx"

#include <openssl/e_os2.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/pkcs7.h>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

TlsConnection::TlsConnection( const Tuple& tuple, Socket fd, Security* security, bool server )
{
   DebugLog (<< "Creating TLS connection " << tuple << " on " << fd);

   ssl = NULL;

   if (server)
   {
      DebugLog( << "Trying to from TLS connection - acting as server" );
   }
   else
   {
      DebugLog( << "Trying to form TLS connection - acting as client" );
   }

   assert( security );
   SSL_CTX* ctx = security->getTlsCtx(server);
   assert(ctx);
   
   ssl = SSL_new(ctx);
   assert(ssl);
   
   bio = BIO_new_socket(fd,0/*close flag*/);
   assert( bio );
   
   SSL_set_bio( ssl, bio, bio );

   int ok=0;

   bool again=true;
   
   while (again)
   {
      again = false;
      
      if (server)
      {
         ok = SSL_accept(ssl);
      }
      else
      {
         ok = SSL_connect(ssl);
      }
      if ( ok <= 0 )
      {
         int err = SSL_get_error(ssl,ok);
         char buf[256];
         ERR_error_string_n(err,buf,sizeof(buf));
         
         switch (err)
         {
            case SSL_ERROR_WANT_READ:
               DebugLog( << "TLS connection want read" );
               again = true;
               break;
            case SSL_ERROR_WANT_WRITE:
               DebugLog( << "TLS connection want write" );
               again = true;
               break;
            case SSL_ERROR_WANT_CONNECT:
               DebugLog( << "TLS connection want connect" );
               again = true;
               break;
#if 0
            case SSL_ERROR_WANT_ACCEPT:
               DebugLog( << "TLS connection want accept" );
               again = true;
               break;
#endif
            default:
               ErrLog( << "TLS connection failed "
                       << "ok=" << ok << " err=" << err << " " << buf );

               switch (err)
               {
                  case SSL_ERROR_NONE: ErrLog( <<" (SSL Error none)" ); break;
                  case SSL_ERROR_SSL: ErrLog( <<" (SSL Error ssl)" ); break;
                  case SSL_ERROR_WANT_READ: ErrLog( <<" (SSL Error want read)" ); break;
                  case SSL_ERROR_WANT_WRITE: ErrLog( <<" (SSL Error want write)" ); break;
                  case SSL_ERROR_WANT_X509_LOOKUP: ErrLog( <<" (SSL Error want x509 lookup)" ); break;
                  case SSL_ERROR_SYSCALL: 
                     ErrLog( <<" (SSL Error want syscall)" ); 
                     ErrLog( <<"Error may be because trying ssl connection to tls server" ); 
                     break;
                  case SSL_ERROR_WANT_CONNECT: ErrLog( <<" (SSL Error want connect)" ); break;
#if ( OPENSSL_VERSION_NUMBER >= 0x0090702fL )
                     case SSL_ERROR_WANT_ACCEPT: ErrLog( <<" (SSL Error want accpet)" ); break;
#endif
               }
               
                     
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
                  DebugLog( << "Error code = " 
                            << code << " file=" << file << " line=" << line );
               }
                     
               bio = NULL;
               throw Transport::Exception( Data("TLS connect failed"), __FILE__, __LINE__ );   
         }
      }
   }
   
   InfoLog( << "TLS connected" ); 

#if 1  
   InfoLog( << "TLS handshake starting" ); 
   again = true;
   while (again)
   {
      again = false;
      ok = SSL_do_handshake(ssl);
      
      if ( ok <= 0 )
      {
         int err = SSL_get_error(ssl,ok);
         char buf[256];
         ERR_error_string_n(err,buf,sizeof(buf));
         
         switch (err)
         {
            case SSL_ERROR_WANT_READ:
               DebugLog( << "TLS handshake want read" );
               again = true;
               break;
            case SSL_ERROR_WANT_WRITE:
               DebugLog( << "TLS handshake want read" );
               again = true;
               break;
            default:
               ErrLog( << "TLS handshake failed "
                       << "ok=" << ok << " err=" << err << " " << buf );
               
               bio = NULL;
               
               throw Transport::Exception( Data("TLS handshake failed"), __FILE__, __LINE__ );   
         }
      }
   }
   
   InfoLog( << "TLS handshake done" ); 
#endif

}

      
int 
TlsConnection::read(char* buf, int count )
{
   assert( ssl ); 
   assert( buf );

   if (!bio)
   {
      DebugLog( << "Got TLS read bad bio  " );
      return 0;
   }
      
   int bytesRead = SSL_read(ssl,buf,count);
   if (bytesRead < 0 )
   {
      int err = SSL_get_error(ssl,bytesRead);
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
            ErrLog( << "Got TLS read ret=" << bytesRead << " error=" << err  << " " << buf  );
            return 0;
         }
         break;
      }
   }

   return bytesRead;
}


int 
TlsConnection::write( const char* buf, int count )
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
            DebugLog( << "Got TLS write got condition of " << err  );
            return 0;
         }
         break;
         default:
         {
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
            ErrLog( << "Got TLS write error=" << err << " ret=" << ret  );
            return 0;
         }
         break;
      }
   }

   DebugLog( << "Did TLS write"  );

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

#if 1 // print session infor       
   SSL_CIPHER *ciph;
   
   ciph=SSL_get_current_cipher(ssl);
   InfoLog( << "TLS sessions set up with " 
            <<  SSL_get_version(ssl) << " "
            <<  SSL_CIPHER_get_version(ciph) << " "
            <<  SSL_CIPHER_get_name(ciph) << " " );
#endif

#ifdef WIN32
   assert(0);
#else
   ErrLog(<< "request peer certificate" );
   X509* cert = SSL_get_peer_certificate(ssl);
   if ( !cert )
   {
      ErrLog(<< "No peer certifiace in TLS connection" );
      return ret;
   }
   ErrLog(<< "Got peer certificate" );

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
   ErrLog(<< "Got peer certificate with " << numExt << " extentions" );

   for ( int i=0; i<numExt; i++ )
   {
      X509_EXTENSION* ext = X509_get_ext(cert,i);
      assert( ext );
      
      const char* str = OBJ_nid2sn(OBJ_obj2nid(X509_EXTENSION_get_object(ext)));
      assert(str);
      
      ErrLog(<< "Got certificate extention" << str );
   }
   
   X509_free(cert); cert=NULL;
#endif
   return ret;
}

