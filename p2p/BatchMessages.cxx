#include "BatchMessages.hxx"

#include "p2p/P2PSubsystem.hxx"
#include "p2p/EventWrapper.hxx"
#include "p2p/Dispatcher.hxx"

#define RESIPROCATE_SUBSYSTEM P2PSubsystem::P2P

using namespace p2p;

BatchMessages::BatchMessages(Dispatcher& dispatcher,
                             std::vector<std::auto_ptr<Message> >& messages,
                             Postable<Event>& postable)
   : mPostable(&postable),
     mResponseCount(messages.size()),
     mSucceeded(true)
{
   for (std::vector<std::auto_ptr<Message> >::iterator i = messages.begin();
        i != messages.end(); i++) 
   {
      dispatcher.sendAsBaseMessage(*i, *this);
   }
}

BatchMessages::~BatchMessages() 
{}

void 
BatchMessages::consume(Message& message) 
{
   if (message.getType() == Message::FailureResponseType) 
   {
      mSucceeded = false;
   }
   mResponseCount--;
   if (mResponseCount == 0) 
   {
      mPostable->post(std::auto_ptr<Event>(this));
   }
}

void
BatchMessages::completed() 
{
   if (mSucceeded) 
   {
      onSuccess();
   }
   else
   {
      onFailure();
   }
}

