#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/TlsConnection.hxx"
#include "resiprocate/Security.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/os/Socket.hxx"

#if defined(USE_SSL)
#include <openssl/e_os2.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/pkcs7.h>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>
#endif

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

TlsConnection::TlsConnection( const Tuple& tuple, Socket fd, Security* security, 
                              bool server, Data domain,  SecurityTypes::SSLType sslType ) :
   Connection(tuple, fd),
   mServer(server),
   mSecurity(security),
   mSslType( sslType ),
   mDomain(domain)
{
#if defined(USE_SSL)
   DebugLog (<< "Creating TLS connection " << tuple << " on " << fd);

   mSsl = NULL;
   mPeerName = Data::Empty;
   mBio= NULL;
  
   if (mServer)
   {
      DebugLog( << "Trying to form TLS connection - acting as server" );
      if ( mDomain.empty() )
      {
         ErrLog(<< "Tranport was not created with a server domain so can not act as server" ); 
         throw Security::Exception("Trying to act as server but no domain specified",
                                   __FILE__,__LINE__);
      }
   }
   else
   {
      DebugLog( << "Trying to form TLS connection - acting as client" );
   }

   assert( mSecurity );
   SSL_CTX* ctx=NULL;
   if ( mSslType ==  SecurityTypes::SSLv23 )
   {
      ctx = mSecurity->getSslCtx();
   }
   else
   {
      ctx = mSecurity->getTlsCtx();
   }   
   assert(ctx);

   mSsl = SSL_new(ctx);
   assert(mSsl);

   if ( mServer )
   {
      assert( mSecurity );

      X509* cert = mSecurity->getDomainCert(mDomain); //mDomainCerts[mDomain];
      if (!cert)
      {
         ErrLog(<< "Don't have certificate for domain " << mDomain );
      }
      
      if( !SSL_use_certificate(mSsl, cert) )
      {
         throw Security::Exception("SSL_use_certificate failed",
                                   __FILE__,__LINE__);
      }
      
      EVP_PKEY* pKey = mSecurity->getDomainKey(mDomain); //mDomainPrivateKeys[mDomain];
      if (!pKey)
      {
         ErrLog(<< "Don't have private key for domain " << mDomain );
      }
      if ( !SSL_use_PrivateKey(mSsl, pKey) )
      {
         throw Security::Exception("SSL_use_PrivateKey failed.",
                                   __FILE__,__LINE__);
      }
   }
   
   mBio = BIO_new_socket(fd, 0/*close flag*/);
   assert( mBio );
   
   SSL_set_bio( mSsl, mBio, mBio );

   mState = mServer ? Accepting : Connecting;

#endif // USE_SSL   
}

TlsConnection::~TlsConnection()
{
   SSL_shutdown(mSsl);
   SSL_free(mSsl);
}


const char*
TlsConnection::fromState(TlsConnection::State s)
{
   switch(s)
   {
      case Handshaking: return "Handshaking"; break;
      case Accepting: return "Accepting"; break;
      case Broken: return "Broken"; break;
      case Connecting: return "Connecting"; break;
      case Up: return "Up"; break;
   }
   return "????";
}

