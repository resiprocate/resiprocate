#ifndef P2P_CONNECT_HXX
#define P2P_CONNECT_HXX

#include "p2p/Message.hxx"
#include "p2p/ConnectBase.hxx"
#include "p2p/EventWrapper.hxx"
namespace p2p
{

class ConnectReq : public ConnectBase
{
public:
	ConnectReq(const DestinationId &dest, const resip::Data &frag, const resip::Data &password, UInt16 application, const resip::Data &role, const std::vector<Candidate> &candidates);
	virtual MessageType getType() const { return ConnectReqType; }
	virtual resip::Data brief() const { return "ConnectReq Message"; }

  std::auto_ptr<Event> event()
    {
      return wrap(this);
    }

protected:
	friend class Message;

	ConnectReq();
};

class ConnectAns : public ConnectBase
{
public:

	virtual MessageType getType() const { return ConnectAnsType; }
	virtual resip::Data brief() const { return "ConnectAns Message"; }

  std::auto_ptr<Event> event()
    {
      return wrap(this);
    }

protected:
	friend class Message;

	// should use Message::makeResponse
	ConnectAns(Message *req, const resip::Data &frag, const resip::Data &password, UInt16 application, const resip::Data &role, const std::vector<Candidate> &candidates);
	ConnectAns();
};


}

#endif
