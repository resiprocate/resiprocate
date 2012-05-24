#include "BatchMessages.hxx"

#include "p2p/P2PSubsystem.hxx"
#include "p2p/EventWrapper.hxx"
#include "p2p/Dispatcher.hxx"
#include "p2p/Connect.hxx"
#include "p2p/FetchAns.hxx"
#include "p2p/Find.hxx"
#include "p2p/Leave.hxx"
#include "p2p/Join.hxx"
#include "p2p/StoreAns.hxx"
#include "p2p/Update.hxx"
#include "p2p/Message.hxx"

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
      dispatcher.send(*i, *this);
   }
}

BatchMessages::~BatchMessages() 
{}

void
BatchMessages::countDown(Message& message)
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

void BatchMessages::consume(PingAns& m)
{
   countDown(m);
}

void BatchMessages::consume(ConnectAns& m)
{
   countDown(m);
}

void BatchMessages::consume(TunnelAns& m)
{
   countDown(m);
}

void BatchMessages::consume(StoreAns& m)
{
   countDown(m);
}

void BatchMessages::consume(FetchAns& m)
{
   countDown(m);
}

void BatchMessages::consume(RemoveAns& m)
{
   countDown(m);
}

void BatchMessages::consume(FindAns& m)
{
   countDown(m);
}

void BatchMessages::consume(JoinAns& m)
{
   countDown(m);
}

void BatchMessages::consume(LeaveAns& m)
{
   countDown(m);
}

void BatchMessages::consume(UpdateAns& m)
{
   countDown(m);
}

void BatchMessages::consume(RouteQueryAns& m)
{
   countDown(m);
}

void BatchMessages::consume(ErrorResponse& m)
{
   countDown(m);
}

