#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>

#ifdef USE_SSL

// (resip's headers first: they include <windows.h>, whose wincrypt.h defines X509_NAME as a
// macro - which OpenSSL's headers undefine, if they come after it)
#include "resip/stack/ssl/Security.hxx"
#include "rutil/Data.hxx"
#include "rutil/Log.hxx"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

using namespace resip;
using namespace std;

// Checks the keys and signatures BaseSecurity makes: generateUserCert's RSA key and
// certificate, and the RFC 4474 identity signature computeIdentity makes and
// checkIdentity verifies (RSASSA-PKCS1-v1_5 with SHA-256).

static int failures = 0;

static void
check(const char* what, bool cond)
{
   cerr << (cond ? "ok   " : "FAIL ") << what << endl;
   if (!cond) ++failures;
}

static EVP_PKEY*
makeKey(int type)
{
   EVP_PKEY* key = nullptr;
   EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(type, nullptr);
   bool ok = ctx && EVP_PKEY_keygen_init(ctx) == 1 &&
             (type == EVP_PKEY_RSA ? EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) == 1
                                   : EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_X9_62_prime256v1) == 1) &&
             EVP_PKEY_keygen(ctx, &key) == 1;
   EVP_PKEY_CTX_free(ctx);
   return ok ? key : nullptr;
}

static X509*
makeSelfSignedCert(EVP_PKEY* key, const char* cn)
{
   X509* cert = X509_new();
   X509_set_version(cert, 2L);
   ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
   X509_gmtime_adj(X509_getm_notBefore(cert), -60);
   X509_gmtime_adj(X509_getm_notAfter(cert), 60 * 60);
   X509_set_pubkey(cert, key);
   // (The name is built, then set: OpenSSL 4's X509_get_subject_name returns it const)
   X509_NAME* subject = X509_NAME_new();
   X509_NAME_add_entry_by_txt(subject, "CN", MBSTRING_ASC, (const unsigned char*)cn, -1, -1, 0);
   X509_set_subject_name(cert, subject);
   X509_NAME_free(subject);
   X509_set_issuer_name(cert, X509_get_subject_name(cert));
   if (X509_sign(cert, key, EVP_sha256()) <= 0)
   {
      X509_free(cert);
      return nullptr;
   }
   return cert;
}

static Data
toPem(X509* cert, EVP_PKEY* key)
{
   BIO* bio = BIO_new(BIO_s_mem());
   if (cert) PEM_write_bio_X509(bio, cert);
   if (key) PEM_write_bio_PrivateKey(bio, key, nullptr, nullptr, 0, nullptr, nullptr);
   char* p = nullptr;
   long len = BIO_get_mem_data(bio, &p);
   Data pem(p, (Data::size_type)len);
   BIO_free(bio);
   return pem;
}

// Verifies an identity signature independently of BaseSecurity: RSASSA-PKCS1-v1_5 with
// SHA-256 over the whole input, as RSA_sign produced before computeIdentity used EVP
static bool
independentlyVerified(X509* cert, const Data& in, const Data& sigBase64)
{
   Data sig = sigBase64.base64decode();
   EVP_PKEY* key = X509_get_pubkey(cert);
   EVP_MD_CTX* ctx = EVP_MD_CTX_new();
   EVP_PKEY_CTX* pctx = nullptr;
   bool ok = key && ctx &&
             EVP_DigestVerifyInit(ctx, &pctx, EVP_sha256(), nullptr, key) == 1 &&
             EVP_PKEY_CTX_set_rsa_padding(pctx, RSA_PKCS1_PADDING) == 1 &&
             EVP_DigestVerify(ctx, (const unsigned char*)sig.data(), sig.size(),
                              (const unsigned char*)in.data(), in.size()) == 1;
   EVP_MD_CTX_free(ctx);
   EVP_PKEY_free(key);
   return ok;
}

