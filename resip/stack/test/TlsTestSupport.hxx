#if !defined(RESIP_TLSTESTSUPPORT_HXX)
#define RESIP_TLSTESTSUPPORT_HXX

// Helpers for TLS unit tests: certificates generated in memory, and a pair of
// loopback TlsTransports driven by hand.  Uses only the OpenSSL 1.1.1 API.

#ifdef USE_SSL

#include <iostream>
#include <memory>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/TransportFailure.hxx"
#include "resip/stack/ssl/TlsTransport.hxx"
#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Timer.hxx"

namespace TlsTest
{

/// An EC P-256 key, or null on failure
inline EVP_PKEY*
makeKey()
{
   EVP_PKEY* key = nullptr;
   EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
   if (ctx &&
       EVP_PKEY_keygen_init(ctx) == 1 &&
       EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_X9_62_prime256v1) == 1)
   {
      EVP_PKEY_keygen(ctx, &key);
   }
   EVP_PKEY_CTX_free(ctx);
   return key;
}

inline bool
addExtension(X509* cert, X509V3_CTX* v3ctx, int nid, const char* value)
{
   X509_EXTENSION* ext = X509V3_EXT_conf_nid(nullptr, v3ctx, nid, value);
   if (!ext) return false;
   const bool added = X509_add_ext(cert, ext, -1) == 1;
   X509_EXTENSION_free(ext);
   return added;
}

/// A CA certificate when issuer is null, otherwise an end entity certificate
/// signed by issuer.  commonName may be null or empty for a certificate without
/// one.  subjectAltNames is in OpenSSL's config syntax, for example
/// "DNS:a.test, URI:sip:a.test"; null or empty for none.  Returns null on failure.
inline X509*
makeCert(EVP_PKEY* key, const char* commonName, const char* subjectAltNames,
         X509* issuer, EVP_PKEY* issuerKey)
{
   static long serial = 1;
   X509* cert = X509_new();
   X509_set_version(cert, 2L);
   ASN1_INTEGER_set(X509_get_serialNumber(cert), serial++);
   X509_gmtime_adj(X509_getm_notBefore(cert), -60 * 60);
   X509_gmtime_adj(X509_getm_notAfter(cert), 60 * 60 * 24);
   X509_set_pubkey(cert, key);
   if (commonName && *commonName)
   {
      X509_NAME_add_entry_by_txt(X509_get_subject_name(cert), "CN", MBSTRING_ASC,
                                 (const unsigned char*)commonName, -1, -1, 0);
   }
   X509_set_issuer_name(cert, X509_get_subject_name(issuer ? issuer : cert));

   X509V3_CTX v3ctx;
   X509V3_set_ctx_nodb(&v3ctx);
   X509V3_set_ctx(&v3ctx, issuer ? issuer : cert, cert, nullptr, nullptr, 0);
   bool ok = addExtension(cert, &v3ctx, NID_subject_key_identifier, "hash");
   if (issuer)
   {
      ok = ok &&
           addExtension(cert, &v3ctx, NID_basic_constraints, "critical,CA:FALSE") &&
           addExtension(cert, &v3ctx, NID_key_usage, "critical,digitalSignature") &&
           addExtension(cert, &v3ctx, NID_ext_key_usage, "serverAuth,clientAuth") &&
           addExtension(cert, &v3ctx, NID_authority_key_identifier, "keyid:always");
   }
   else
   {
      ok = ok &&
           addExtension(cert, &v3ctx, NID_basic_constraints, "critical,CA:TRUE") &&
           addExtension(cert, &v3ctx, NID_key_usage, "critical,keyCertSign,cRLSign");
   }
   if (ok && subjectAltNames && *subjectAltNames)
   {
      ok = addExtension(cert, &v3ctx, NID_subject_alt_name, subjectAltNames);
   }

   if (!ok || X509_sign(cert, issuerKey ? issuerKey : key, EVP_sha256()) <= 0)
   {
      X509_free(cert);
      return nullptr;
   }
   return cert;
}

inline resip::Data
toPem(X509* cert)
{
   BIO* bio = BIO_new(BIO_s_mem());
   PEM_write_bio_X509(bio, cert);
   char* pem = nullptr;
   const long len = BIO_get_mem_data(bio, &pem);
   resip::Data result(pem, (resip::Data::size_type)len);
   BIO_free(bio);
   return result;
}

inline bool
writeCertFile(const char* file, X509* cert)
{
   BIO* bio = BIO_new_file(file, "w");
   if (!bio) return false;
   const bool ok = PEM_write_bio_X509(bio, cert) == 1;
   BIO_free(bio);
   return ok;
}

inline bool
writeKeyFile(const char* file, EVP_PKEY* key)
{
   BIO* bio = BIO_new_file(file, "w");
   if (!bio) return false;
   const bool ok = PEM_write_bio_PrivateKey(bio, key, nullptr, nullptr, 0, nullptr, nullptr) == 1;
   BIO_free(bio);
   return ok;
}

/// An encoded OPTIONS request to targetHost, sent from clientPort
inline resip::Data
makeRequest(const resip::Data& targetHost, int clientPort, int serverPort)
{
   resip::NameAddr target;
   target.uri().scheme() = "sip";
   target.uri().user() = "server";
   target.uri().host() = targetHost;
   target.uri().port() = serverPort;
   target.uri().param(resip::p_transport) = "tls";

   resip::NameAddr from;
   from.uri().scheme() = "sip";
   from.uri().user() = "client";
   from.uri().host() = "127.0.0.1";
   from.uri().port() = clientPort;

   std::unique_ptr<resip::SipMessage> msg(resip::Helper::makeRequest(target, from, resip::OPTIONS));
   msg->header(resip::h_Vias).front().transport() = resip::Tuple::toData(resip::TLS);
   msg->header(resip::h_Vias).front().sentHost() = "127.0.0.1";
   msg->header(resip::h_Vias).front().sentPort() = clientPort;

   resip::Data encoded;
   {
      resip::DataStream strm(encoded);
      msg->encode(strm);
   }
   return encoded;
}

/// Runs both transports until the server has received a SIP message, the
/// client has reported a transport failure, or a few seconds have passed.
/// Returns the message, or null if none arrived.
inline std::unique_ptr<resip::SipMessage>
receive(resip::TlsTransport& client, resip::Fifo<resip::TransactionMessage>& clientFifo,
        resip::TlsTransport& server, resip::Fifo<resip::TransactionMessage>& serverFifo)
{
   const uint64_t deadline = resip::Timer::getTimeMs() + 5000;
   while (resip::Timer::getTimeMs() < deadline)
   {
      resip::FdSet fdset;
      server.buildFdSet(fdset);
      client.buildFdSet(fdset);
      fdset.selectMilliSeconds(10);
      server.process(fdset);
      client.process(fdset);

      while (clientFifo.messageAvailable())
      {
         std::unique_ptr<resip::TransactionMessage> msg(clientFifo.getNext());
         if (resip::TransportFailure* failure = dynamic_cast<resip::TransportFailure*>(msg.get()))
         {
            std::cerr << "     client transport failure: " << *failure << std::endl;
            return nullptr;
         }
      }
      while (serverFifo.messageAvailable())
      {
         std::unique_ptr<resip::TransactionMessage> msg(serverFifo.getNext());
         if (dynamic_cast<resip::SipMessage*>(msg.get()))
         {
            return std::unique_ptr<resip::SipMessage>(static_cast<resip::SipMessage*>(msg.release()));
         }
      }
   }
   std::cerr << "     timed out waiting for the request" << std::endl;
   return nullptr;
}

}

#endif // USE_SSL

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
 *
 */
