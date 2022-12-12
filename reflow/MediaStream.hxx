#if !defined(MediaStream_hxx)
#define MediaStream_hxx

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// !slg! At least for builds in Visual Studio on windows this include needs to be above ASIO includes since inlined shared_from_this has 
// a different linkage signature if included after - haven't investigated the full details as to exactly why this happens
#include <memory>

namespace asio
{
   class io_context;
   typedef io_context io_service;
#ifdef USE_SSL
   namespace ssl
   {
      class context;
   }
#endif
}

#include "Srtp2Helper.hxx"

#include "dtls_wrapper/DtlsFactory.hxx"
#include "FlowContext.hxx"
#include "Flow.hxx"
#include "RTCPEventLoggingHandler.hxx"
#include "rutil/MediaConstants.hxx"

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
               const char* stunPassword = 0,
               bool forceCOMedia = false,
               std::shared_ptr<RTCPEventLoggingHandler> rtcpEventLoggingHandler = nullptr,
               std::shared_ptr<FlowContext> context = nullptr);
   virtual ~MediaStream();

   Flow* getRtpFlow() { return mRtpFlow; }
   Flow* getRtcpFlow() { return mRtcpFlow; }

   // SRTP methods - should be called before sending or receiving on RTP or RTCP flows
   bool createOutboundSRTPSession(resip::MediaConstants::SrtpCryptoSuite cryptoSuite, const char* key, unsigned int keyLen);
   bool createInboundSRTPSession(resip::MediaConstants::SrtpCryptoSuite cryptoSuite, const char* key, unsigned int keyLen);

protected:
   friend class Flow;

   // SRTP members
   dtls::DtlsFactory* mDtlsFactory;
   volatile bool mSRTPSessionInCreated;
   volatile bool mSRTPSessionOutCreated;
   resip::Mutex mMutex;
   resip::MediaConstants::SrtpCryptoSuite mCryptoSuiteIn;
   resip::MediaConstants::SrtpCryptoSuite mCryptoSuiteOut;
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
   bool mForceCOMedia;

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

 Copyright (c) 2007-2008, Plantronics, Inc.
 Copyright (c) 2008-2018, SIP Spectrum, Inc.
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
