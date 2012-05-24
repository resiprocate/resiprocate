#include "CreateCert.hxx"

#include <openssl/x509.h>
#include <openssl/x509v3.h>


#include <string>
#include <rutil/Random.hxx>

using namespace std;
using namespace dtls;
using namespace resip;

int dtls::createCert (const resip::Data& pAor, int expireDays, int keyLen, X509*& outCert, EVP_PKEY*& outKey )
{
   int ret;
   
   cerr << "Generating new user cert for " << pAor << endl;
   Data aor = "sip:" + pAor;
   
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
   
//    ret = X509_NAME_add_entry_by_txt( subject, "O",  MBSTRING_ASC, 
//                                      (unsigned char *) domain.data(), domain.size(), 
//                                      -1, 0);
   assert(ret);
   ret = X509_NAME_add_entry_by_txt( subject, "CN", MBSTRING_ASC, 
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
   
   static char CA_FALSE[] = "CA:FALSE";
   ext = X509V3_EXT_conf_nid(NULL, NULL, NID_basic_constraints, CA_FALSE);
   ret = X509_add_ext( cert, ext, -1);
   assert(ret);
   X509_EXTENSION_free(ext);
   
   // TODO add extensions NID_subject_key_identifier and NID_authority_key_identifier
   
   ret = X509_sign(cert, privkey, EVP_sha1());
   assert(ret);

   outCert = cert;
   outKey = privkey;
   return ret; 
}


/* ====================================================================

 Copyright (c) 2007-2008, Eric Rescorla and Derek MacDonald 
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:
 
 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 
 
 3. None of the contributors names may be used to endorse or promote 
    products derived from this software without specific prior written 
    permission. 
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
