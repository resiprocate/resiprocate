#ifndef P2P_CONNECT_HXX
#define P2P_CONNECT_HXX

#include "p2p/Message.hxx"

namespace p2p
{

class ConnectReq : public Message, private s2c::ConnectReqAnsStruct
{
public:
	friend class Message;

	ConnectReq(const resip::Data &frag, const resip::Data &password, UInt16 port, const resip::Data &role, const std::vector<resip::Data> &candidates);

	virtual MessageType getType() const { return ConnectReqType; }
	virtual void getEncodedPayload(resip::DataStream &dataStream) { }
   virtual void decodePayload(resip::DataStream &dataStream);
protected:
	ConnectReq();
};

class ConnectAns : public Message, private s2c::ConnectReqAnsStruct
{
public:
	friend class Message;

	virtual MessageType getType() const { return ConnectAnsType; }
	virtual void getEncodedPayload(resip::DataStream &dataStream) { }
   virtual void decodePayload(resip::DataStream &dataStream);
protected:
	ConnectAns();
};


}

#endif
