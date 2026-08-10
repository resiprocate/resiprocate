#include <cstdlib>
#include <iostream>

#include <openssl/asn1.h>
#include <openssl/bn.h>
#include <openssl/x509.h>

#include "resip/stack/ssl/Security.hxx"
#include "rutil/Data.hxx"
#include "rutil/Log.hxx"

using namespace resip;
using namespace std;

// RFC 5280 section 4.1.2.2: "The serial number MUST be a positive integer."
//
// The serial is drawn with Random::getCryptoRandom(), which fills a whole int
// from RAND_bytes and is therefore negative about half the time, where the
// getRandom() it replaced was always positive. ASN1_INTEGER_set() stores a
// negative value as a negative INTEGER, so the certificate carries one.

static int failures = 0;

static void
check(const char* what, bool cond)
{
   cerr << (cond ? "ok   " : "FAIL ") << what << endl;
   if (!cond) ++failures;
}

// true when the serial of this DER encoded certificate is negative
static bool
serialIsNegative(const Data& der)
{
   const unsigned char* p = (const unsigned char*)der.data();
   X509* cert = d2i_X509(0, &p, (long)der.size());
   if (!cert) return false;
   BIGNUM* bn = ASN1_INTEGER_to_BN(X509_get_serialNumber(cert), 0);
   const bool negative = bn && BN_is_negative(bn);
   if (bn) BN_free(bn);
   X509_free(cert);
   return negative;
}

int
main(int, char**)
{
   Log::initialize(Log::Cout, Log::None, "testCertSerial");

   Security security(BaseSecurity::StrongestSuite);

   // The check has to be able to see a negative serial, otherwise "none were
   // negative" below would also be the answer for a certificate the probe
   // simply failed to read.
   //
   // The certificate for this has to be built from scratch rather than parsed
   // and edited: i2d_X509() re-emits the DER it cached while parsing, so a
   // serial written afterwards never reaches the encoding and the control would
   // silently be looking at the original value instead of the one it set.
   {
      bool sawIt = false;
      bool builtOne = false;

      EVP_PKEY* key = EVP_PKEY_new();
      RSA* rsa = RSA_new();
      BIGNUM* e = BN_new();
      BN_set_word(e, RSA_F4);
      if (RSA_generate_key_ex(rsa, 1024, e, 0) == 1 &&
          EVP_PKEY_set1_RSA(key, rsa) == 1)
      {
         X509* cert = X509_new();
         X509_set_version(cert, 2L);
         ASN1_INTEGER_set(X509_get_serialNumber(cert), -12345);
         X509_gmtime_adj(X509_getm_notBefore(cert), 0);
         X509_gmtime_adj(X509_getm_notAfter(cert), 60 * 60 * 24);
         X509_set_pubkey(cert, key);

         X509_NAME* name = X509_get_subject_name(cert);
         X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                                    (const unsigned char*)"control", -1, -1, 0);
         X509_set_issuer_name(cert, name);

         if (X509_sign(cert, key, EVP_sha256()) > 0)
         {
            unsigned char* buf = 0;
            const int len = i2d_X509(cert, &buf);
            if (len > 0)
            {
               builtOne = true;
               sawIt = serialIsNegative(Data((const char*)buf, len));
               OPENSSL_free(buf);
            }
         }
         X509_free(cert);
      }
      BN_free(e);
      RSA_free(rsa);
      EVP_PKEY_free(key);

      check("control: a certificate with a negative serial could be built", builtOne);
      check("control: a negative serial is detected", sawIt);
   }

   const int runs = 40;
   int negative = 0;
   for (int i = 0; i < runs; ++i)
   {
      Data aor("user");
      aor += Data(i);
      aor += "@example.org";

      security.generateUserCert(aor, 1 /* expireDays */, 1024 /* keyLen */);
      if (!security.hasUserCert(aor))
      {
         cerr << "     konnte kein Zertifikat erzeugen fuer " << aor << endl;
         continue;
      }
      if (serialIsNegative(security.getUserCertDER(aor)))
      {
         ++negative;
      }
   }

   cerr << "     " << negative << " von " << runs
        << " erzeugten Zertifikaten mit negativer Seriennummer" << endl;
   check("generateUserCert produces positive serials only", negative == 0);

   cerr << (failures == 0 ? "\nall checks passed\n" : "\nFAILURES\n");
   return failures == 0 ? 0 : -1;
}
