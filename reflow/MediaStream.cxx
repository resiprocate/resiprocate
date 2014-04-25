#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/function.hpp>

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/Timer.hxx>

#include "MediaStream.hxx"
#include "FlowManagerSubsystem.hxx"
#include "FlowManager.hxx"


using namespace flowmanager;
#ifdef USE_SSL
using namespace dtls;
#endif
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM FlowManagerSubsystem::FLOWMANAGER

MediaStream::MediaStream(asio::io_service& ioService,
#ifdef USE_SSL
                         asio::ssl::context& sslContext,
#endif
                         MediaStreamHandler& mediaStreamHandler,
                         const StunTuple& localRtpBinding, 
                         const StunTuple& localRtcpBinding, 
#ifdef USE_SSL
                         DtlsFactory* dtlsFactory,
#endif 
                         NatTraversalMode natTraversalMode,
                         const char* natTraversalServerHostname, 
                         unsigned short natTraversalServerPort, 
                         const char* stunUsername,
                         const char* stunPassword) :
#ifdef USE_SSL
   mDtlsFactory(dtlsFactory),
#endif  
   mSRTPSessionInCreated(false),
   mSRTPSessionOutCreated(false),
   mNatTraversalMode(natTraversalMode),
   mNatTraversalServerHostname(natTraversalServerHostname),
   mNatTraversalServerPort(natTraversalServerPort),
   mStunUsername(stunUsername),
   mStunPassword(stunPassword),
   mMediaStreamHandler(mediaStreamHandler)
{
   // Rtcp is enabled if localRtcpBinding transport type != None
   mRtcpEnabled = localRtcpBinding.getTransportType() != StunTuple::None;

   if(mRtcpEnabled)
   {
      mRtpFlow = new Flow(ioService, 
#ifdef USE_SSL
                          sslContext, 
#endif
                          RTP_COMPONENT_ID, 
                          localRtpBinding, 
                          *this);

      mRtcpFlow = new Flow(ioService, 
#ifdef USE_SSL
                           sslContext, 
#endif
                           RTCP_COMPONENT_ID,
                           localRtcpBinding, 
                           *this);

      mRtpFlow->activateFlow(StunMessage::PropsPortPair);

      // If doing an allocation then wait until RTP flow is allocated, then activate RTCP flow
      if(natTraversalMode != TurnAllocation)
      {
         mRtcpFlow->activateFlow();
      }
   }
   else
   {
      mRtpFlow = new Flow(ioService, 
#ifdef USE_SSL
                          sslContext, 
#endif
                          RTP_COMPONENT_ID,
                          localRtpBinding, 
                          *this);
      mRtpFlow->activateFlow(StunMessage::PropsPortEven);
      mRtcpFlow = 0;
   }
}

MediaStream::~MediaStream() 
{
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
   }
   delete mRtpFlow;
   if(mRtcpEnabled)
   {
      delete mRtcpFlow;
   }
}

bool 
MediaStream::createOutboundSRTPSession(SrtpCryptoSuite cryptoSuite, const char* key, unsigned int keyLen)
{
   if(keyLen != SRTP_MASTER_KEY_LEN)
   {
      ErrLog(<< "Unable to create outbound SRTP session, invalid keyLen=" << keyLen);
      return false;
   }

   err_status_t status;
   Lock lock(mMutex);
   if(mSRTPSessionOutCreated)
   {
      // Check if settings are the same - if so just return true
      if(cryptoSuite == mCryptoSuiteOut && memcmp(mSRTPMasterKeyOut, key, keyLen) == 0)
      {
         InfoLog(<< "Outbound SRTP session settings unchanged.");
         return true;
      }
      else
      {
         InfoLog(<< "Re-creating outbound SRTP session with new settings.");
         mSRTPSessionOutCreated = false;
         srtp_dealloc(mSRTPSessionOut);
      }
   }
   memset(&mSRTPPolicyOut, 0, sizeof(mSRTPPolicyOut));

   // Copy key locally
   memcpy(mSRTPMasterKeyOut, key, SRTP_MASTER_KEY_LEN);

   // load default srtp/srtcp policy settings
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
      return false;
   }

   // set remaining policy settings
   mSRTPPolicyOut.ssrc.type = ssrc_any_outbound;   
   mSRTPPolicyOut.key = mSRTPMasterKeyOut;
   mSRTPPolicyOut.window_size = 64;

   // Allocate and initailize the SRTP sessions
   status = srtp_create(&mSRTPSessionOut, &mSRTPPolicyOut);
   if(status)
   {
      ErrLog(<< "Unable to create srtp out session, error code=" << status);
      return false;
   }
   mSRTPSessionOutCreated = true;

   return true;
}

bool 
MediaStream::createInboundSRTPSession(SrtpCryptoSuite cryptoSuite, const char* key, unsigned int keyLen)
{
   if(keyLen != SRTP_MASTER_KEY_LEN)
   {
      ErrLog(<< "Unable to create inbound SRTP session, invalid keyLen=" << keyLen);
      return false;
   }

   err_status_t status;
   Lock lock(mMutex);
   if(mSRTPSessionInCreated)
   {
      // Check if settings are the same - if so just return true
      if(cryptoSuite == mCryptoSuiteIn && memcmp(mSRTPMasterKeyIn, key, keyLen) == 0)
      {
         InfoLog(<< "Inbound SRTP session settings unchanged.");
         return true;
      }
      else
      {
         InfoLog(<< "Re-creating inbound SRTP session with new settings.");
         mSRTPSessionInCreated = false;
         srtp_dealloc(mSRTPSessionIn);
      }
   }
   memset(&mSRTPPolicyIn, 0, sizeof(mSRTPPolicyIn));

   // Copy key locally
   memcpy(mSRTPMasterKeyIn, key, SRTP_MASTER_KEY_LEN);

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
      return false;
   }

   // set remaining policy settings
   mSRTPPolicyIn.ssrc.type = ssrc_any_inbound;   
   mSRTPPolicyIn.key = mSRTPMasterKeyIn;
   mSRTPPolicyIn.window_size = 64;

   // Allocate and initailize the SRTP sessions
   status = srtp_create(&mSRTPSessionIn, &mSRTPPolicyIn);
   if(status)
   {
      ErrLog(<< "Unable to create srtp in session, error code=" << status);
      return false;
   }
   mSRTPSessionInCreated = true;

   return true;
}

err_status_t 
MediaStream::srtpProtect(void* data, int* size, bool rtcp)
{
   Lock lock(mMutex);
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
   Lock lock(mMutex);
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
            mMediaStreamHandler.onMediaStreamReady(mRtpFlow->getSessionTuple(), mRtcpFlow->getSessionTuple());
         }
      }
      else if(mRtpFlow && mRtpFlow->isReady())
      {
         mMediaStreamHandler.onMediaStreamReady(mRtpFlow->getSessionTuple(), StunTuple());
      }
   }
}

void 
MediaStream::onFlowError(unsigned int componentId, unsigned int errorCode)
{
   mMediaStreamHandler.onMediaStreamError(errorCode);  // TODO assign real error code
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
