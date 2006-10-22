#ifndef DtlsFactory_hxx
#define DtlsFactory_hxx

#include <memory>
#include <openssl/e_os2.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>

#include "DtlsTimer.hxx"

class DtlsSocket;
class DtlsSocketContext;

namespace dtls
{

//Not threadsafe. Timers must fire in the same thread as dtls processing.
class DtlsFactory
{
   public:
     DtlsFactory(std::auto_ptr<DtlsTimerContext> tc);

     // Note: this orphans any DtlsSockets you were stupid enough
     // not to free
     ~DtlsFactory();
     
     DtlsSocket* createClient(DtlsSocketContext* context);
     DtlsSocket* createServer(DtlsSocketContext* context);

     DtlsTimerContext& getTimerContext() {return *mTimerContext;}
       
      //context accessor
private:
     friend class DtlsSocket;
     SSL_CTX* mContext;
     std::auto_ptr<DtlsTimerContext> mTimerContext;
};

}
#endif
