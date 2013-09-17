#ifndef RESIP_WssConnection_hxx
#define RESIP_WssConnection_hxx

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "resip/stack/ssl/TlsConnection.hxx"
#include "resip/stack/WsConnectionBase.hxx"

namespace resip
{

class WssConnection :  public TlsConnection, public WsConnectionBase
{
   public:
      WssConnection( Transport* transport, const Tuple& who, Socket fd,
                     Security* security, bool server, Data domain,
                     SecurityTypes::SSLType sslType ,
                     Compression &compression);

   private:
      /// No default c'tor
      WssConnection();
};

}
#endif
