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

IncomingApplicationMessage*
TransporterMessage::castIncomingApplicationMessage()
{
   if (getMessageType() == IncomingApplicationMessageType)
   {
      return static_cast<IncomingApplicationMessage*>(this);
   }
   return 0;
}

IncomingReloadMessage*
TransporterMessage::castIncomingReloadMessage()
{
   if (getMessageType() == IncomingReloadMessageType)
   {
      return static_cast<IncomingReloadMessage*>(this);
   }
   return 0;
}

}
