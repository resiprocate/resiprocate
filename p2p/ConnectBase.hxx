#ifndef P2P_CONNECTBASE_HXX
#define P2P_CONNECTBASE_HXX

#include "p2p/Message.hxx"
#include "p2p/MessageHelper.hxx"

namespace p2p
{

class ConnectBase : public Message, protected s2c::ConnectReqAnsStruct
{
public:
	friend class Message;

	ConnectBase(const resip::Data &frag, const resip::Data &password, UInt16 application, const resip::Data &role, const std::vector<resip::Data> &candidates);
	virtual void getEncodedPayload(resip::DataStream &dataStream);
protected:
   virtual void decodePayload(resip::DataStream &dataStream);
	ConnectBase();
};

}

#endif
