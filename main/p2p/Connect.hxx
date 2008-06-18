#ifndef P2P_CONNECT_HXX
#define P2P_CONNECT_HXX

#include "p2p/Message.hxx"

namespace p2p
{

class ConnectReq : public Message
{
public:
//	ConnectReq(const resip::Data &frag, const resip::Data &password, UInt16 port, const resip::Data &role, const std::vector<resip::Data> &candidates);
	ConnectReq() {}

	virtual MessageType getType() const { return ConnectReqType; }
	virtual void getEncodedPayload(resip::DataStream &dataStream) const { }
};

class ConnectAns : public Message
{
public:
	virtual MessageType getType() const { return ConnectAnsType; }
	virtual void getEncodedPayload(resip::DataStream &dataStream) const { }
};


}

#endif
