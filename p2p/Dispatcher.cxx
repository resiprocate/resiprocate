#include "p2p/Dispatcher.hxx"

#include "p2p/P2PSubsystem.hxx"
#include "p2p/ForwardingLayer.hxx"
#include "p2p/Message.hxx"

#define RESIPROCATE_SUBSYSTEM P2PSubsystem::P2P

using namespace p2p;

Dispatcher::Dispatcher()
   : mForwardingLayer(0)
{}

void
Dispatcher::init(ForwardingLayer& forwardingLayer) 
{
   mForwardingLayer = &forwardingLayer;
}

void 
Dispatcher::registerPostable(Message::MessageType type,
                             Postable<Event>& postable)
{
   Registry::iterator it = mRegistry.find(type);
   if (it == mRegistry.end())
   {
      mRegistry[type] = &postable;
   }
   else
   {
      assert(0);
   }
}

void 
Dispatcher::send(std::auto_ptr<Message> message)
{
   //.dcm. more with timers
   mForwardingLayer->post(message);
}

static const resip::Data NO_HANDLER("Message not understood");

void 
Dispatcher::post(std::auto_ptr<Message> message)
{
   Registry::iterator it = mRegistry.find(message->getType());
   if (it == mRegistry.end())
   {
      if (message->isRequest())
      {
         mForwardingLayer->post(std::auto_ptr<Message>(
                                message->makeErrorResponse(Message::Error::Forbidden, 
                                NO_HANDLER)));
      } 
      else 
      {
         DebugLog(<< "Response for unregistered message type, dropping " 
                  << message->brief());
      }
   } 
   else 
   {
      it->second->post(message->event());
   }
}

