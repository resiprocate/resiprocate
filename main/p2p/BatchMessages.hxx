#ifndef P2P_BatchMessages_hxx
#define P2P_BatchMessages_hxx

#include <memory>
#include <vector>

#include "EventConsumer.hxx"
#include "Event.hxx"

namespace p2p
{

class Dispatcher;

class BatchMessages : public EventConsumer, public Event
{
      BatchMessages(Dispatcher& dispatcher,
                    std::vector<std::auto_ptr<Message> >& messages,
                    Postable<Event>& postable);
      virtual ~BatchMessages() = 0;
      
      virtual void onFailure() = 0;
      virtual void onSuccess() = 0;
      // called by creator on consumption
      void completed();

   protected:
      virtual void consume(Message& message);
   private:
      Postable<Event>* mPostable;
      int mResponseCount;
      bool mSucceeded;
};
   
}

#endif
