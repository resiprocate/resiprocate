#ifndef P2P_Event_hxx
#define P2P_Event_hxx

class Event 
{
   public:
      virtual void dispatch(EventConsumer& consumer) = 0;
};

#endif // P2P_Event_hxx
