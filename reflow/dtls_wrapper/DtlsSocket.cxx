#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_SSL

#include <iostream>
#ifdef WIN32
#include <winsock2.h>  // struct timeval
#else
#include <sys/time.h>
#endif
#include "rutil/ResipAssert.h"
#include <ctype.h>
#include <string.h>

#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"
#include "bf_dwrap.hxx"

#include <utility>

using namespace std;
using namespace dtls;

// Our local timers
class dtls::DtlsSocketTimer : public DtlsTimer
{
  public:
     DtlsSocketTimer(unsigned int seq,DtlsSocket *socket): DtlsTimer(seq),mSocket(socket){}
     ~DtlsSocketTimer()
      {
      }
     
      void expired()
      {
         mSocket->expired(this);        
      }
  private:
     DtlsSocket *mSocket;
};

int dummy_cb(int d, X509_STORE_CTX *x)
{
   return 1;
}

DtlsSocket::DtlsSocket(std::unique_ptr<DtlsSocketContext> socketContext, DtlsFactory* factory, enum SocketType type):
   mSocketContext(std::move(socketContext)),
   mFactory(factory),
   mReadTimer(0),
   mSsl(newSsl(factory, type)),
   mInBio(SSL_get_rbio(mSsl)),
   mOutBio(SSL_get_wbio(mSsl)),
   mSocketType(type),
   mHandshakeCompleted(false),
   mHandshakeFailed(false)
{
   mSocketContext->setDtlsSocket(this);
}

DtlsSocket::DtlsSocket(std::unique_ptr<DtlsSocketContext> socketContext, DtlsFactory* factory, SSL* acceptedSsl, const std::string& peer):
   mSocketContext(std::move(socketContext)),
   mFactory(factory),
   mReadTimer(0),
   mSsl(acceptedSsl),
   mInBio(SSL_get_rbio(acceptedSsl)),
   mOutBio(SSL_get_wbio(acceptedSsl)),
   mSocketType(Server),
   mHandshakeCompleted(false),
   mHandshakeFailed(false),
   mCookiePeer(peer)
{
   mSocketContext->setDtlsSocket(this);

   // DTLSv1_listen left the SSL doing cookie exchange, so the cookie callbacks may still be
   // asked to check one (eg. for a retransmitted ClientHello)
   SSL_set_ex_data(mSsl, DtlsFactory::cookiePeerIndex(), &mCookiePeer);
}

SSL*
DtlsSocket::newSsl(DtlsFactory* factory, SocketType type)
{
   resip_assert(factory->mContext);
   SSL* ssl=SSL_new(factory->mContext);
   resip_assert(ssl!=0);

   switch(type)
   {
   case Client:
      SSL_set_connect_state(ssl);
      break;
   case Server:
      SSL_set_accept_state(ssl);
      SSL_set_verify(ssl,SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, dummy_cb);
      break;
   default:
      resip_assert(0);
   }

   BIO* inBio=BIO_new(BIO_f_dwrap());
   BIO_push(inBio,BIO_new(BIO_s_mem()));

   BIO* outBio=BIO_new(BIO_f_dwrap());
   BIO_push(outBio,BIO_new(BIO_s_mem()));

   SSL_set_bio(ssl,inBio,outBio);
   return ssl;
}

DtlsSocket::~DtlsSocket()
{
   if(mReadTimer) mReadTimer->invalidate();

   // Free the SSL - note: this also free's the BIO's.  No SSL_shutdown(): the close_notify it
   // writes would only go to the memory BIO freed here, never to the peer, and before the
   // handshake has completed it just leaves an error on this thread's queue.
   SSL_free(mSsl);
}

void
DtlsSocket::expired(DtlsSocketTimer* timer)
{
   // The timer has fired, so nothing will fire it again: it's ours to delete.  (DtlsTimer::fire
   // doesn't touch the timer after calling expired().)
   if(timer == mReadTimer)
   {
      mReadTimer = 0;
   }
   delete timer;

   handleTimeout();
}

void
DtlsSocket::startClient()
{
   resip_assert(mSocketType == Client);

   doHandshakeIteration();
}

