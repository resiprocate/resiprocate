#ifndef RESIP_WsConnection_hxx
#define RESIP_WsConnection_hxx

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "resip/stack/TcpConnection.hxx"
#include "resip/stack/WsConnectionBase.hxx"
#include "rutil/SharedPtr.hxx"

namespace resip
{

class WsConnection :  public TcpConnection, public WsConnectionBase
{
   public:
      WsConnection(Transport* transport,
                   const Tuple& who, Socket fd,
                   Compression &compression,
                   SharedPtr<WsConnectionValidator> wsConnectionValidator);

   private:
      /// No default c'tor
      WsConnection();
};

}
#endif
