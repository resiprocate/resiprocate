#include <openssl/pem.h>
#include <openssl/ossl_typ.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include "resiprocate/X509Contents.hxx"
#include "resiprocate/Pkcs8Contents.hxx"
#include "resiprocate/MultipartMixedContents.hxx"
#include "resiprocate/Uri.hxx"

using namespace resip;



int main()
{
   int err;
   Uri aor;
   Data passphrase;
   RSA *rsa;
   EVP_PKEY *privkey;
   X509 *selfcert;
   unsigned char *buffer;
   BUF_MEM *bptr;

   rsa = RSA_generate_key(1024, RSA_F4, NULL, NULL);
   assert(rsa);    // couldn't make key pair

   EVP_PKEY_assign_RSA(privkey, rsa);
   assert(privkey);

//   err = makeSelfCert(selfcert, privkey);
   assert(!err);   // couldn't make cert
   
   i2d_X509(selfcert, &buffer);
   assert(buffer);   
   X509Contents *certpart = new X509Contents(Data(buffer, strlen(buffer)));
   assert(certpart);
    
   // make an in-memory BIO        [ see  BIO_s_mem(3) ]
   BIO *mbio = BIO_new(BIO_s_mem());

   // encrypt the the private key with the passphrase and put it in the BIO in DER format
   i2d_PKCS8PrivateKey_bio( mbio, privkey, EVP_des_ede3_cbc, passphrase.data(), 
      passphrase.length(), NULL, NULL);

   // dump the BIO into a Contents
   BIO_get_mem_ptr(mbio, &bptr);
   Pkcs8Contents *keypart = new Pkcs8Contents(Data(bptr->data, bptr->length));
   assert(keypart);
   BIO_free(mbio);

   MultipartMixedContents *certsbody = new MultipartMixedContents;
   certsbody->parts().push_back(certpart);
   certsbody->parts().push_back(keypart);
}  assert(certsbody);



int makeSelfCert(X509 *selfcert, EVP_PKEY *privkey)   // should include a Uri type at the end of the function call
{
  int serial;
  assert(sizeof(int)==4);
  const long duration = 60*60*24*30   // make cert valid for 30 days
  X509_NAME *subject;

  Data domain = new Data("example.org");
  Data userAtDomain = new Data("user@example.org");

  // Setup the subjectAltName structure here with sip:, im:, and pres: URIs
  // TODO:

  X509_set_version(selfcert, 2L);	// set version to X509v3 (starts from 0)

//  RAND_bytes((char *) serial , 4);
serial = 1;
//  X509_set_serialNumber(selfcert, (ASN1_INTEGER) serial);
  ASN1_INTEGER_set(X509_get_serialNumber(selfcert),serial);

  X509_NAME_add_entry_by_txt( subject, "O",  MBSTRING_UTF8, domain.data(), domain.length(), -1, 0);
  X509_NAME_add_entry_by_txt( subject, "CN", MBSTRING_UTF8, userAtDomain.data(), userAtDomain.length(), -1, 0);

  X509_set_issuer_name(selfcert, subject);
  X509_set_subject_name(selfcert, subject);

  X509_gmtime_adj(X509_get_notBefore(selfcert),0);
  X509_gmtime_adj(X509_get_notAfter(selfcert), duration);

  X509_set_pubkey(selfcert, privkey);

  // need to fiddle with this to make this work with lists of IA5 URIs and UTF8
  //ext = X509V3_EXT_conf_nid( NULL , NULL , NID_subject_alt_name, subjectAltNameStr.cstr() );
  //X509_add_ext( selfcert, ext, -1);
  //X509_EXTENSION_free(ext);

  ext = X509V3_EXT_conf_nid(NULL, NULL, NID_basic_contraints, "CA:FALSE");
  X509_add_ext( selfcert, ext, -1);
  X509_EXTENSION_free(ext);

  // add extensions NID_subject_key_identifier and NID_authority_key_identifier

  X509_sign(selfcert, privkey, EVP_sha1()); 
}

