#if !defined(RETURN_TLSTESTSUPPORT_HXX)
#define RETURN_TLSTESTSUPPORT_HXX

// Certificates and checks shared by reTurn's TLS tests

#include <iostream>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

namespace TlsTestSupport
{

inline int& failures()
{
   static int count = 0;
   return count;
}

inline void
check(const char* what, bool cond)
{
   std::cerr << (cond ? "ok   " : "FAIL ") << what << std::endl;
   if (!cond) ++failures();
}

// An EC P-256 key
inline EVP_PKEY*
makeKey()
{
   EVP_PKEY* key = nullptr;
   EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
   bool ok = ctx && EVP_PKEY_keygen_init(ctx) == 1 &&
             EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_X9_62_prime256v1) == 1 &&
             EVP_PKEY_keygen(ctx, &key) == 1;
   EVP_PKEY_CTX_free(ctx);
   return ok ? key : nullptr;
}

// A certificate for key, issued by issuer (self-signed when issuerCert is null), with the given
// subjectAltName (eg. "DNS:localhost") unless that's null
inline X509*
makeCert(EVP_PKEY* key, const char* cn, const char* subjectAltName, X509* issuerCert, EVP_PKEY* issuerKey)
{
   X509* cert = X509_new();
   X509_set_version(cert, 2L);
   static long serial = 1;
   ASN1_INTEGER_set(X509_get_serialNumber(cert), serial++);
   X509_gmtime_adj(X509_getm_notBefore(cert), -60);
   X509_gmtime_adj(X509_getm_notAfter(cert), 60 * 60);
   X509_set_pubkey(cert, key);
   X509_NAME_add_entry_by_txt(X509_get_subject_name(cert), "CN", MBSTRING_ASC, (const unsigned char*)cn, -1, -1, 0);
   X509_set_issuer_name(cert, issuerCert ? X509_get_subject_name(issuerCert) : X509_get_subject_name(cert));

   X509V3_CTX ctx;
   X509V3_set_ctx(&ctx, issuerCert ? issuerCert : cert, cert, nullptr, nullptr, 0);
   X509_EXTENSION* bc = X509V3_EXT_conf_nid(nullptr, &ctx, NID_basic_constraints, issuerCert ? "CA:FALSE" : "critical,CA:TRUE");
   X509_add_ext(cert, bc, -1);
   X509_EXTENSION_free(bc);
   if (subjectAltName)
   {
      X509_EXTENSION* san = X509V3_EXT_conf_nid(nullptr, &ctx, NID_subject_alt_name, subjectAltName);
      X509_add_ext(cert, san, -1);
      X509_EXTENSION_free(san);
   }
   X509_sign(cert, issuerKey ? issuerKey : key, EVP_sha256());
   return cert;
}

// Writes a certificate and/or key to a PEM file
inline bool
writePem(const char* file, X509* cert, EVP_PKEY* key)
{
   // A BIO rather than a FILE*: on Windows OpenSSL can't use the application's C runtime FILEs
   BIO* bio = BIO_new_file(file, "w");
   if (!bio) return false;
   bool ok = (!cert || PEM_write_bio_X509(bio, cert) == 1) &&
             (!key || PEM_write_bio_PrivateKey(bio, key, nullptr, nullptr, 0, nullptr, nullptr) == 1);
   BIO_free(bio);
   return ok;
}

}

#endif

/* ====================================================================
 *
 * Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 */
