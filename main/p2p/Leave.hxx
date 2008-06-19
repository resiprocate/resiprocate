#ifndef P2P_Leave_hxx
#define P2P_Leave_hxx

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

protected:
	virtual void decodePayload(resip::DataStream &dataStream);
	LeaveAns();
};


class LeaveReq : public Message
{
public:
	friend class Message;

	LeaveReq(const DestinationId &dest, NodeId node);
      
	virtual MessageType getType() const { return Message::LeaveReqType; }
	virtual void getEncodedPayload(resip::DataStream &data);

protected:
	virtual void decodePayload(resip::DataStream &dataStream);
	LeaveReq();

	NodeId mNode;
};

}

#endif
