#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>

#include "FlowManagerSubsystem.hxx"
#include "FlowManager.hxx"
#include "MediaStream.hxx"

using namespace flowmanager;
#ifdef USE_SSL
#ifdef USE_DTLS
using namespace dtls;
#endif
#endif
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM FlowManagerSubsystem::FLOWMANAGER

MediaStream::MediaStream(asio::io_service& ioService,
                         MediaStreamHandler& mediaStreamHandler,
#ifdef USE_SSL
#ifdef USE_DTLS
                         DtlsFactory* dtlsFactory,
#endif 
#endif 
                         NatTraversalMode natTraversalMode,
                         const char* natTraversalServerHostname, 
                         unsigned short natTraversalServerPort, 
                         const char* stunUsername,
                         const char* stunPassword) :
#ifdef USE_SSL
#ifdef USE_DTLS
   mDtlsFactory(dtlsFactory),
#endif  
#endif  
   mSRTPSessionInCreated(false),
   mSRTPSessionOutCreated(false),
   mSRTPEnabled(false),
   mNatTraversalMode(natTraversalMode),
   mIceAttempted(natTraversalMode == Ice),
   mNatTraversalServerHostname(natTraversalServerHostname),
   mNatTraversalServerPort(natTraversalServerPort),
   mStunUsername(stunUsername),
   mStunPassword(stunPassword),
   mMediaStreamHandler(mediaStreamHandler),
   mRtcpEnabled(false),
   mRtpFlow(NULL),
   mRtcpFlow(NULL),
   mIOService(ioService)
{
}

MediaStream::~MediaStream() 
{
   shutdown();
}

void
MediaStream::initialize(
#ifdef USE_SSL
                        asio::ssl::context* sslContext,
#endif
                        const StunTuple& localRtpBinding, 
                        const StunTuple& localRtcpBinding
)
{
   Lock lock(mMutex);
   Condition cv;
   mIOService.post(boost::bind(&MediaStream::initializeImpl, this, sslContext, localRtpBinding, localRtcpBinding, boost::ref(cv)));
   cv.wait(mMutex);
}

void
MediaStream::initializeImpl(
#ifdef USE_SSL
                        asio::ssl::context* sslContext,
#endif
                        const StunTuple& localRtpBinding, 
                        const StunTuple& localRtcpBinding,
                        resip::Condition& cv
)
{
   Lock lock(mMutex);
   mRtcpEnabled = (localRtcpBinding.getTransportType() != StunTuple::None);
   if(mRtcpEnabled)
   {
      mRtpFlow = new Flow(mIOService, 
#ifdef USE_SSL
                          *sslContext, 
#endif
                          RTP_COMPONENT_ID, 
                          localRtpBinding, 
                          *this);
      mRtpFlow->initialize();

      mRtcpFlow = new Flow(mIOService, 
#ifdef USE_SSL
                           *sslContext, 
#endif
                           RTCP_COMPONENT_ID,
                           localRtcpBinding, 
                           *this);
      mRtcpFlow->initialize();

      mRtpFlow->activateFlow(StunMessage::PropsPortPair);

      // If doing an allocation then wait until RTP flow is allocated, then activate RTCP flow
      if(mNatTraversalMode != TurnAllocation)
      {
         mRtcpFlow->activateFlow();
      }
   }
   else
   {
      mRtpFlow = new Flow(mIOService, 
#ifdef USE_SSL
                          *sslContext, 
#endif
                          RTP_COMPONENT_ID,
                          localRtpBinding, 
                          *this);
      mRtpFlow->initialize();
      mRtpFlow->activateFlow(StunMessage::PropsPortEven);
      mRtcpFlow = 0;
   }
   cv.signal();
}

void
MediaStream::shutdown()
{
   Lock lock(mMutex);
   Condition cv;
   mIOService.dispatch(boost::bind(&MediaStream::shutdownImpl, this, boost::ref(cv)));
   cv.wait(mMutex);
}

