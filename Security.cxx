#if defined(USE_SSL)

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/pkcs7.h>

#include "sip2/sipstack/SipStack.hxx"
#include "sip2/sipstack/Security.hxx"
#include "sip2/sipstack/Contents.hxx"
#include "sip2/sipstack/Pkcs7Contents.hxx"
#include "sip2/sipstack/PlainContents.hxx"
#include "sip2/util/Logger.hxx"
#include "sip2/util/Random.hxx"
#include "sip2/util/DataStream.hxx"


using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::SIP



Security::Security()
{
   privateKey = NULL;
   publicCert = NULL;
   certAuthorities = NULL;
   
   static bool initDone=false;
   if ( !initDone )
   {
      initDone = true;
      
      OpenSSL_add_all_algorithms();
      ERR_load_crypto_strings();

      Random::initialize();
   }
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
      assert(0);
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
   
   DebugLog( << "Loaded public cert");
   
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
   
   DebugLog( << "Loaded public CAs");

   return true;
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
   
   DebugLog( << "Loaded private key");
   
   return true;
}



Pkcs7Contents* 
Security::sign( Contents* bodyIn )
{
   assert( bodyIn );
   
   Data bodyData;
   oDataStream strm(bodyData);
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
   
   PKCS7* pkcs7 = PKCS7_sign( publicCert, privateKey, chain, in, 0 /*flags*/);
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
  
   // DebugLog( << "Signed body is <" << outData.hex() << ">" );

   Pkcs7Contents* outBody = new Pkcs7Contents( outData );
   assert( outBody );

   return outBody;
}


Contents* 
Security::uncode( Pkcs7Contents* sBody )
{
   // for now, assume that this is only a singed message 
   
   assert( sBody );
   
   const char* p = sBody->text().data();
   int   s = sBody->text().size();
   
   BIO* in;
   in = BIO_new_mem_buf( (void*)p,s);
   assert(in);
   DebugLog( << "ceated in BIO");
    
   BIO* out;
   out = BIO_new(BIO_s_mem());
   assert(out);
   DebugLog( << "created out BIO" );

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
 
#if 0
   STACK_OF(PKCS7_SIGNER_INFO)* sk=PKCS7_get_signer_info(pkcs7);
   if (sk == NULL)
   {
      ErrLog( << "No signer info" );
   }  
   for (int i=0; i<sk_PKCS7_SIGNER_INFO_num(sk); i++)
   {
      PKCS7_SIGNER_INFO* si=sk_PKCS7_SIGNER_INFO_value(sk,i);
      
      STACK_OF(X509_ATTRIBUTE)* aa = PKCS7_get_signed_attributes(si);
      for ( int j=0; j<sk_X509_ATTRIBUTE_num(aa); j++)
      {
         X509_ATTRIBUTE* a = sk_X509_ATTRIBUTE_value(aa,j);

         InfoLog( << "set = " << a->set );
         if ( a->set == 0 )
         {
            ASN1_TYPE* t = a->value.single;
            int ti = ASN1_TYPE_get(t);
            InfoLog( << "type = " << ti );
         }
         if ( a->set == 1 )
         {
            STACK_OF(ASN1_TYPE)* sa = a->value.set;
            
            for (int k=0; k<sk_ASN1_TYPE_num(sa); k++)
            {
               ASN1_TYPE* at = sk_ASN1_TYPE_value(sa,k);
               
               int ti = ASN1_TYPE_get(at);
               InfoLog( << "type = " << ti ); 
            }
            
         }
      }
   }
#endif
    
/*  
 	STACK_OF(GENERAL_NAME) *gens;
	STACK *ret;
	gens = X509_get_ext_d2i(x, NID_subject_alt_name, NULL, NULL);
	ret = get_email(X509_get_subject_name(x), gens);
	sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);

     X509_NAME* name = 	X509_get_subject_name(X509 *a);
     	
      STACK *emlst;
      emlst = X509_get1_email(x);
      for (j = 0; j < sk_num(emlst); j++)
         BIO_printf(STDout, "%s\n", sk_value(emlst, j));
      X509_email_free(emlst);

      InfoLog( << name->bytes );
*/
   
   STACK_OF(X509)* certs;
   certs = sk_X509_new_null();
   assert( certs );
   //sk_X509_push(certs, publicCert);
   
   int flags = 0;
   //flags |=  PKCS7_NOVERIFY;
   
   assert( certAuthorities );
   
   if ( PKCS7_verify(pkcs7, certs, certAuthorities, pkcs7Bio, out, flags ) != 1 )
   {
      ErrLog( << "Problems doing PKCS7_verify" );
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
      
      return NULL;
   }

   BIO_flush(out);
   char* outBuf=NULL;
   long size = BIO_get_mem_data(out,&outBuf);
   assert( size >= 0 );
   
   Data outData(outBuf,size);
   
   DebugLog( << "uncodec body is <" << outData << ">" );

   PlainContents* outBody = new PlainContents( outData );
   assert( outBody );

   return outBody;  
}

     

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0  *  * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved. *  * Redistribution and use in source and binary forms, with or without * modification, are permitted provided that the following conditions * are met: *  * 1. Redistributions of source code must retain the above copyright *    notice, this list of conditions and the following disclaimer. *  * 2. Redistributions in binary form must reproduce the above copyright *    notice, this list of conditions and the following disclaimer in *    the documentation and/or other materials provided with the *    distribution. *  * 3. The names "VOCAL", "Vovida Open Communication Application Library", *    and "Vovida Open Communication Application Library (VOCAL)" must *    not be used to endorse or promote products derived from this *    software without prior written permission. For written *    permission, please contact vocal@vovida.org. * * 4. Products derived from this software may not be called "VOCAL", nor *    may "VOCAL" appear in their name, without prior written *    permission of Vovida Networks, Inc. *  * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL, * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH * DAMAGE. *  * ==================================================================== *  * This software consists of voluntary contributions made by Vovida * Networks, Inc. and many individuals on behalf of Vovida Networks, * Inc.  For more information on Vovida Networks, Inc., please see * <http://www.vovida.org/>. *
 */
