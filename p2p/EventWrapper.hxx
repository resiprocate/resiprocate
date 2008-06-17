#ifndef P2P_EventWrapper_hxx
#define P2P_EventWrapper_hxx

#include "p2p/p2p.hxx"
#include "p2p/EventConsumer.hxx"

#include <memory>

namespace p2p
{

template <class T> class EventWrapper : public Event
{
   public:
      EventWrapper(Event* event) 
         : mEvent(event)
      {}

      static EventWrapper* wrap(T* event) 
      {
         return new EventWrapper(event);
      }
      
      T& get();
      const T& get() const;
      virtual void dispatch(EventConsumer& consumer) 
      {
         consumer.consume(this);
      }

   private:
      std::auto_ptr<T> mEvent;
};

}

#endif // P2P_Event_hxx
