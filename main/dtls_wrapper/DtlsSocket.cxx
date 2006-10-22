#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"

using namespace dtls;

DtlsSocket::DtlsSocket(std::auto_ptr<DtlsSocketContext>, DtlsFactory* factory, enum SocketType type) 
{
   ssl=SSL_new(factory->mContext);

  switch(type){
    case Client:
      SSL_set_connect_state(ssl);
      break;
    case Server:
      SSL_set_accept_state(ssl);
      break;
    default:
      assert(0);
  }
}

bool
DtlsSocket::checkFingerprint(const char* fingerprint, unsigned int len){
  return false;
}