void
MediaStream::shutdownImpl(resip::Condition& cv)
{
   Lock lock(mMutex);

   if(mSRTPSessionOutCreated)
   {
      mSRTPSessionOutCreated = false;
      srtp_dealloc(mSRTPSessionOut);
   }
   if(mSRTPSessionInCreated)
   {
      mSRTPSessionInCreated = false;
      srtp_dealloc(mSRTPSessionIn);
   }
   
   mRtpFlow->shutdown();
   delete mRtpFlow;

   if(mRtcpEnabled)
   {
      mRtcpFlow->shutdown();
      delete mRtcpFlow;
   }

   cv.signal();
}

void 
MediaStream::createOutboundSRTPSession(SrtpCryptoSuite cryptoSuite, const resip::Data& key)
{
   mIOService.post(boost::bind(&MediaStream::createOutboundSRTPSessionImpl, this, cryptoSuite, key));
}

void 
MediaStream::createOutboundSRTPSessionImpl(SrtpCryptoSuite cryptoSuite, const resip::Data& key)
{
   if(key.size() != SRTP_MASTER_KEY_LEN)
   {
      ErrLog(<< "Unable to create outbound SRTP session, invalid keyLen=" << key.size());
      return;
   }

   err_status_t status;
   Lock lock(mMutex);
   if(mSRTPSessionOutCreated)
   {
      // Check if settings are the same - if so just return true
      if(cryptoSuite == mCryptoSuiteOut && memcmp(mSRTPMasterKeyOut, key.data(), key.size()) == 0)
      {
         InfoLog(<< "Outbound SRTP session settings unchanged.");
         return;
      }
      else
      {
         InfoLog(<< "Re-creating outbound SRTP session with new settings.");
         mSRTPSessionOutCreated = false;
         srtp_dealloc(mSRTPSessionOut);
      }
   }

   // Copy key locally
   memcpy(mSRTPMasterKeyOut, key.data(), SRTP_MASTER_KEY_LEN);

   // load default srtp/srtcp policy settings
   memset(&mSRTPPolicyOut, 0, sizeof(srtp_policy_t));

   mCryptoSuiteOut = cryptoSuite;
   switch(cryptoSuite)
   {
   case SRTP_AES_CM_128_HMAC_SHA1_80:
      crypto_policy_set_aes_cm_128_hmac_sha1_80(&mSRTPPolicyOut.rtp);
      crypto_policy_set_aes_cm_128_hmac_sha1_80(&mSRTPPolicyOut.rtcp);
      break;
   case SRTP_AES_CM_128_HMAC_SHA1_32:
      crypto_policy_set_aes_cm_128_hmac_sha1_32(&mSRTPPolicyOut.rtp);
      crypto_policy_set_aes_cm_128_hmac_sha1_32(&mSRTPPolicyOut.rtcp);
      break;
   default:
      ErrLog(<< "Unable to create outbound SRTP session, invalid crypto suite=" << cryptoSuite);
      return;
   }

   // set remaining policy settings
   mSRTPPolicyOut.ssrc.type = ssrc_any_outbound;   
   mSRTPPolicyOut.key = mSRTPMasterKeyOut;
   mSRTPPolicyOut.next = 0;
   mSRTPPolicyOut.window_size = 64;

   // Allocate and initailize the SRTP sessions
   status = srtp_create(&mSRTPSessionOut, &mSRTPPolicyOut);
   if(status)
   {
      ErrLog(<< "Unable to create srtp out session, error code=" << status);
      return;
   }
   mSRTPSessionOutCreated = true;
}

void 
MediaStream::createInboundSRTPSession(SrtpCryptoSuite cryptoSuite, const resip::Data& key)
{
   mIOService.post(boost::bind(&MediaStream::createInboundSRTPSessionImpl, this, cryptoSuite, key));
}

