#ifndef P2P_LEAVE_HXX
#define P2P_LEAVE_HXX

#include "p2p/Message.hxx"

namespace p2p
{

class LeaveReq;

class LeaveAns: public Message, private s2c::LeaveReqStruct
{
public:
	friend class Message;

	LeaveAns(p2p::LeaveReq *request);

	virtual MessageType getType() const { return Message::LeaveAnsType; }
	virtual void getEncodedPayload(resip::DataStream &data);
	virtual void decodePayload(resip::DataStream &dataStream);

protected:
	LeaveAns();
};


class LeaveReq : public Message
{
public:
	friend class Message;

	LeaveReq(NodeId node);
      
	virtual MessageType getType() const { return Message::LeaveReqType; }
	virtual void getEncodedPayload(resip::DataStream &data);
	virtual void decodePayload(resip::DataStream &dataStream);

protected:
	LeaveReq();

	NodeId mNode;
};

}

#endif
