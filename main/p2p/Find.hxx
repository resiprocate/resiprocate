#ifndef P2P_Find_hxx
#define P2P_Find_hxx

#include "p2p/Message.hxx"

namespace p2p 
{

class FindAns : public Message
{
   public:
      virtual MessageType getMessageType() const { return FindAnsType; }
};


class FindReq : public Message
{
   public:
      virtual MessageType getMessageType() const { return FindReqType; }
};

}

#endif
