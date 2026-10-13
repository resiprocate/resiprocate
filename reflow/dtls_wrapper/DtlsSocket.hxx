#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_SSL

#ifndef DtlsSocket_hxx
#define DtlsSocket_hxx

#include <functional>
#include <memory>
#include <string>
#include <vector>
extern "C" 
{
#include "../Srtp2Helper.hxx"
}

#include <openssl/e_os2.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>

namespace dtls
{
class DtlsFactory;
class DtlsSocket;
class DtlsTimer;

class DtlsSocketContext
{
   public:
      //memory is only valid for duration of callback; must be copied if queueing
      //is required 
      virtual ~DtlsSocketContext(){}      
      virtual void write(const unsigned char* data, unsigned int len)=0;
      virtual void handshakeCompleted()=0;
      virtual void handshakeFailed(const char *err)=0;

   protected:
      DtlsSocket *mSocket;
     
   private:
      friend class DtlsSocket;
     
      void setDtlsSocket(DtlsSocket *sock) {mSocket=sock;}
};

class SrtpSessionKeys
{
   public:
      std::vector<unsigned char> clientMasterKey;
      std::vector<unsigned char> serverMasterKey;
      std::vector<unsigned char> clientMasterSalt;
      std::vector<unsigned char> serverMasterSalt;
};

class DtlsSocketTimer;
   
class DtlsSocket
{
   public:
	   enum SocketType { Client, Server};
      ~DtlsSocket(); 

      // Inspects packet to see if it's a DTLS packet, if so continue processing
      bool handlePacketMaybe(const unsigned char* bytes, size_t len);
      
      // Called by DtlSocketTimer when timer expires - causes a retransmission (forceRetransmit)
      void expired(DtlsSocketTimer*);
      
      // Size of the buffer getMyCertFingerprint and computeFingerprint write to: a SHA-256
      // fingerprint, as uppercase hex pairs separated by colons
      static const size_t FingerprintSize = 32 * 3;

      // Retrieves the finger print of the certificate presented by the remote party, computed with
      // the hash function an SDP fingerprint names (see checkFingerprint) - returns false if there is
      // no certificate, the hash function isn't supported, or the fingerprint doesn't fit in
      // fingerprintSize (EVP_MAX_MD_SIZE * 3 is always enough)
      bool getRemoteFingerprint(const char* hashFunction, char *fingerprint, size_t fingerprintSize);

      // Checks the certificate presented by the remote party against an SDP fingerprint
      // (RFC 8122): hashFunction is its hash function textual name (eg. "sha-256"), and
      // fingerprint its uppercase or lowercase hex pairs separated by colons
      bool checkFingerprint(const char* hashFunction, const char* fingerprint);

      // Returns true if checkFingerprint supports this hash function textual name
      static bool isFingerprintHashFunctionSupported(const char* hashFunction);

      // Retrieves the finger print of our local certificate, same as getMyCertFingerprint from DtlsFactory
      void getMyCertFingerprint(char *fingerprint);

      // For client sockets only - causes a client handshake to start (doHandshakeIteration)
      void startClient();

      // Returns the socket type: Client or Server
	   SocketType getSocketType() {return mSocketType;} 

      // Retreives the SRTP session keys from the Dtls session, sized for the negotiated SRTP
      // profile - empty if no profile was negotiated or libsrtp doesn't support it
      SrtpSessionKeys getSrtpSessionKeys();

      // Utility fn to compute a certificate's SHA-256 fingerprint into a buffer of FingerprintSize
      static void computeFingerprint(X509 *cert, char *fingerprint);

      // Utility fn to compute a certificate's fingerprint with the given digest - returns false
      // if it doesn't fit in fingerprintSize (EVP_MAX_MD_SIZE * 3 is always enough)
      static bool computeFingerprint(X509 *cert, const EVP_MD* digest, char *fingerprint, size_t fingerprintSize);

