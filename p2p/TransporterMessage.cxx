#include "p2p/TransporterMessage.hxx"

namespace p2p
{

NewConnection*
TransporterMessage::castNewConnection()
{
   if (getMessageType() == NewConnectionType)
   {
      return static_cast<NewConnection*>(this);
   }
   return 0;
}

ClosedConnection*
TransporterMessage::castClosedConnection()
{
   if (getMessageType() == ClosedConnectionType)
   {
      return static_cast<ClosedConnection*>(this);
   }
   return 0;
}

IncomingMessage*
TransporterMessage::castIncomingMessage()
{
   if (getMessageType() == IncomingMessageType)
   {
      return static_cast<IncomingMessage*>(this);
   }
   return 0;
}

}
