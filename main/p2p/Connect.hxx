#ifndef P2P_CONNECT_HXX
#define P2P_CONNECT_HXX

#include "p2p/Message.hxx"

namespace p2p
{

class ConnectReqMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return ConnectReq; }
};

class ConnectAnsMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return ConnectAns; }
};


}

#endif
