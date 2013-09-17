#include "rutil/Logger.hxx"
#include "rutil/Socket.hxx"
#include "resip/stack/TcpConnection.hxx"
#include "resip/stack/WsConnection.hxx"
#include "resip/stack/Tuple.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

WsConnection::WsConnection(Transport* transport,
                           const Tuple& who, Socket fd,
                           Compression &compression,
                           SharedPtr<WsConnectionValidator> wsConnectionValidator)
  : TcpConnection(transport,who, fd, compression), WsConnectionBase(wsConnectionValidator)
{
   DebugLog (<< "Creating WS connection " << who << " on " << fd);
}
