#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"
extern "C" {
#include <openssl/srtp.h>
}

using namespace dtls;

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
    r=SSL_CTX_set_tlsext_use_srtp(mContext, "SRTP_AES128_CM_SHA1_80:SRTP_AES128_CM_SHA1_32");
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
