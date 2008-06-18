#ifndef P2P_FIND_HXX
#define P2P_FIND_HXX

#include "p2p/Message.hxx"

namespace p2p 
{

class FindAnsMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return FindAnsType; }
};


class FindReqMessage : public Message
{
   public:
      virtual MessageType getMessageType() const { return FindReqType; }
};

}

#endif
