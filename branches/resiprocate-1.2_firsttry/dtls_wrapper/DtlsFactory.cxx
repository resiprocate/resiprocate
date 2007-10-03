#include "DtlsFactory.hxx"

using namespace dtls;

DtlsFactory::DtlsFactory(std::auto_ptr<DtlsTimerContext> tc):
  mTimerContext(tc)
  {
    mContext=SSL_CTX_new(DTLSv1_method());
  }

DtlsFactory::~DtlsFactory()
  {
    SSL_CTX_free(mContext);

  }

DtlsSocket*
DtlsFactory::createClient(DtlsSocketContext* context){
  return 0;
}

DtlsSocket*
DtlsFactory::createServer(DtlsSocketContext* context){
  return 0;
}