bool
DtlsSocket::handlePacketMaybe(const unsigned char* bytes, size_t len)
{
   DtlsFactory::PacketType pType=DtlsFactory::demuxPacket(bytes,len);

   if(pType!=DtlsFactory::dtls)
      return false;

   int r;
   BIO_reset(mInBio);
   BIO_reset(mOutBio);

   r=BIO_write(mInBio,bytes,(int)len);
   resip_assert(r==(int)len);  // Can't happen

   // Note: we must catch any below exceptions--if there are any
   if(mHandshakeCompleted)
   {
      handlePacketAfterHandshake();
   }
   else
   {
      doHandshakeIteration();
   }

   return true;
}

void
DtlsSocket::handleTimeout()
{
   if(mHandshakeCompleted || mHandshakeFailed)
      return;

   BIO_reset(mInBio);
   BIO_reset(mOutBio);

   // Retransmits the last flight if OpenSSL's timer has expired (doubling it each time), and
   // fails once OpenSSL has retransmitted as many times as it allows
   ERR_clear_error();
   if(DTLSv1_handle_timeout(mSsl) < 0)
   {
      char errbuf[1024];
      errbuf[0]=0;
      ERR_error_string_n(ERR_peek_error(),errbuf,sizeof(errbuf));
      mHandshakeFailed = true;
      mSocketContext->handshakeFailed(errbuf[0] ? errbuf : "DTLS handshake timed out");
      ERR_clear_error();
   }

   writeOutput();
   scheduleTimer();
}

void
DtlsSocket::handlePacketAfterHandshake()
{
   // No application data is expected over DTLS-SRTP, but reading processes what the peer sends
   // after the handshake: eg. its last flight again if it didn't get our final one, which OpenSSL
   // answers by retransmitting ours
   ERR_clear_error();
   unsigned char buffer[2048];
   while(SSL_read(mSsl, buffer, sizeof(buffer)) > 0)
   {
   }
   ERR_clear_error();

   writeOutput();
}

void
DtlsSocket::writeOutput()
{
   // If mOutBio is now nonzero-length, then we need to write the
   // data to the network. TODO: warning, MTU issues!
   unsigned char *outBioData;
   int outBioLen=BIO_get_mem_data(mOutBio,&outBioData);
   if(outBioLen)
   {
      mSocketContext->write(outBioData,outBioLen);
   }
}

void
DtlsSocket::scheduleTimer()
{
   // Replaces any timer set before: a timer that's been invalidated deletes itself when it fires
   if(mReadTimer)
   {
      mReadTimer->invalidate();
      mReadTimer = 0;
   }

   // OpenSSL runs the retransmission timer while the handshake is in progress (starting at 1
   // second and doubling); fire ours when it expires
   struct timeval tv;
   if(!mHandshakeCompleted && !mHandshakeFailed && DTLSv1_get_timeout(mSsl, &tv) == 1)
   {
      unsigned int waitMs = (unsigned int)tv.tv_sec * 1000 + ((unsigned int)tv.tv_usec + 999) / 1000;
      mReadTimer = new DtlsSocketTimer(0, this);
      mReadTimer->setLock(mSocketContext->timerLock());
      mFactory->mTimerContext->addTimer(mReadTimer, waitMs > 0 ? waitMs : 1);
   }
}

void
DtlsSocket::doHandshakeIteration()
{
   int r;
   char errbuf[1024];
   int sslerr;

   if(mHandshakeCompleted || mHandshakeFailed)
      return;

   // OpenSSL requires this thread's error queue to be empty before the call, since
   // SSL_get_error() checks it first (the handshake state machine currently clears it
   // itself, but that isn't documented)
   ERR_clear_error();
   r=SSL_do_handshake(mSsl);
   errbuf[0]=0;
   ERR_error_string_n(ERR_peek_error(),errbuf,sizeof(errbuf));

   // Now handle handshake errors */
   switch(sslerr=SSL_get_error(mSsl,r))
   {
   case SSL_ERROR_NONE:
      mHandshakeCompleted = true;
      mSocketContext->handshakeCompleted();
      break;
   case SSL_ERROR_WANT_READ:
      // Waiting for the peer's next flight (any we've just written goes out below)
      break;
   default:
      cerr << "SSL error " << sslerr << endl;

      mHandshakeFailed = true;
      mSocketContext->handshakeFailed(errbuf);
      // Don't leave this failure's errors on the queue: the next OpenSSL call on this thread
      // that checks SSL_get_error() without clearing it first would take them as its own
      ERR_clear_error();
      // Note: need to fall through to propagate alerts, if any
      break;
   }

   writeOutput();
   scheduleTimer();
}

