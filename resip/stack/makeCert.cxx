#include "precompile.h"
#include <openssl/ssl.h>
#include <openssl/pem.h>
#include <openssl/ossl_typ.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include "resip/stack/X509Contents.hxx"
#include "resip/stack/Pkcs8Contents.hxx"
#include "resip/stack/MultipartMixedContents.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Random.hxx"

using namespace resip;

int makeSelfCert(X509** selfcert, EVP_PKEY* privkey);

int main()
{
   int err;
   Uri aor;
   Data passphrase;
   RSA *rsa = NULL;
   EVP_PKEY *privkey = NULL;
   X509 *selfcert = NULL;
   BUF_MEM *bptr = NULL;

   // initilization:  are these needed?
//   CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ON);
//   bio_err=BIO_new_fp(stderr, BIO_NOCLOSE);
 
   Random::initialize();

   rsa = RSA_generate_key(1024, RSA_F4, NULL, NULL);
   assert(rsa);    // couldn't make key pair

   EVP_PKEY_assign_RSA(privkey, rsa);
   assert(privkey);

   selfcert = X509_new();
   err = makeSelfCert(&selfcert, privkey);
   assert(!err);   // couldn't make cert
   
   unsigned char* buffer = NULL;     
   int len = i2d_X509(selfcert, &buffer);   // if buffer is NULL, openssl
                                  // assigns memory for buffer
   assert(buffer);
   Data derData((char *) buffer, len);
   X509Contents *certpart = new X509Contents( derData );
   assert(certpart);
    
   // make an in-memory BIO        [ see  BIO_s_mem(3) ]
   BIO *mbio = BIO_new(BIO_s_mem());

   // encrypt the the private key with the passphrase and put it in the BIO in DER format
   i2d_PKCS8PrivateKey_bio( mbio, privkey, EVP_des_ede3_cbc(), 
      (char *) passphrase.data(), 
      passphrase.size(), NULL, NULL);

   // dump the BIO into a Contents
   BIO_get_mem_ptr(mbio, &bptr);
   Pkcs8Contents *keypart = new Pkcs8Contents(Data(bptr->data, bptr->length));
   assert(keypart);
   BIO_free(mbio);

   MultipartMixedContents *certsbody = new MultipartMixedContents;
   certsbody->parts().push_back(certpart);
   certsbody->parts().push_back(keypart);
   assert(certsbody);
}


int makeSelfCert(X509 **cert, EVP_PKEY *privkey)   // should include a Uri type at the end of the function call
{
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
  
  X509_set_version(selfcert, 2L);	// set version to X509v3 (starts from 0)

  //  RAND_bytes((char *) serial , 4);
  //serial = 1;
  serial = Random::getRandom();  // get an int worth of randomness
  ASN1_INTEGER_set(X509_get_serialNumber(selfcert),serial);

  X509_NAME_add_entry_by_txt( subject, "O",  MBSTRING_UTF8, (unsigned char *) domain.data(), domain.size(), -1, 0);
  X509_NAME_add_entry_by_txt( subject, "CN", MBSTRING_UTF8, (unsigned char *) userAtDomain.data(), userAtDomain.size(), -1, 0);

  X509_set_issuer_name(selfcert, subject);
  X509_set_subject_name(selfcert, subject);

  X509_gmtime_adj(X509_get_notBefore(selfcert),0);
  X509_gmtime_adj(X509_get_notAfter(selfcert), duration);

  X509_set_pubkey(selfcert, privkey);

  // need to fiddle with this to make this work with lists of IA5 URIs and UTF8
  //ext = X509V3_EXT_conf_nid( NULL , NULL , NID_subject_alt_name, subjectAltNameStr.cstr() );
  //X509_add_ext( selfcert, ext, -1);
  //X509_EXTENSION_free(ext);

  ext = X509V3_EXT_conf_nid(NULL, NULL, NID_basic_constraints, "CA:FALSE");
  X509_add_ext( selfcert, ext, -1);
  X509_EXTENSION_free(ext);

  // add extensions NID_subject_key_identifier and NID_authority_key_identifier

  X509_sign(selfcert, privkey, EVP_sha1());

  return true; 
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
