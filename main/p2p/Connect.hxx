#ifndef P2P_CONNECT_HXX
#define P2P_CONNECT_HXX

#include "p2p/Message.hxx"
#include "p2p/ConnectBase.hxx"

namespace p2p
{

class ConnectReq : public ConnectBase
{
public:
	friend class Message;

	ConnectReq(const resip::Data &frag, const resip::Data &password, UInt16 application, const resip::Data &role, const std::vector<resip::Data> &candidates);
	virtual MessageType getType() const { return ConnectReqType; }

protected:
	ConnectReq();
};

class ConnectAns : public ConnectBase
{
public:
	friend class Message;

	ConnectAns(const resip::Data &frag, const resip::Data &password, UInt16 application, const resip::Data &role, const std::vector<resip::Data> &candidates);
	virtual MessageType getType() const { return ConnectAnsType; }

protected:
	ConnectAns();
};


}

#endif