static bool
equalsIgnoreCase(const char* a, const char* b)
{
   for(; *a && *b; a++, b++)
   {
      if(tolower((unsigned char)*a) != tolower((unsigned char)*b))
         return false;
   }
   return *a == *b;
}

// The RFC 8122 hash function textual names we accept: those of the IANA registry,
// less md2 and md5, which aren't safe to authenticate a certificate with
static const EVP_MD*
fingerprintDigest(const char* hashFunction)
{
   if(!hashFunction) return 0;
   if(equalsIgnoreCase(hashFunction, "sha-1")) return EVP_sha1();
   if(equalsIgnoreCase(hashFunction, "sha-224")) return EVP_sha224();
   if(equalsIgnoreCase(hashFunction, "sha-256")) return EVP_sha256();
   if(equalsIgnoreCase(hashFunction, "sha-384")) return EVP_sha384();
   if(equalsIgnoreCase(hashFunction, "sha-512")) return EVP_sha512();
   return 0;
}

bool
DtlsSocket::isFingerprintHashFunctionSupported(const char* hashFunction)
{
   return fingerprintDigest(hashFunction) != 0;
}

bool
DtlsSocket::getRemoteFingerprint(const char* hashFunction, char *fingerprint, size_t fingerprintSize)
{
   const EVP_MD* digest = fingerprintDigest(hashFunction);
   if(!digest)
      return false;

   X509 *x;
#if OPENSSL_VERSION_NUMBER < 0x30000000L
   x=SSL_get_peer_certificate(mSsl);
#else
   x=SSL_get1_peer_certificate(mSsl);
#endif
   if(!x) // No certificate
      return false;

   bool computed = computeFingerprint(x, digest, fingerprint, fingerprintSize);
   X509_free(x);
   return computed;
}

bool
DtlsSocket::checkFingerprint(const char* hashFunction, const char* fingerprint)
{
   char fprint[EVP_MAX_MD_SIZE * 3];

   // The whole fingerprint must match; its hex digits may be in either case (RFC 8122 section 5)
   return fingerprint &&
          getRemoteFingerprint(hashFunction, fprint, sizeof(fprint)) &&
          equalsIgnoreCase(fprint, fingerprint);
}

void
DtlsSocket::getMyCertFingerprint(char *fingerprint)
{
   mFactory->getMyCertFingerprint(fingerprint);
}

// Maps the SRTP protection profile DTLS negotiated to libsrtp's - returns false if there
// isn't one, or libsrtp doesn't support it (the AES-GCM profiles need libsrtp built with GCM)
static bool
negotiatedSrtpProfile(SRTP_PROTECTION_PROFILE* negotiated, srtp_profile_t& profile)
{
   if(!negotiated)
      return false;

   switch(negotiated->id)
   {
   case SRTP_AES128_CM_SHA1_80: profile = srtp_profile_aes128_cm_sha1_80; break;
   case SRTP_AES128_CM_SHA1_32: profile = srtp_profile_aes128_cm_sha1_32; break;
   case SRTP_AEAD_AES_128_GCM:  profile = srtp_profile_aead_aes_128_gcm;  break;
   case SRTP_AEAD_AES_256_GCM:  profile = srtp_profile_aead_aes_256_gcm;  break;
   default: return false;
   }

   srtp_crypto_policy_t policy;
   return srtp_crypto_policy_set_from_profile_for_rtp(&policy, profile) == srtp_err_status_ok &&
          srtp_crypto_policy_set_from_profile_for_rtcp(&policy, profile) == srtp_err_status_ok;
}

