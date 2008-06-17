#ifndef P2P_UPDATE_HXX
#define P2P_UPDATE_HXX

#include "p2p/Message.hxx"

namespace p2p
{

class UpdateAnsMessage : public Message
{
public:
	virtual MessageType getMessageType() const { return UpdateAns; }
};


class UpdateReqMessage : public Message
{
public:
	UpdateReqMessage(resip::Data overlaySpecificBlob);
	virtual MessageType getMessageType() const { return UpdateReq; }
protected:
	resip::Data mOverlaySpecific;
};

}

#endif
