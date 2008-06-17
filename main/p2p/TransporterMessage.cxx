#include "p2p/TransporterMessage.hxx"

namespace p2p
{

ConnectionOpened*
TransporterMessage::castConnectionOpened()
{
   if (getMessageType() == ConnectionOpenedType)
   {
      return static_cast<ConnectionOpened*>(this);
   }
   return 0;
}

ConnectionClosed*
TransporterMessage::castConnectionClosed()
{
   if (getMessageType() == ConnectionClosedType)
   {
      return static_cast<ConnectionClosed*>(this);
   }
   return 0;
}

ApplicationMessageArrived*
TransporterMessage::castApplicationMessageArrived()
{
   if (getMessageType() == ApplicationMessageArrivedType)
   {
      return static_cast<ApplicationMessageArrived*>(this);
   }
   return 0;
}

MessageArrived*
TransporterMessage::castMessageArrived()
{
   if (getMessageType() == MessageArrivedType)
   {
      return static_cast<MessageArrived*>(this);
   }
   return 0;
}

}
