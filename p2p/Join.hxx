#if !defined(P2P_JOIN_HXX)
#define P2P_JOIN_HXX

#include "p2p/Message.hxx"

namespace p2p 
{

class JoinAnsMessage : public Message
{
public:
    JoinAnsMessage(resip::Data overlaySpecific);
    virtual MessageType getMessageType() const { return JoinAns; }
protected:
    resip::Data mOverlaySpecific;
};


class JoinReqMessage : public Message
{
   public:
      JoinReqMessage(NodeId nodeID, const resip::Data &overlaySpecific);

      virtual MessageType getMessageType() const { return JoinReq; }
      NodeId getNodeID() const { return mNodeID; }
protected:
      NodeId mNodeID;
      resip::Data mOverlaySpecific;
};

}

#endif