int
main(int, char**)
{
   Log::initialize(Log::Cout, Log::None, "testSecurityKeys");

   // generateUserCert.  It also tries to write the certificate and key to the Security's
   // directory, which is harmless if that doesn't exist: the failure is only logged.
   {
      Security security;
      const Data aor("keys@example.org");
      security.generateUserCert(aor);
      X509* cert = security.hasUserCert(aor) ? security.getUserCert(aor) : nullptr;
      EVP_PKEY* key = security.hasUserPrivateKey(aor) ? security.getUserPrivateKey(aor) : nullptr;
      check("generateUserCert: made a certificate and key", cert && key);
      if (cert && key)
      {
         check("generateUserCert: a 2048 bit RSA key by default",
               EVP_PKEY_base_id(key) == EVP_PKEY_RSA && EVP_PKEY_bits(key) == 2048);
         check("generateUserCert: the key is the certificate's", X509_check_private_key(cert, key) == 1);
         check("generateUserCert: signed with SHA-256", X509_get_signature_nid(cert) == NID_sha256WithRSAEncryption);
      }

      const Data aor3072("keys3072@example.org");
      security.generateUserCert(aor3072, 1, 3072);
      EVP_PKEY* key3072 = security.hasUserPrivateKey(aor3072) ? security.getUserPrivateKey(aor3072) : nullptr;
      check("generateUserCert: a key of the size asked for", key3072 && EVP_PKEY_bits(key3072) == 3072);
   }

   // computeIdentity / checkIdentity
   EVP_PKEY* rsaKey = makeKey(EVP_PKEY_RSA);
   EVP_PKEY* ecKey = makeKey(EVP_PKEY_EC);
   X509* rsaCert = rsaKey ? makeSelfSignedCert(rsaKey, "example.org") : nullptr;
   X509* ecCert = ecKey ? makeSelfSignedCert(ecKey, "ec.example.org") : nullptr;
   check("test keys and certificates could be created", rsaCert && ecCert);
   if (rsaCert && ecCert)
   {
      Security security;
      security.addDomainCertPEM("example.org", toPem(rsaCert, nullptr));
      security.addDomainPrivateKeyPEM("example.org", toPem(nullptr, rsaKey));

      const Data in("sip:alice@example.org:sip:bob@example.org:a84b4c76e66710:INVITE:Thu, 21 Feb 2002 13:02:03 GMT");
      Data sig = security.computeIdentity("example.org", in);
      check("identity: signed", !sig.empty());
      check("identity: the signature verifies", security.checkIdentity("example.org", in, sig));
      check("identity: the signature is RSASSA-PKCS1-v1_5 with SHA-256", independentlyVerified(rsaCert, in, sig));
      check("identity: a changed input doesn't verify", !security.checkIdentity("example.org", in + "x", sig));

      Data raw = sig.base64decode();
      raw[raw.size() / 2] ^= 0x01;
      check("identity: a changed signature doesn't verify", !security.checkIdentity("example.org", in, raw.base64encode()));

      check("identity: a certificate without an RSA key doesn't verify (no assert)",
            !security.checkIdentity("example.org", in, sig, ecCert));

      security.addDomainPrivateKeyPEM("ec.example.org", toPem(nullptr, ecKey));
      bool threw = false;
      try
      {
         security.computeIdentity("ec.example.org", in);
      }
      catch (BaseSecurity::Exception&)
      {
         threw = true;
      }
      check("identity: signing with a key that isn't RSA throws", threw);
   }
   X509_free(rsaCert);
   X509_free(ecCert);
   EVP_PKEY_free(rsaKey);
   EVP_PKEY_free(ecKey);

   cerr << (failures == 0 ? "\nall checks passed\n" : "\nFAILURES\n");
   return failures == 0 ? 0 : -1;
}

#else // USE_SSL

int
main(int, char**)
{
   return 0;
}

#endif // USE_SSL

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
 *
 */
