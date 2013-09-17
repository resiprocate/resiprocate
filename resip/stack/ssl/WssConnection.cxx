#include "rutil/Logger.hxx"
#include "rutil/Socket.hxx"
#include "resip/stack/ssl/TlsConnection.hxx"
#include "resip/stack/ssl/WssConnection.hxx"
#include "resip/stack/Tuple.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

WssConnection::WssConnection(Transport* transport, const Tuple& who, Socket fd,
                              Security* security, bool server, Data domain,
                              SecurityTypes::SSLType sslType , Compression &compression,
                              SharedPtr<WsConnectionValidator> wsConnectionValidator)
  : TlsConnection(transport, who, fd, security, server, domain, sslType, compression),
    WsConnectionBase(wsConnectionValidator)
{
   DebugLog (<< "Creating WSS connection " << who << " on " << fd);
}
