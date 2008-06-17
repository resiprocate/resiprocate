#ifndef P2P_Event_hxx
#define P2P_Event_hxx

namespace p2p
{

class EventConsumer
{
   public:
      virtual void consumer(CertDoneEvent& certdone);
};

}

#endif
