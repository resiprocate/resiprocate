#if !defined(FlowManager_hxx)
#define FlowManager_hxx

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "FlowContext.hxx"
#include "MediaStream.hxx"
#include "FlowManagerException.hxx"
#include "RTCPEventLoggingHandler.hxx"

#include "dtls_wrapper/DtlsFactory.hxx"
#include <openssl/crypto.h>
#include <openssl/ssl.h>

#include <asio.hpp>
#include <asio/ssl.hpp>

#include <map>
#include <memory>
#include <utility>

using namespace reTurn;

namespace flowmanager
{

/**
  This class represents the Flow Manager.  It is responsible for sending/receiving
  media and performing the necessary NAT traversal.  
  
  Threading Notes:  This class implements a thread
  to manage the asyncrouns reTurn client library calls.  Essentially all
  asyncrounous operations for all Flows will be called from this one thread.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/
class IOServiceThread;
class FlowDtlsTimerContext;

class FlowManager
{
public:  
   FlowManager();  // throws FlowManagerException
   virtual ~FlowManager();

   // This API assumes that RTCP localBinding is always the same as RTP binding but add one to the port number
   // We can add a new API in the future to accomodate, custom RTCP bindings as required
   MediaStream* createMediaStream(MediaStreamHandler& mediaStreamHandler,
                                  const StunTuple& localBinding, 
                                  bool rtcpEnabled = true,
                                  MediaStream::NatTraversalMode natTraversalMode = MediaStream::NoNatTraversal,
                                  const char* natTraversalServerHostname = 0, 
                                  unsigned short natTraversalServerPort = 0, 
                                  const char* stunUsername = 0,
                                  const char* stunPassword = 0,
                                  bool forceCOMedia = false,
                                  std::shared_ptr<FlowContext> context = nullptr);

   void initializeDtlsFactory(const char* certAor);
#ifdef USE_SSL
   // Creates the self-signed RSA certificate initializeDtlsFactory uses, for the AOR sip:<pAor>,
   // signed with SHA-256.  Returns non-zero on success.
   static int createCert(const resip::Data& pAor, int expireDays, int keyLen, X509*& outCert, EVP_PKEY*& outKey);
#endif
   dtls::DtlsFactory* getDtlsFactory() { return mDtlsFactory; }

#ifdef USE_SSL
   /// Adds the root (CA) certificates in a PEM file to those TURN servers' TLS certificates are
   /// verified with.  "ca.pem" in the working directory is loaded at construction, if it exists.
   /// Returns false if the file can't be loaded.
   bool addTlsRootCertificates(const resip::Data& rootCertificatesFile);
#endif

   /// Whether flows using TURN over TLS check that the server's certificate is for the TURN
   /// server's host name (or IP address) - default true.  Applies to media streams created after
   /// the call.
   void setValidateTlsServerHostname(bool validate) { mValidateTlsServerHostname = validate; }

   void setRTCPEventLoggingHandler(std::shared_ptr<RTCPEventLoggingHandler> handler) noexcept { mRtcpEventLoggingHandler = handler; }
   RTCPEventLoggingHandler* getRTCPEventLoggingHandler() const noexcept { return 0 != mRtcpEventLoggingHandler.get() ? mRtcpEventLoggingHandler.get() : 0; }

protected: 

private:
   static void srtpEventHandler(srtp_event_data_t *data);

   std::shared_ptr<RTCPEventLoggingHandler> mRtcpEventLoggingHandler;

   // Member variables used to manager asio io service thread
   asio::io_context mIOService;
   IOServiceThread* mIOServiceThread;
   asio::executor_work_guard<asio::io_context::executor_type>* mIOServiceWork;
   asio::ssl::context mSslContext;
   
   X509* mClientCert;
   EVP_PKEY* mClientKey;
   dtls::DtlsFactory* mDtlsFactory;
   FlowDtlsTimerContext* mDtlsTimerContext;  // owned by mDtlsFactory
   bool mValidateTlsServerHostname = true;
};

}

#endif


/* ====================================================================

 Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

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
