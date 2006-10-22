#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"

using namespace dtls;

DtlsSocket::DtlsSocket(std::auto_ptr<DtlsSocketContext>, DtlsFactory* factory, enum SocketType type) {
  mssl=SSL_new(factory->mContext);

  switch(type){
    case Client:
      SSL_set_state_connect(ssl);
      break;
    case Server:
      SSL_set_state_accept(ssl);
      break;
    default:
      assert(0);
  }
}

bool
DtlsSocket::checkFingerprint(const char* fingerprint, unsigned int len){
  return false;
}

