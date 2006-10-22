#include "DtlsSocket.hxx"

using namespace dtls;

DtlsSocket::DtlsSocket(std::auto_ptr<DtlsSocketContext>, DtlsFactory* factory) {
  ;
}

bool
DtlsSocket::checkFingerprint(const char* fingerprint, unsigned int len){
  return false;
}

