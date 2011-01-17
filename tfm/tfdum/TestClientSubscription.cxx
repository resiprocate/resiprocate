#include "resip/dum/DialogUsageManager.hxx"
#include "tfm/TestEndPoint.hxx"
#include "tfm/CommonAction.hxx"
#include "TestClientSubscription.hxx"
#include "DumUserAgent.hxx"
#include "resip/dum/ClientSubscription.hxx"
#include "DumExpect.hxx"

#include "resip/dum/Handles.hxx"

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;

TestClientSubscription::TestClientSubscription(DumUserAgent* ua)
   : TestUsage(ua)
{
}

CommonAction* 
TestClientSubscription::acceptUpdate(int statusCode)
{
   return new CommonAction(mUa, "acceptUpdate", 
                           boost::bind(&ClientSubscription::acceptUpdate, boost::bind<ClientSubscription*>(&ClientSubscriptionHandle::get, boost::ref(mHandle)), 
                                       statusCode));
}

CommonAction* 
TestClientSubscription::rejectUpdate(int statusCode, const Data& reasonPhrase)
{
   return new CommonAction(mUa, "rejectUpdate", 
                           boost::bind(&ClientSubscription::rejectUpdate, boost::bind<ClientSubscription*>(&ClientSubscriptionHandle::get, boost::ref(mHandle)),
                                       statusCode, reasonPhrase));
}

CommonAction* 
TestClientSubscription::requestRefresh(int expires)
{
   return new CommonAction(mUa, "requestRefresh", 
                           boost::bind(&ClientSubscription::requestRefresh, boost::bind<ClientSubscription*>(&ClientSubscriptionHandle::get, boost::ref(mHandle)),
                                       expires));
}

CommonAction* 
TestClientSubscription::end()
{
   return new CommonAction(mUa, "end", boost::bind(&ClientSubscription::end, boost::bind<ClientSubscription*>(&ClientSubscriptionHandle::get, boost::ref(mHandle))));
}

bool 
TestClientSubscription::isMyEvent(Event* e)
{
   ClientSubscriptionEvent* sub = dynamic_cast<ClientSubscriptionEvent*>(e);
   if (sub)
   {
      if (mHandle.isValid())
      {
         StackLog(<< "My handle id: " << mHandle.getId() << " " 
                 << "Compared handle id: " << sub->getHandle().getId() << " " << *e);
         return sub->getHandle() == mHandle;
      }
      else
      {
         StackLog(<< "Handle has not been bound yet: " << *e);
         
         return true; //!dcm! -- not ideal, may have to suffice until dumv2
      }
   }
   else
   {
      StackLog(<< "not a ClientSubscriptionEvent");
   }
   return false;
}

TestClientSubscription::ExpectBase* 
TestClientSubscription::expect(ClientSubscriptionEvent::Type t, 
                               MessageMatcher* matcher, 
                               ExpectPreCon& pred,
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientSubscriptionEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction);
}

TestClientSubscription::ExpectBase* 
TestClientSubscription::expect(ClientSubscriptionEvent::Type t, 
                               ExpectPreCon& pred, 
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientSubscriptionEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction);
}

TestClientSubscription::ExpectBase* 
TestClientSubscription::expect(ClientSubscriptionEvent::Type t, 
                               MessageMatcher* matcher, 
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientSubscriptionEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction);
}

