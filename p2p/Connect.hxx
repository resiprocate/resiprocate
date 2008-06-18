#ifndef P2P_CONNECT_HXX
#define P2P_CONNECT_HXX

#include "p2p/Message.hxx"

namespace p2p
{

class ConnectReqMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return ConnectReqType; }
};

class ConnectAnsMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return ConnectAnsType; }
};


}

#endif