TlsConnection::State
TlsConnection::checkState()
{
#if defined(USE_SSL)
   //DebugLog(<<"state is " << fromState(mState));

   if (mState == Up || mState == Broken)
   {
      return mState;
   }
   
   int ok=0;
   
   ERR_clear_error();
   
   if (mState != Handshaking)
   {
      if (mState == Accepting)
      {
         ok = SSL_accept(mSsl);
      }
      else
      {
         ok = SSL_connect(mSsl);
      }

      if ( ok <= 0 )
      {
         int err = SSL_get_error(mSsl,ok);
         char buf[256];
         ERR_error_string_n(err,buf,sizeof(buf));
         StackLog( << "TLS error in " 
                   << ( (mState == Accepting) ? "accept" : "connect" )
                   << " ok=" << ok << " err=" << err << " " << buf );
          
         switch (err)
         {
         case SSL_ERROR_WANT_READ:
            StackLog( << "TLS connection want read" );
            return mState;
         case SSL_ERROR_WANT_WRITE:
            StackLog( << "TLS connection want write" );
            return mState;
         case SSL_ERROR_WANT_CONNECT:
            StackLog( << "TLS connection want connect" );
            return mState;
#if  ( OPENSSL_VERSION_NUMBER >= 0x0090702fL )
         case SSL_ERROR_WANT_ACCEPT:
            StackLog( << "TLS connection want accept" );
            return mState;
#endif
         }
	   
         ErrLog( << "TLS connection failed "
                 << "ok=" << ok << " err=" << err << " " << buf );
         
         switch (err)
         {
            case SSL_ERROR_NONE: 
               ErrLog( <<" (SSL Error none)" );
               break;
            case SSL_ERROR_SSL: 
               ErrLog( <<" (SSL Error ssl)" );
               break;
            case SSL_ERROR_WANT_READ: 
               ErrLog( <<" (SSL Error want read)" ); 
               break;
            case SSL_ERROR_WANT_WRITE: 
               ErrLog( <<" (SSL Error want write)" ); 
               break;
            case SSL_ERROR_WANT_X509_LOOKUP: 
               ErrLog( <<" (SSL Error want x509 lookup)" ); 
               break;
            case SSL_ERROR_SYSCALL: 
               ErrLog( <<" (SSL Error want syscall)" ); 
               ErrLog( <<"Error may be because trying ssl connection to tls server" ); 
               break;
            case SSL_ERROR_WANT_CONNECT: 
               ErrLog( <<" (SSL Error want connect)" ); 
               break;
#if ( OPENSSL_VERSION_NUMBER >= 0x0090702fL )
            case SSL_ERROR_WANT_ACCEPT: 
               ErrLog( <<" (SSL Error want accept)" ); 
               break;
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
            InfoLog( << "Error code = " 
                     << code << " file=" << file << " line=" << line );
         }
         
         mState = Broken;
         mBio = 0;
         ErrLog (<< "Couldn't TLS connect");
         return mState;
      }
      
      InfoLog( << "TLS connected" ); 
      mState = Handshaking;
   }

   InfoLog( << "TLS handshake starting" ); 

   ok = SSL_do_handshake(mSsl);
      
   if ( ok <= 0 )
   {
      int err = SSL_get_error(mSsl,ok);
      char buf[256];
      ERR_error_string_n(err,buf,sizeof(buf));
         
      switch (err)
      {
         case SSL_ERROR_WANT_READ:
            DebugLog( << "TLS handshake want read" );
            return mState;
         case SSL_ERROR_WANT_WRITE:
            DebugLog( << "TLS handshake want write" );
            return mState;
         default:
            ErrLog( << "TLS handshake failed "
                    << "ok=" << ok << " err=" << err << " " << buf );
            mBio = NULL;
            mState = Broken;
            return mState;
      }
   }

   //post-connection verification: check that certificate name matches domain name
   if (mSecurity->requireServerAuthentication())
   {
      X509* cert = SSL_get_peer_certificate(mSsl);
      if (cert)
      {
         if (!mSecurity->compareCertName(cert, who().getTargetDomain()))
         {
            mState = Broken;
            mBio = 0;
            ErrLog (<< "Certificate name mismatch ");
            return mState;
         }
         X509_free(cert); 
         cert=NULL;
      }
      else
      {
         //!dcm! -- add 'require mutual tls' logic here
         if (!mServer)
         {
            ErrLog(<< "No server certificate in TLS connection" );
            mState = Broken;
            mBio = 0;
            return mState;
         }
      }
   }

   InfoLog( << "TLS handshake done" ); 
   mState = Up;

   // force peer name to get checked and perhaps cert loaded
   computePeerName();

#endif // USE_SSL   
   return mState;
}

      
int 
TlsConnection::read(char* buf, int count )
{
#if defined(USE_SSL)
   assert( mSsl ); 
   assert( buf );

   switch(checkState())
   {
      case Broken:
         return -1;
         break;
      case Up:
         break;
      default:
         return 0;
         break;
   }

   if (!mBio)
   {
      DebugLog( << "Got TLS read bad bio  " );
      return 0;
   }
      
   if ( !isGood() )
   {
      return -1;
   }
   
   int bytesRead = SSL_read(mSsl,buf,count);
   if (bytesRead <= 0 )
   {
      int err = SSL_get_error(mSsl,bytesRead);
      switch (err)
      {
         case SSL_ERROR_WANT_READ:
         case SSL_ERROR_WANT_WRITE:
         case SSL_ERROR_NONE:
         {
            StackLog( << "Got TLS read got condition of " << err  );
            return 0;
         }
         break;
         default:
         {
            char buf[256];
            ERR_error_string_n(err,buf,sizeof(buf));
            ErrLog( << "Got TLS read ret=" << bytesRead << " error=" << err  << " " << buf  );
            return -1;
         }
         break;
      }
      assert(0);
   }
   StackLog(<<"SSL bytesRead="<<bytesRead);
   return bytesRead;
#endif // USE_SSL
   return -1;
}


