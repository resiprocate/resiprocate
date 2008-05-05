#if !defined(MediaStream_hxx)
#define MediaStream_hxx

#include <asio.hpp>
#include <asio/ssl.hpp>

#include <srtp.h>

#include "dtls_wrapper/DtlsFactory.hxx"
#include "Flow.hxx"

using namespace reTurn;

namespace flowmanager
{

/**
  This class represents a media stream, that consists of a series of componenets 
  or flows.  For media streams based on RTP, there are two components per media 
  stream - one for RTP, and one for RTCP.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/
class MediaStreamHandler
{
public:
   MediaStreamHandler() {}
   virtual ~MediaStreamHandler() {}

   virtual void onMediaStreamReady(const StunTuple& rtpTuple, const StunTuple& rtcpTuple) = 0;
   virtual void onMediaStreamError(unsigned int errorCode) = 0;
};

#define RTP_COMPONENT_ID   1
#define RTCP_COMPONENT_ID  2

class MediaStream
{
public:
   enum NatTraversalMode
   {
      NoNatTraversal,
      StunBindDiscovery,
      TurnAllocation
   };

   enum SrtpCryptoSuite
   {
      SRTP_AES_CM_128_HMAC_SHA1_32,
      SRTP_AES_CM_128_HMAC_SHA1_80
   };

   MediaStream(asio::io_service& ioService,
               asio::ssl::context& sslContext,
               MediaStreamHandler& mediaStreamHandler,
               const StunTuple& localRtpBinding, 
               const StunTuple& localRtcpBinding,   // pass in transport type = None to disable RTCP
               dtls::DtlsFactory* dtlsFactory = 0,
               NatTraversalMode natTraversalMode = NoNatTraversal,
               const char* natTraversalServerHostname = 0, 
               unsigned short natTraversalServerPort = 0, 
               const char* stunUsername = 0,
               const char* stunPassword = 0); 
   virtual ~MediaStream();

   Flow* getRtpFlow() { return mRtpFlow; }
   Flow* getRtcpFlow() { return mRtcpFlow; }

   // SRTP methods
   bool createOutboundSRTPSession(SrtpCryptoSuite cryptoSuite, const char* key, unsigned int keyLen);
   bool createInboundSRTPSession(SrtpCryptoSuite cryptoSuite, const char* key, unsigned int keyLen);

protected:
   friend class Flow;

   // SRTP members
   dtls::DtlsFactory* mDtlsFactory;
   volatile bool mSRTPSessionInCreated;
   volatile bool mSRTPSessionOutCreated;
   resip::Mutex mMutex;
   SrtpCryptoSuite mCryptoSuiteIn;
   SrtpCryptoSuite mCryptoSuiteOut;
   uint8_t mSRTPMasterKeyIn[SRTP_MASTER_KEY_LEN];
   uint8_t mSRTPMasterKeyOut[SRTP_MASTER_KEY_LEN];
   srtp_policy_t mSRTPPolicyIn;
   srtp_policy_t mSRTPPolicyOut;
   srtp_t mSRTPSessionIn;
   srtp_t mSRTPSessionOut;

   err_status_t srtpProtect(void* data, int* size, bool rtcp);
   err_status_t srtpUnprotect(void* data, int* size, bool rtcp);

   // Nat Traversal Members
   NatTraversalMode mNatTraversalMode;
   resip::Data mNatTraversalServerHostname;
   unsigned short mNatTraversalServerPort;
   resip::Data mStunUsername;
   resip::Data mStunPassword;

private:
   // Note: these member variables are set at creation time and never changed, thus
   //       they do not require mutex protection
   MediaStreamHandler& mMediaStreamHandler;
   bool mRtcpEnabled;

   Flow* mRtpFlow;
   Flow* mRtcpFlow;

   virtual void onFlowReady(unsigned int componentId);
   virtual void onFlowError(unsigned int componentId, unsigned int errorCode);
};

}

#endif


/* ====================================================================

 Original contribution Copyright (C) 2008 Plantronics, Inc.
 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */
