#ifndef P2P_LEAVE_HXX
#define P2P_LEAVE_HXX

#include "p2p/Message.hxx"

namespace p2p
{

class LeaveAnsMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return LeaveAns; }
};


class LeaveReqMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return LeaveReq; }
};

}

#endif
