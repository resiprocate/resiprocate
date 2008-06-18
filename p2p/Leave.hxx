#ifndef P2P_LEAVE_HXX
#define P2P_LEAVE_HXX

#include "p2p/Message.hxx"

namespace p2p
{

class LeaveReq;

class LeaveAns: public Message
{
   public:
	LeaveAns(p2p::LeaveReq *request);

	virtual MessageType getType() const { return Message::LeaveAnsType; }
    virtual void getEncodedPayload(resip::DataStream &data) const;
};


class LeaveReq : public Message
{
   public:
   	LeaveReq(NodeId node);

	virtual MessageType getType() const { return Message::LeaveReqType; }
    virtual void getEncodedPayload(resip::DataStream &data) const;
protected:
	NodeId mNode;
};

}

#endif
