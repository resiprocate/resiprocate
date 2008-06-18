#ifndef P2P_EventWrapper_hxx
#define P2P_EventWrapper_hxx

#include "p2p/p2p.hxx"
#include "p2p/Event.hxx"
#include "p2p/EventConsumer.hxx"

#include <memory>

namespace p2p
{

template <class T> 
class EventWrapper : public Event
{
   public:
      EventWrapper(T* t) : mWrapped()
      {}
      
      virtual void dispatch(EventConsumer& consumer) 
      {
         consumer.consume(*mWrapped);
      }

   private:
      std::auto_ptr<T> mWrapped;
};

template <class T>
static std::auto_ptr<Event> wrap(T* t) 
{
   return std::auto_ptr<Event>(new EventWrapper<T>(t));
}

}

#endif // P2P_Event_hxx
