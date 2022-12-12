#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_SSL

#ifndef DtlsSocket_hxx
#define DtlsSocket_hxx

#include <memory>
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
      bool handlePacketMaybe(const unsigned char* bytes, unsigned int len);
      
      // Called by DtlSocketTimer when timer expires - causes a retransmission (forceRetransmit)
      void expired(DtlsSocketTimer*);
      
      // Retrieves the finger print of the certificate presented by the remote party
      bool getRemoteFingerprint(char *fingerprint);

      // Retrieves the finger print of the certificate presented by the remote party and checks
      // it agains the passed in certificate
      bool checkFingerprint(const char* fingerprint, unsigned int len);

      // Retrieves the finger print of our local certificate, same as getMyCertFingerprint from DtlsFactory
      void getMyCertFingerprint(char *fingerprint);

      // For client sockets only - causes a client handshake to start (doHandshakeIteration)
      void startClient();

      // Returns the socket type: Client or Server
	   SocketType getSocketType() {return mSocketType;} 

      // Retreives the SRTP session keys from the Dtls session
      SrtpSessionKeys getSrtpSessionKeys();

      // Utility fn to compute a certificates fingerprint
      static void computeFingerprint(X509 *cert, char *fingerprint);
 
      // Retrieves the DTLS negotiated SRTP profile - may return 0 if profile selection failed
      SRTP_PROTECTION_PROFILE* getSrtpProfile();      

      // Creates SRTP session policies appropriately based on socket type (client vs server) and keys
      // extracted from the DTLS handshake process
      void createSrtpSessionPolicies(srtp_policy_t& outboundPolicy, srtp_policy_t& inboundPolicy);      
      
      // returns true if the DTLS handshake has completed
      bool handshakeCompleted() { return mHandshakeCompleted; }

      DtlsSocketContext* getSocketContext() { return mSocketContext.get(); }

   private:
      friend class DtlsFactory;

      // Causes an immediate handshake iteration to happen, which will retransmit the handshake
      void forceRetransmit();     

      // Creates an SSL socket, and if client sets state to connect_state and if server sets state to accept_state.  Sets SSL BIO's.
      DtlsSocket(std::unique_ptr<DtlsSocketContext> socketContext, DtlsFactory* factory, enum SocketType);

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
};

}
#endif

#endif 
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
