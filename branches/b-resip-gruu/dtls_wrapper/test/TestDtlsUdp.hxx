#ifndef testDtlsUdp_hxx
#define testDtlsUdp_hxx

#include <memory>
#include "DtlsSocket.hxx"

extern "C" 
{
#include <srtp.h>
}

namespace dtls
{

class TestDtlsUdpSocketContext: public DtlsSocketContext {
  public:
     TestDtlsUdpSocketContext(int fd,struct sockaddr_in *peerAddr);
     virtual ~TestDtlsUdpSocketContext(){};
     virtual void write(const unsigned char* data, unsigned int len);
     virtual void handshakeCompleted();
     virtual void handshakeFailed(const char *err);
     void sendRtpData(const unsigned char *data, unsigned int len);
     void recvRtpData(unsigned char *in, unsigned int inlen, unsigned char *out, unsigned int *outlen,unsigned int maxoutlen);
     
  private:
     int mFd;
     struct sockaddr_in mPeerAddr;
     uint32_t ssrc;   // TODO: initialize with something
     srtp_policy_t srtpPolicyIn;
     srtp_policy_t srtpPolicyOut;     
     srtp_t srtpIn;
     srtp_t srtpOut;
     bool useSrtp; 
     uint16_t mRtpSeq; // TODO: initialize with something
};

}

#endif


/* ====================================================================

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
