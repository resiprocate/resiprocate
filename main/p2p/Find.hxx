#ifndef P2P_FIND_HXX
#define P2P_FIND_HXX

#include "p2p/Message.hxx"

namespace p2p 
{

class FindAnsMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return FindAns; }
};


class FindReqMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return FindReq; }
};

}

#endif