void 
MediaStream::createInboundSRTPSessionImpl(SrtpCryptoSuite cryptoSuite, const resip::Data& key)
{
   if(key.size() != SRTP_MASTER_KEY_LEN)
   {
      ErrLog(<< "Unable to create inbound SRTP session, invalid keyLen=" << key.size());
      return;
   }

   err_status_t status;
   Lock lock(mMutex);
   if(mSRTPSessionInCreated)
   {
      // Check if settings are the same - if so just return true
      if(cryptoSuite == mCryptoSuiteIn && memcmp(mSRTPMasterKeyIn, key.data(), key.size()) == 0)
      {
         InfoLog(<< "Inbound SRTP session settings unchanged.");
         return;
      }
      else
      {
         InfoLog(<< "Re-creating inbound SRTP session with new settings.");
         mSRTPSessionInCreated = false;
         srtp_dealloc(mSRTPSessionIn);
      }
   }

   // Copy key locally
   memcpy(mSRTPMasterKeyIn, key.data(), SRTP_MASTER_KEY_LEN);

   memset(&mSRTPPolicyIn, 0, sizeof(srtp_policy_t));

   // load default srtp/srtcp policy settings
   mCryptoSuiteIn = cryptoSuite;
   switch(cryptoSuite)
   {
   case SRTP_AES_CM_128_HMAC_SHA1_80:
      crypto_policy_set_aes_cm_128_hmac_sha1_80(&mSRTPPolicyIn.rtp);
      crypto_policy_set_aes_cm_128_hmac_sha1_80(&mSRTPPolicyIn.rtcp);
      break;
   case SRTP_AES_CM_128_HMAC_SHA1_32:
      crypto_policy_set_aes_cm_128_hmac_sha1_32(&mSRTPPolicyIn.rtp);
      crypto_policy_set_aes_cm_128_hmac_sha1_32(&mSRTPPolicyIn.rtcp);
      break;
   default:
      ErrLog(<< "Unable to create inbound SRTP session, invalid crypto suite=" << cryptoSuite);
      return;
   }

   // set remaining policy settings
   mSRTPPolicyIn.ssrc.type = ssrc_any_inbound;   
   mSRTPPolicyIn.key = mSRTPMasterKeyIn;
   mSRTPPolicyIn.next = 0;
   mSRTPPolicyIn.window_size = 64;

   // Allocate and initailize the SRTP sessions
   status = srtp_create(&mSRTPSessionIn, &mSRTPPolicyIn);
   if(status)
   {
      ErrLog(<< "Unable to create srtp in session, error code=" << status);
      return;
   }
   mSRTPSessionInCreated = true;
}

err_status_t 
MediaStream::srtpProtect(void* data, int* size, bool rtcp)
{
   err_status_t status = err_status_no_ctx;
   if(mSRTPSessionOutCreated)
   {
      if(rtcp)
      {
         status = srtp_protect_rtcp(mSRTPSessionOut, data, size);  
      }
      else
      {
         status = srtp_protect(mSRTPSessionOut, data, size);  
      }
   }
   return status;
}

err_status_t 
MediaStream::srtpUnprotect(void* data, int* size, bool rtcp)
{
   err_status_t status = err_status_no_ctx;
   if(mSRTPSessionInCreated)
   {
      if(rtcp)
      {
         status = srtp_unprotect_rtcp(mSRTPSessionIn, data, size);  
      }
      else
      {
         status = srtp_unprotect(mSRTPSessionIn, data, size);  
      }
   }
   return status;
}

void 
MediaStream::setOutgoingIceUsernameAndPassword(const resip::Data& username, const resip::Data& password)
{
   mIOService.post(boost::bind(&MediaStream::setOutgoingIceUsernameAndPasswordImpl, this, username, password));
}

void
MediaStream::setOutgoingIceUsernameAndPasswordImpl(const resip::Data& username, const resip::Data& password)
{
   Lock lock(mMutex);
   if (mRtpFlow)
   {
      mRtpFlow->setOutgoingIceUsernameAndPassword(username, password);
   }
   if (mRtcpFlow)
   {
      mRtcpFlow->setOutgoingIceUsernameAndPassword(username, password);
   }
}

void 
MediaStream::setLocalIcePassword(const resip::Data& password)
{
   mIOService.post(boost::bind(&MediaStream::setLocalIcePasswordImpl, this, password));
}

void 
MediaStream::setLocalIcePasswordImpl(const resip::Data& password)
{
   Lock lock(mMutex);
   if (mRtpFlow)
   {
      mRtpFlow->setLocalIcePassword(password);
   }
   if (mRtcpFlow)
   {
      mRtcpFlow->setLocalIcePassword(password);
   }
}

