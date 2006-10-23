#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"
extern "C" {
#include <openssl/srtp.h>
}

using namespace dtls;
const char* DtlsFactory::DefaultSrtpProfile = "SRTP_AES128_CM_SHA1_80:SRTP_AES128_CM_SHA1_32";

DtlsFactory::DtlsFactory(std::auto_ptr<DtlsTimerContext> tc,X509 *cert, EVP_PKEY *privkey):
  mTimerContext(tc),mCert(cert)
  {
    int r;
    
    mContext=SSL_CTX_new(DTLSv1_method());
    assert(mContext);
    
    r=SSL_CTX_use_certificate(mContext, cert);
    assert(r==1);

    r=SSL_CTX_use_PrivateKey(mContext, privkey);
    assert(r==1);

    // Set SRTP profiles: TODO, make configurable
    r=SSL_CTX_set_tlsext_use_srtp(mContext, DefaultSrtpProfile);
    assert(r==0);
  }

DtlsFactory::~DtlsFactory()
  {
    SSL_CTX_free(mContext);

  }

DtlsSocket*
DtlsFactory::createClient(std::auto_ptr<DtlsSocketContext> context){
  return new DtlsSocket(context,this,DtlsSocket::Client);
}

DtlsSocket*
DtlsFactory::createServer(std::auto_ptr<DtlsSocketContext> context){
  return new DtlsSocket(context,this,DtlsSocket::Server);  
}

void
DtlsFactory::getMyCertFingerprint(char *fingerprint){
  DtlsSocket::computeFingerprint(mCert,fingerprint);
}

PacketType
DtlsFactory::demuxPacket(const unsigned char *data, unsigned int len) {
  assert(len>=1);
  
  if((data[0]==0)   || (data[0]==1))
    return stun;
  if((data[0]>=128) && (data[0]<=191))
    return rtp;
  if((data[0]>=20)  && (data[0]<=64))
    return dtls;

  return unknown;
}
