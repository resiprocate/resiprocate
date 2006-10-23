#ifndef testDtlsUdp_hxx
#define testDtlsUdp_hxx

#include <memory>
#include "DtlsSocket.hxx"

namespace dtls
{

class TestDtlsUdpSocketContext: public DtlsSocketContext {
  public:
     TestDtlsUdpSocketContext(int fd,struct sockaddr_in *peerAddr);
     virtual ~TestDtlsUdpSocketContext(){};
     virtual void write(const unsigned char* data, unsigned int len);
     virtual void handshakeCompleted();
     virtual void handshakeFailed(const char *err);

  private:
     int mFd;
     struct sockaddr_in mPeerAddr;
};

}

#endif