SrtpSessionKeys
DtlsSocket::getSrtpSessionKeys()
{
   //TODO: probably an exception candidate
   resip_assert(mHandshakeCompleted);

   SrtpSessionKeys keys;

   srtp_profile_t profile;
   if(!negotiatedSrtpProfile(getSrtpProfile(), profile))
   {
      return keys;
   }
   unsigned int keyLen = srtp_profile_get_master_key_length(profile);
   unsigned int saltLen = srtp_profile_get_master_salt_length(profile);

   // RFC 5764 section 4.2: client key, server key, client salt, server salt
   unsigned char material[2 * SRTP_MAX_KEY_LEN];
   size_t materialLen = 2 * (keyLen + saltLen);
   if(materialLen > sizeof(material) ||
      !SSL_export_keying_material(
      mSsl,
      material,
      materialLen,
      "EXTRACTOR-dtls_srtp", 19, NULL, 0, 0))
   {
      return keys;
   }

   size_t offset = 0;

   keys.clientMasterKey.assign(material + offset, material + offset + keyLen);
   offset += keys.clientMasterKey.size();
   keys.serverMasterKey.assign(material + offset, material + offset + keyLen);
   offset += keys.serverMasterKey.size();
   keys.clientMasterSalt.assign(material + offset, material + offset + saltLen);
   offset += keys.clientMasterSalt.size();
   keys.serverMasterSalt.assign(material + offset, material + offset + saltLen);
   offset += keys.serverMasterSalt.size();

   OPENSSL_cleanse(material, sizeof(material));
   return keys;
}

SRTP_PROTECTION_PROFILE*
DtlsSocket::getSrtpProfile()
{
   //TODO: probably an exception candidate
   resip_assert(mHandshakeCompleted);
   return SSL_get_selected_srtp_profile(mSsl);
}

void
DtlsSocket::computeFingerprint(X509 *cert, char *fingerprint)
{
   bool computed = computeFingerprint(cert, EVP_sha256(), fingerprint, FingerprintSize);
   resip_assert(computed);
}

bool
DtlsSocket::computeFingerprint(X509 *cert, const EVP_MD* digest, char *fingerprint, size_t fingerprintSize)
{
   unsigned char md[EVP_MAX_MD_SIZE];
   unsigned int n;

   if(X509_digest(cert,digest,md,&n) != 1 || n == 0 ||
      fingerprintSize < n * 3)  // 2 hex digits and a colon (or the terminating null) per byte
   {
      return false;
   }

   static const char hex[] = "0123456789ABCDEF";
   for(unsigned int i=0;i<n;i++)
   {
      *fingerprint++ = hex[md[i] >> 4];
      *fingerprint++ = hex[md[i] & 0x0f];
      *fingerprint++ = (i < n-1) ? ':' : 0;
   }
   return true;
}

// Fills in a policy for one direction's keys, from the negotiated profile
static void
initSrtpPolicy(srtp_policy_t& policy, srtp_profile_t profile, const std::vector<unsigned char>& key, const std::vector<unsigned char>& salt)
{
   memset(&policy, 0, sizeof(srtp_policy_t));
   policy.window_size = 128;
   policy.allow_repeat_tx = 1;

   uint8_t* keyAndSalt = new uint8_t[SRTP_MAX_KEY_LEN];
   memcpy(keyAndSalt, key.data(), key.size());
   memcpy(keyAndSalt + key.size(), salt.data(), salt.size());
   policy.key = keyAndSalt;

   // Can't fail: negotiatedSrtpProfile has checked libsrtp supports the profile
   srtp_crypto_policy_set_from_profile_for_rtp(&policy.rtp, profile);
   srtp_crypto_policy_set_from_profile_for_rtcp(&policy.rtcp, profile);
   policy.next = NULL;
}

