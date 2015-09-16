#include "p2p/Dispatcher.hxx"

#include "p2p/P2PSubsystem.hxx"
#include "p2p/EventWrapper.hxx"
#include "p2p/ForwardingLayer.hxx"
#include "rutil/Logger.hxx"
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
      resip_assert(0);
   }
}

void 
Dispatcher::send(std::auto_ptr<Message> message, Postable<Event>& postable)
{
   //.dcm. more with timers
   
   mTidMap[message->getTransactionId()] = Entry(postable);
   mForwardingLayer->forward(message);
}

static const resip::Data NO_HANDLER("Message not understood");

void 
Dispatcher::post(std::auto_ptr<Message> message)
{
   DebugLog(<<"Dispatcher received " << message->brief());
   
   Registry::iterator it = mRegistry.find(message->getType());
   if (it == mRegistry.end())
   {
      if (message->isRequest())
      {
         mForwardingLayer->forward(std::auto_ptr<Message>(
                                      message->makeErrorResponse(Message::Error::Forbidden, 
                                                                 NO_HANDLER)));
      } 
      else 
      {
         TidMap::iterator id = mTidMap.find(message->getTransactionId());
         if (id !=  mTidMap.end())
         {
            Entry& entry = id->second;
            entry.mPostable->post(message.release()->event());
            mTidMap.erase(id);
         }
         else
         {
            DebugLog(<< "Unexpected response, dropping " 
                     << message->brief());
         }
      }
   } 
   else 
   {
      it->second->post(message.release()->event());
   }
}