      // Retrieves the DTLS negotiated SRTP profile - may return 0 if profile selection failed
      SRTP_PROTECTION_PROFILE* getSrtpProfile();

      // Creates SRTP session policies appropriately based on socket type (client vs server), the
      // negotiated SRTP profile and keys extracted from the DTLS handshake process.  Returns false
      // if no SRTP profile was negotiated or libsrtp doesn't support it.  On success the policies'
      // key members point to memory allocated with new[]: the caller must delete[] them once
      // it has created its SRTP sessions (srtp_create copies them).
      bool createSrtpSessionPolicies(srtp_policy_t& outboundPolicy, srtp_policy_t& inboundPolicy);
      
      // returns true if the DTLS handshake has completed
      bool handshakeCompleted() { return mHandshakeCompleted; }

      // returns true if the DTLS handshake has failed
      bool handshakeFailed() const { return mHandshakeFailed; }

      DtlsSocketContext* getSocketContext() { return mSocketContext.get(); }

   private:
      friend class DtlsFactory;
      friend class DtlsListener;

      // Causes an immediate handshake iteration to happen, which will retransmit the handshake
      void forceRetransmit();

      // Creates an SSL socket, and if client sets state to connect_state and if server sets state to accept_state.  Sets SSL BIO's.
      DtlsSocket(std::unique_ptr<DtlsSocketContext> socketContext, DtlsFactory* factory, enum SocketType);

      // A server socket continuing the handshake DtlsListener started on acceptedSsl, with peer
      DtlsSocket(std::unique_ptr<DtlsSocketContext> socketContext, DtlsFactory* factory, SSL* acceptedSsl, const std::string& peer);

      // Creates the SSL object for a socket of the given type, with its BIOs
      static SSL* newSsl(DtlsFactory* factory, SocketType type);

      // Give CPU cyces to the handshake process - checks current state and acts appropraitely
      void doHandshakeIteration();

      // returns the amount of time between handshake retranmssions (500ms)
      int getReadTimeout();
      
      // Internals
      std::unique_ptr<DtlsSocketContext> mSocketContext;
      DtlsFactory* mFactory;
      DtlsTimer *mReadTimer;  // Timer used during handshake process
      
      // OpenSSL context data
      SSL *mSsl;
      BIO *mInBio;
      BIO *mOutBio;
      
      SocketType mSocketType;
      bool mHandshakeCompleted;
      bool mHandshakeFailed;
      std::string mCookiePeer;  // the peer of a socket created by DtlsListener, for the cookie callbacks
};

// Stands in front of the server DtlsSockets for one local address, doing the cookie exchange of
// RFC 6347 section 4.2.1: a ClientHello from a new peer is answered with a HelloVerifyRequest and
// nothing is kept for the peer; only a ClientHello that returns a valid cookie - which proves the
// peer receives at its address - gets a server DtlsSocket.  So packets from spoofed addresses
// can't make us keep state, or answer them with a full handshake flight (amplification).
class DtlsListener
{
   public:
      ~DtlsListener();

      // Handles a DTLS packet from peer (anything that identifies its address and port, eg.
      // "192.0.2.1:5004"), which has no DtlsSocket yet.  Returns a new server DtlsSocket, with the
      // context makeContext() returns, once peer's ClientHello carries a valid cookie - the socket
      // has already handled that ClientHello.  Otherwise returns null, with response holding what
      // to send back to peer (a HelloVerifyRequest), if anything.
      DtlsSocket* handlePacket(const unsigned char* bytes, size_t len, const std::string& peer,
                               std::vector<unsigned char>& response,
                               const std::function<std::unique_ptr<DtlsSocketContext>()>& makeContext);

   private:
      friend class DtlsFactory;
      explicit DtlsListener(DtlsFactory* factory);

      DtlsFactory* mFactory;
      SSL* mSsl;          // the SSL DTLSv1_listen runs on, handed to the socket it creates
      std::string mPeer;  // the peer of the packet being handled, for the cookie callbacks
};

}
#endif

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
