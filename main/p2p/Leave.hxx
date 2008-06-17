#ifndef P2P_LEAVE_HXX
#define P2P_LEAVE_HXX

#include "p2p/Message.hxx"

namespace p2p
{

class LeaveAns: public Message
{
   public:
	LeaveAns() {}

	virtual MessageType getMessageType() const { return Message::LeaveAns; }
    virtual void getPayload(resip::Data &data) const;
};


class LeaveReq : public Message
{
   public:
   	LeaveReq(NodeId node);

	virtual MessageType getMessageType() const { return Message::LeaveReq; }
    virtual void getPayload(resip::Data &data) const;
protected:
	NodeId mNode;
};

}

#endif
