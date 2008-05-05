#if !defined(FlowDtlsSocketContext_hxx)
#define FlowDtlsSocketContext_hxx 

#include "dtls_wrapper/DtlsSocket.hxx"
#include <srtp.h>
#include <asio.hpp>

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

