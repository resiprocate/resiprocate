#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if !defined(FlowDtlsSocketContext_hxx)
#define FlowDtlsSocketContext_hxx 

#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif
#ifdef WIN32
#include <srtp.h>
#else
#include <srtp/srtp.h>
#endif

#include "dtls_wrapper/DtlsSocket.hxx"
#include "Flow.hxx"

/**
  This class is used during media sessions that use Dtls-Srtp
  for a media transport.  It handles callbacks from the Dtls
  wrapper module and hold Srtp session policies for a particular
  dtls endpoint.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

namespace flowmanager
{

class FlowDtlsSocketContext : public dtls::DtlsSocketContext
{
public:
   FlowDtlsSocketContext(Flow& flow, const asio::ip::address& address, unsigned short port);
   virtual ~FlowDtlsSocketContext();

   // DtlsSocketContext Virtual Fns
   virtual void write(const unsigned char* data, unsigned int len);
   virtual void handshakeCompleted();
   virtual void handshakeFailed(const char *err);

   srtp_t* getSrtpSessionIn() { return &mSRTPSessionIn; }
   srtp_t* getSrtpSessionOut() { return &mSRTPSessionOut; }
   bool isSrtpInitialized() { return mSrtpInitialized; }
   void fingerprintMismatch();

   err_status_t srtpProtect(void* data, int* size, bool rtcp);
   err_status_t srtpUnprotect(void* data, int* size, bool rtcp);

private:   
   Flow& mFlow;
   asio::ip::address mAddress;
   unsigned short mPort;
   srtp_policy_t mSRTPPolicyIn;
   srtp_policy_t mSRTPPolicyOut;
   srtp_t mSRTPSessionIn;
   srtp_t mSRTPSessionOut;
   volatile bool mSrtpInitialized;
};

}

#endif
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
