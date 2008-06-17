#ifndef P2P_FIND_HXX
#define P2P_FIND_HXX

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
