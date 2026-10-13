#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_SSL
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <iostream>

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/Timer.hxx>

#include "FlowDtlsSocketContext.hxx"
#include "FlowManagerSubsystem.hxx"

using namespace flowmanager;
using namespace resip;
using namespace dtls;
using namespace std;

#define RESIPROCATE_SUBSYSTEM FlowManagerSubsystem::FLOWMANAGER

FlowDtlsSocketContext::FlowDtlsSocketContext(Flow& flow, const asio::ip::address& address, unsigned short port) 
   : mFlow(flow), mAddress(address), mPort(port), mSrtpInitialized(false)
{
}

FlowDtlsSocketContext::~FlowDtlsSocketContext()
{
   destroySrtpSessions();
}

void
FlowDtlsSocketContext::write(const unsigned char* data, unsigned int len)
{
   InfoLog(<< "Dtls write to " << mAddress.to_string() << ":" << mPort << " called.  ComponentId=" << mFlow.getComponentId());
   mFlow.rawSendTo(mAddress, mPort, (const char*)data, len);
}

void
FlowDtlsSocketContext::handshakeCompleted()
{
   InfoLog(<< "Flow Dtls Handshake Completed!  ComponentId=" << mFlow.getComponentId());

   char fprint[DtlsSocket::FingerprintSize];
   if(mSocket->getRemoteFingerprint("sha-256", fprint, sizeof(fprint)))
   {
      InfoLog(<< "Remote fingerprint (SHA-256) = " << fprint << "  ComponentId=" << mFlow.getComponentId());
   }

   SRTP_PROTECTION_PROFILE* srtp_profile = mSocket->getSrtpProfile();
   if(srtp_profile)
   {
      InfoLog(<< "SRTP Extension negotiated profile=" << srtp_profile->name << "  ComponentId=" << mFlow.getComponentId());
   }

   // The Flow calls us with its mutex held, so read its fingerprint directly.  If the remote SDP
   // hasn't arrived yet, SRTP is set up when Flow::setRemoteSDPFingerprint is called
   checkRemoteFingerprint(mFlow.mRemoteSDPFingerprintHashFunction, mFlow.mRemoteSDPFingerprint);
}

void
FlowDtlsSocketContext::handshakeFailed(const char *err)
{
   ErrLog(<< "Flow Dtls Handshake failed: " << err << "  ComponentId=" << mFlow.getComponentId());
}

std::shared_ptr<std::mutex>
FlowDtlsSocketContext::timerLock()
{
   // The Flow's mutex, which it holds whenever it uses its DtlsSockets
   return mFlow.mSharedMutex;
}

void
FlowDtlsSocketContext::checkRemoteFingerprint(const resip::Data& hashFunction, const resip::Data& fingerprint)
{
   if(!mSocket->handshakeCompleted())
   {
      return;
   }

   if(fingerprint.empty())
   {
      InfoLog(<< "Waiting for the remote SDP fingerprint before setting up SRTP.  ComponentId=" << mFlow.getComponentId());
      return;
   }

   if(mSocket->checkFingerprint(hashFunction.c_str(), fingerprint.c_str()))
   {
      if(!mSrtpInitialized)
      {
         InfoLog(<< "Remote certificate matches the SDP fingerprint (" << hashFunction << " " << fingerprint << "), setting up SRTP.  ComponentId=" << mFlow.getComponentId());
         if(!createSrtpSessions())
         {
            ErrLog(<< "Unable to set up SRTP from the Dtls handshake.  ComponentId=" << mFlow.getComponentId());
         }
      }
   }
   else
   {
      // Log the certificate's fingerprint computed with the SDP's hash function, so the two can be compared
      char fprint[EVP_MAX_MD_SIZE * 3];
      Data remoteFingerprint = mSocket->getRemoteFingerprint(hashFunction.c_str(), fprint, sizeof(fprint)) ?
                               Data(fprint) : Data("unavailable (unsupported hash function)");
      WarningLog(<< "Remote certificate does not match the SDP fingerprint, "
                 << (mSrtpInitialized ? "tearing down SRTP" : "not setting up SRTP")
                 << ".  SDP fingerprint=" << hashFunction << " " << fingerprint
                 << ", certificate fingerprint=" << remoteFingerprint << "  ComponentId=" << mFlow.getComponentId());
      // Ensure Srtp is not initalized, so we will not process media packets from this endpoint
      destroySrtpSessions();
   }
}

bool
FlowDtlsSocketContext::createSrtpSessions()
{
   srtp_policy_t policyIn;
   srtp_policy_t policyOut;
   if(!mSocket->createSrtpSessionPolicies(policyOut, policyIn))
   {
      return false;
   }

   bool created = false;
   if(srtp_create(&mSRTPSessionIn, &policyIn) == srtp_err_status_ok)
   {
      if(srtp_create(&mSRTPSessionOut, &policyOut) == srtp_err_status_ok)
      {
         created = true;
      }
      else
      {
         srtp_dealloc(mSRTPSessionIn);
      }
   }

   // srtp_create has derived the session keys, so the master keys DtlsSocket::createSrtpSessionPolicies
   // allocated are no longer needed
   delete[] policyIn.key;
   delete[] policyOut.key;

   mSrtpInitialized = created;
   return created;
}

void
FlowDtlsSocketContext::destroySrtpSessions()
{
   if(mSrtpInitialized)
   {
      mSrtpInitialized = false;
      srtp_dealloc(mSRTPSessionIn);
      srtp_dealloc(mSRTPSessionOut);
   }
}

srtp_err_status_t
FlowDtlsSocketContext::srtpProtect(void* data, int* size, bool rtcp)
{
   srtp_err_status_t status = srtp_err_status_no_ctx;
   if(mSrtpInitialized)
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

srtp_err_status_t
FlowDtlsSocketContext::srtpUnprotect(void* data, int* size, bool rtcp)
{
   srtp_err_status_t status = srtp_err_status_no_ctx;
   if(mSrtpInitialized)
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

#endif 
/* ====================================================================

 Copyright (c) 2008-2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
