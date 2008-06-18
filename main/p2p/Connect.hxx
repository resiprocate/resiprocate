#ifndef P2P_CONNECT_HXX
#define P2P_CONNECT_HXX

#include "p2p/Message.hxx"
#include "p2p/ConnectBase.hxx"

namespace p2p
{

class ConnectReq : public ConnectBase
{
public:
	ConnectReq(const resip::Data &frag, const resip::Data &password, UInt16 application, const resip::Data &role, const std::vector<resip::Data> &candidates);
	virtual MessageType getType() const { return ConnectReqType; }

protected:
	friend class Message;

	ConnectReq();
};

class ConnectAns : public ConnectBase
{
public:

	virtual MessageType getType() const { return ConnectAnsType; }

protected:
	friend class Message;

	// should use Message::makeResponse
	ConnectAns(Message *req, const resip::Data &frag, const resip::Data &password, UInt16 application, const resip::Data &role, const std::vector<resip::Data> &candidates);
	ConnectAns();
};


}

#endif