void 
MediaStream::onFlowReady(unsigned int componentId)
{
   if(componentId == RTP_COMPONENT_ID && mNatTraversalMode == TurnAllocation && mRtcpFlow)
   {
      // RTP Flow is ready - we can now activate RTCP flow using the reservation token
      mRtcpFlow->activateFlow(mRtpFlow->getReservationToken());
   }
   else
   {
      if(mRtpFlow && mRtcpFlow)
      {
         if(mRtpFlow->isReady() && mRtcpFlow->isReady())
         {
            mMediaStreamHandler.onMediaStreamReady(this, mRtpFlow->getSessionTuple(), mRtcpFlow->getSessionTuple());
         }
      }
      else if(mRtpFlow && mRtpFlow->isReady())
      {
         mMediaStreamHandler.onMediaStreamReady(this, mRtpFlow->getSessionTuple(), StunTuple());
      }
   }
}

void 
MediaStream::onFlowError(unsigned int componentId, unsigned int errorCode)
{
   if ((errorCode == asio::error::host_not_found || errorCode == 8008) && mIceAttempted)
   {
      // .jjg. ignore this error if ICE is in use, since there's a chance media will still flow
      if (mRtpFlow && mRtcpFlow)
      {
         if (componentId == RTCP_COMPONENT_ID)
         {
            mNatTraversalServerHostname = resip::Data::Empty;
            mRtpFlow->activateFlow();
            mRtcpFlow->activateFlow();
         }
      }
      else if (mRtpFlow)
      {
         if (componentId == RTP_COMPONENT_ID)
         {
            mNatTraversalServerHostname = resip::Data::Empty;
            mRtpFlow->activateFlow();
         }
      }
   }
   else
   {
      mMediaStreamHandler.onMediaStreamError(this, errorCode);  // TODO assign real error code
   }
}

void 
MediaStream::onFlowIceComplete(unsigned int componentId, bool iAmIceControlling)
{
   if(mNatTraversalMode == Ice)
   {
      if(mRtpFlow && mRtcpFlow)
      {
         if(mRtpFlow->isReady() && mRtcpFlow->isReady())
         {
            mMediaStreamHandler.onIceComplete(
               this, 
               mRtpFlow->getLocalNominatedIceCandidate(), 
               mRtcpFlow->getLocalNominatedIceCandidate(), 
               mRtpFlow->getRemoteNominatedIceCandidate(), 
               mRtcpFlow->getRemoteNominatedIceCandidate(), 
               iAmIceControlling);
         }
      }
      else if(mRtpFlow && mRtpFlow->isReady())
      {
         mMediaStreamHandler.onIceComplete(
            this, 
            mRtpFlow->getLocalNominatedIceCandidate(), 
            reTurn::IceCandidate(), 
            mRtpFlow->getRemoteNominatedIceCandidate(), 
            reTurn::IceCandidate(), 
            iAmIceControlling);
      }
   }
}

void 
MediaStream::onFlowIceFailed(unsigned int componentId, bool iAmIceControlling)
{
   if(mNatTraversalMode == Ice)
   {
      if(mRtpFlow && mRtcpFlow)
      {
         if(componentId == RTCP_COMPONENT_ID)
         {
            StunTuple fallbackRtpTuple = mRtpFlow->getSessionTuple();
            StunTuple fallbackRtcpTuple = mRtcpFlow->getSessionTuple();
            mNatTraversalMode = StunBindDiscovery;
            mMediaStreamHandler.onIceFailed(
               this,
               fallbackRtpTuple,
               fallbackRtcpTuple,
               iAmIceControlling);
         }
      }
      else if(mRtpFlow)
      {
         StunTuple fallbackRtpTuple = mRtpFlow->getSessionTuple();
         mNatTraversalMode = StunBindDiscovery;
         mMediaStreamHandler.onIceFailed(
            this, 
            fallbackRtpTuple,
            StunTuple(),
            iAmIceControlling);
      }
   }
}

void 
MediaStream::setSRTPEnabled(bool enabled)
{
   mIOService.post(boost::bind(&MediaStream::setSRTPEnabledImpl, this, enabled));
}

void
MediaStream::setIceDisabled()
{
   mIOService.post(boost::bind(&MediaStream::setIceDisabledImpl, this));
}

void
MediaStream::setIceDisabledImpl()
{
   if(mNatTraversalMode == Ice)
   {
      // fall back to STUN, since we will still want our public IP (if we have one) to show up
      mNatTraversalMode = StunBindDiscovery;
   }
}


/* ====================================================================

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
