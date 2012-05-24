#ifndef testDtlsUdp_hxx
#define testDtlsUdp_hxx

#include <memory>
#include "DtlsSocket.hxx"

extern "C" 
{
#include <srtp/srtp.h>
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
