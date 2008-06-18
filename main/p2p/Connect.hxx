#ifndef P2P_CONNECT_HXX
#define P2P_CONNECT_HXX

#include "p2p/Message.hxx"

namespace p2p
{

class ConnectReqMessage : public Message
{
   public:
      ~ConnectReqMessage() {}
      virtual MessageType getType() const { return ConnectReqType; }
      virtual void getEncodedPayload(resip::DataStream &dataStream) const { }
};

class ConnectAnsMessage : public Message
{
   public:
      ~ConnectAnsMessage() {}
      virtual MessageType getType() const { return ConnectAnsType; }
      virtual void getEncodedPayload(resip::DataStream &dataStream) const { }
};


}

#endif
