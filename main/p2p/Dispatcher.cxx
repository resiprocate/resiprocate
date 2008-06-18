#include "p2p/Dispatcher.hxx"

#include "rutil/P2PSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::P2P

using namespace p2p;

Dispatcher::Dispatcher(ForwardingLayer& forwardingLayer)
   : mForwardingLayer(forwardingLayer)
{
   
}

void 
Dispatcher::registerPostable(Message::MessageType type,
                             Postable<Event>& postable)
{
   Registry::iterator it == mRegistry.find(message->getType());
   if (it == mRegistry.end())
   {
      mRegistry[type] = postable;
   } else {
      assert(0);
   }
}

void 
Dispatcher::send(std::auto_ptr<Message> message)
{
   //.dcm. more with timers
   mForwardingLayer->post(message);
}

static const Data NO_HANDLER("Message not understood");

void 
Dispatcher::post(std::auto_ptr<Message> message)
{
   Registry::iterator it == mRegistry.find(message->getType());
   if (it == mRegistry.end())
   {
      if (message->isRequest())
      {
         mForwardingLayer->post(auto_ptr<Message>(makeErrorResponse(Messge::Error::Forbidden, 
                                NO_HANDLER)));
      } else {
         DebugLog(<< "Response for unregistered message type, dropping " 
                  << message->brief());
      }
   } else {
      it->second->post(message->wrap());
   }
}

