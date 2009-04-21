#include <openssl/ssl.h>
#include <openssl/pem.h>
#include <openssl/ossl_typ.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include "resip/stack/X509Contents.hxx"
#include "resip/stack/Pkcs8Contents.hxx"
#include "resip/stack/MultipartMixedContents.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Random.hxx"
#include "rutil/Logger.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

int makeSelfCert(X509** selfcert, EVP_PKEY* privkey);

int main(int argc, char* argv[])
{
   int stat;
   Uri aor;
   Data passphrase("password");
   RSA *rsa = NULL;
   EVP_PKEY *privkey = NULL;
   X509 *selfcert = NULL;
   BUF_MEM *bptr = NULL;
 
   Log::initialize(Log::Cerr, Log::Err, argv[0]);
   Log::setLevel(Log::Debug);
   SSL_library_init();
   SSL_load_error_strings();
   OpenSSL_add_all_algorithms();
   //OpenSSL_add_ssl_algorithms() is insufficient here...

   // make sure that necessary algorithms exist:
   assert(EVP_des_ede3_cbc());

   Random::initialize();

   rsa = RSA_generate_key(1024, RSA_F4, NULL, NULL);
   assert(rsa);    // couldn't make key pair
   
   // TODO: remove this once we've tested this
   stat = PEM_write_RSAPrivateKey( stdout, rsa, NULL, NULL, 0, NULL, NULL); // Write this out for debugging
      
   privkey = EVP_PKEY_new();
   assert(privkey);
   stat = EVP_PKEY_set1_RSA(privkey, rsa);
   assert(stat);

   selfcert = X509_new();
   assert(selfcert);
   stat = makeSelfCert(&selfcert, privkey);
   assert(stat);   // couldn't make cert
   
   unsigned char* buffer = NULL;     
   int len = i2d_X509(selfcert, &buffer);   // if buffer is NULL, openssl
											// assigns memory for buffer
   assert(buffer);
   Data derData((char *) buffer, len);
   X509Contents *certpart = new X509Contents( derData );
   assert(certpart);
   
//  TDOD: remove later, just useful for debugging
//   stat = PEM_write_PKCS8PrivateKey( stdout, privkey, NULL, NULL, 0, NULL, NULL);
	  
   // make an in-memory BIO        [ see  BIO_s_mem(3) ]
   BIO *mbio = BIO_new(BIO_s_mem());
   assert(mbio);

   // encrypt the the private key with the passphrase and put it in the BIO in DER format
   stat = i2d_PKCS8PrivateKey_bio( mbio, privkey, EVP_des_ede3_cbc(), 
      (char *) passphrase.data(), passphrase.size(), NULL, NULL);
   assert(stat);

   // dump the BIO into a Contents and free the BIO
   BIO_get_mem_ptr(mbio, &bptr);
   Pkcs8Contents *keypart = new Pkcs8Contents(Data(bptr->data, bptr->length));
   assert(keypart);
   BIO_free(mbio);

   // make the multipart body
   MultipartMixedContents *certsbody = new MultipartMixedContents;
   certsbody->parts().push_back(certpart);
   certsbody->parts().push_back(keypart);
   assert(certsbody);
   
   Data foo;
   DataStream foostr(foo);
   certsbody->encode(foostr);
   foostr.flush();
   
   DebugLog ( << foo );
}


int makeSelfCert(X509 **cert, EVP_PKEY *privkey)   // should include a Uri type at the end of the function call
{
  int stat;
  int serial;
  assert(sizeof(int)==4);
  const long duration = 60*60*24*30;   // make cert valid for 30 days
  X509* selfcert = NULL;
  X509_NAME *subject = NULL;
  X509_EXTENSION *ext = NULL;

  Data domain("example.org");
  Data userAtDomain("user@example.org");

  // Setup the subjectAltName structure here with sip:, im:, and pres: URIs
  // TODO:

  selfcert = *cert;
  subject = X509_NAME_new();
  ext = X509_EXTENSION_new();
  
  X509_set_version(selfcert, 2L);	// set version to X509v3 (starts from 0)

  //  RAND_bytes((char *) serial , 4);
  //serial = 1;
  serial = Random::getRandom();  // get an int worth of randomness
  ASN1_INTEGER_set(X509_get_serialNumber(selfcert),serial);

  stat = X509_NAME_add_entry_by_txt( subject, "O",  MBSTRING_UTF8, (unsigned char *) domain.data(), domain.size(), -1, 0);
  assert(stat);
  stat = X509_NAME_add_entry_by_txt( subject, "CN", MBSTRING_UTF8, (unsigned char *) userAtDomain.data(), userAtDomain.size(), -1, 0);
  assert(stat);
  
  stat = X509_set_issuer_name(selfcert, subject);
  assert(stat);
  stat = X509_set_subject_name(selfcert, subject);
  assert(stat);

  X509_gmtime_adj(X509_get_notBefore(selfcert),0);
  X509_gmtime_adj(X509_get_notAfter(selfcert), duration);

  stat = X509_set_pubkey(selfcert, privkey);
  assert(stat);

  // need to fiddle with this to make this work with lists of IA5 URIs and UTF8
  // using GENERAL_NAMES seems like a promissing approach
  // (search for GENERAL_NAMES in Security.cxx)
  //
  //ext = X509V3_EXT_conf_nid( NULL , NULL , NID_subject_alt_name, subjectAltNameStr.cstr() );
  //X509_add_ext( selfcert, ext, -1);
  //X509_EXTENSION_free(ext);

  static char CA_FALSE[] = "CA:FALSE";
  ext = X509V3_EXT_conf_nid(NULL, NULL, NID_basic_constraints, CA_FALSE);
  stat = X509_add_ext( selfcert, ext, -1);
  assert(stat);  
  X509_EXTENSION_free(ext);

  // add extensions NID_subject_key_identifier and NID_authority_key_identifier

  stat = X509_sign(selfcert, privkey, EVP_sha1());
  assert(stat);
   
  return true; 
}

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
