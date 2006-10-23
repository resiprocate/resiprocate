#ifndef testDtlsUdp_hxx
#define testDtlsUdp_hxx

#include <memory>
#include "DtlsSocket.hxx"

extern "C" 
{
#include <srtp/include/srtp.h>
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
     uint32_t ssrc;
};

}

#endif