int 
TlsConnection::write( const char* buf, int count )
{
#if defined(USE_SSL)
   assert( mSsl );
   assert( buf );
   int ret;
 
   switch(checkState())
   {
      case Broken:
         return -1;
         break;
      case Up:
         break;
      default:
         return 0;
         break;
   }

   if (!mBio)
   {
      DebugLog( << "Got TLS write bad bio "  );
      return 0;
   }
        
   ret = SSL_write(mSsl,(const char*)buf,count);
   if (ret < 0 )
   {
      int err = SSL_get_error(mSsl,ret);
      switch (err)
      {
         case SSL_ERROR_WANT_READ:
         case SSL_ERROR_WANT_WRITE:
         case SSL_ERROR_NONE:
         {
            StackLog( << "Got TLS write got condition of " << err  );
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
            return -1;
         }
         break;
      }
   }

   StackLog( << "Did TLS write"  );

   return ret;
#endif // USE_SSL
   return -1;
}


bool 
TlsConnection::hasDataToRead() // has data that can be read 
{
#if defined(USE_SSL)
   if (checkState() != Up)
   {
      return false;
   }

   int p = SSL_pending(mSsl);
   //DebugLog(<<"hasDataToRead(): "<<p);
   return (p>0);
#else // not USE_SSL
   return false;
#endif 
}


bool 
TlsConnection::isGood() // has data that can be read 
{
#if defined(USE_SSL)
   if ( mBio == 0 )
   {
      return false;
   }

   int mode = SSL_get_shutdown(mSsl);
   if ( mode != 0 ) 
   {
      return false;
   }

#endif       
   return true;
}

void
TlsConnection::computePeerName()
{
#if defined(USE_SSL)
   assert(mSsl);
   
   if (checkState() != Up)
   {
      return;
   }

   if (!mBio)
   {
      ErrLog( << "bad bio" );
      return;
   }

   // print session infor       
   SSL_CIPHER *ciph;
   ciph=SSL_get_current_cipher(mSsl);
   InfoLog( << "TLS sessions set up with " 
            <<  SSL_get_version(mSsl) << " "
            <<  SSL_CIPHER_get_version(ciph) << " "
            <<  SSL_CIPHER_get_name(ciph) << " " );

   // get the certificate if other side has one 
   X509* cert = SSL_get_peer_certificate(mSsl);
   if ( !cert )
   {
      DebugLog(<< "No peer certificate in TLS connection" );
      return;
   }

   // TODO - check that this certificate is valid 

   // look at the Common Name to fine the peerName of the cert 
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
      Data name(d,l);
      DebugLog( << "got x509 string type=" << t << " len="<< l << " data=" << d );
      assert( name.size() == (unsigned)l );
      
      DebugLog( << "Found common name in cert of " << name );
      
      mPeerName = name;
   }

#if 0  // junk code to print certificates extentions for debugging 
   int numExt = X509_get_ext_count(cert);
   ErrLog(<< "Got peer certificate with " << numExt << " extentions" );

   for ( int i=0; i<numExt; i++ )
   {
      X509_EXTENSION* ext = X509_get_ext(cert,i);
      assert( ext );
      
      const char* str = OBJ_nid2sn(OBJ_obj2nid(X509_EXTENSION_get_object(ext)));
      assert(str);
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
         ASN1_IA5STRING* uri = gen->d.uniformResourceIdentifier;
         int l = uri->length;
         unsigned char* dat = uri->data;
         Data name(dat,l);
         InfoLog(<< "subjectAltName of TLS seesion cert contains <" << name << ">" );
         
         mPeerName = name;
      }
          
      if (gen->type == GEN_EMAIL)
      {
         DebugLog(<< "subjectAltName of cert has EMAIL type" );
      }
          
      if(gen->type == GEN_URI) 
      {
         DebugLog(<< "subjectAltName of cert has GEN_URI type" );
      }
   }
   sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);

   // add the certificate to the Security store
   assert( mPeerName != Data::Empty );
   if ( !mSecurity->hasDomainCert( mPeerName ) )
   {
      unsigned char* buf = NULL;
      int len = i2d_X509( cert, &buf );
      Data derCert( buf, len );
      mSecurity->addDomainCertDER(mPeerName,derCert);
      OPENSSL_free(buf); buf=NULL;
   }

   X509_free(cert); cert=NULL;
#endif // USE_SSL
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