bool
DtlsSocket::createSrtpSessionPolicies(srtp_policy_t& outboundPolicy, srtp_policy_t& inboundPolicy)
{
   resip_assert(mHandshakeCompleted);

   srtp_profile_t profile;
   if(!negotiatedSrtpProfile(getSrtpProfile(), profile))
   {
      return false;
   }

   SrtpSessionKeys srtp_key = getSrtpSessionKeys();
   if(srtp_key.clientMasterKey.empty())
   {
      return false;
   }

   srtp_policy_t client_policy;
   initSrtpPolicy(client_policy, profile, srtp_key.clientMasterKey, srtp_key.clientMasterSalt);
   srtp_policy_t server_policy;
   initSrtpPolicy(server_policy, profile, srtp_key.serverMasterKey, srtp_key.serverMasterSalt);

   if (mSocketType == Client)
   {
      client_policy.ssrc.type = ssrc_any_outbound;
      outboundPolicy = client_policy;

      server_policy.ssrc.type  = ssrc_any_inbound;
      inboundPolicy = server_policy;
   }
   else
   {
      server_policy.ssrc.type  = ssrc_any_outbound;
      outboundPolicy = server_policy;

      client_policy.ssrc.type = ssrc_any_inbound;
      inboundPolicy = client_policy;
   }

   OPENSSL_cleanse(srtp_key.clientMasterKey.data(), srtp_key.clientMasterKey.size());
   OPENSSL_cleanse(srtp_key.serverMasterKey.data(), srtp_key.serverMasterKey.size());
   OPENSSL_cleanse(srtp_key.clientMasterSalt.data(), srtp_key.clientMasterSalt.size());
   OPENSSL_cleanse(srtp_key.serverMasterSalt.data(), srtp_key.serverMasterSalt.size());
   return true;
}

DtlsListener::DtlsListener(DtlsFactory* factory) :
   mFactory(factory),
   mSsl(0)
{
}

DtlsListener::~DtlsListener()
{
   if(mSsl) SSL_free(mSsl);
}

DtlsSocket*
DtlsListener::handlePacket(const unsigned char* bytes, size_t len, const std::string& peer,
                           std::vector<unsigned char>& response,
                           const std::function<std::unique_ptr<DtlsSocketContext>()>& makeContext)
{
   response.clear();
   if(len == 0 || DtlsFactory::demuxPacket(bytes, len) != DtlsFactory::dtls)
   {
      return 0;
   }

   if(!mSsl)
   {
      mSsl = DtlsSocket::newSsl(mFactory, DtlsSocket::Server);
      SSL_set_ex_data(mSsl, DtlsFactory::cookiePeerIndex(), &mPeer);
   }
   mPeer = peer;

   BIO* inBio = SSL_get_rbio(mSsl);
   BIO* outBio = SSL_get_wbio(mSsl);
   BIO_reset(inBio);
   BIO_reset(outBio);
   int r = BIO_write(inBio, bytes, (int)len);
   resip_assert(r == (int)len);  // Can't happen

   // Answers a ClientHello without a valid cookie for peer with a HelloVerifyRequest, keeping
   // nothing: returns 1 only for a ClientHello with a valid cookie
   ERR_clear_error();
   BIO_ADDR* address = BIO_ADDR_new();
   r = DTLSv1_listen(mSsl, address);
   BIO_ADDR_free(address);

   unsigned char* out = 0;
   long outLen = BIO_get_mem_data(outBio, &out);
   if(outLen > 0)
   {
      response.assign(out, out + outLen);
   }

   if(r <= 0)
   {
      // Not a ClientHello with a valid cookie (r is 0), or one OpenSSL couldn't handle (r < 0,
      // so start again with a fresh SSL).  Don't leave its errors for the next caller on this thread.
      ERR_clear_error();
      if(r < 0)
      {
         SSL_free(mSsl);
         mSsl = 0;
      }
      return 0;
   }

   // The peer has proved it receives at its address: the SSL, holding its ClientHello, becomes
   // the new server socket's, which carries on the handshake
   DtlsSocket* socket = new DtlsSocket(makeContext(), mFactory, mSsl, peer);
   mSsl = 0;
   socket->doHandshakeIteration();
   return socket;
}

#endif 
/* ====================================================================

 Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
