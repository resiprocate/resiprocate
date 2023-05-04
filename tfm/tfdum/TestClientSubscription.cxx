#include "resip/dum/DialogUsageManager.hxx"
#include "tfm/TestEndPoint.hxx"
#include "tfm/CommonAction.hxx"
#include "TestClientSubscription.hxx"
#include "DumUserAgent.hxx"
#include "resip/dum/ClientSubscription.hxx"
#include "DumExpect.hxx"

#include "resip/dum/Handles.hxx"

#include <functional>

#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;

TestClientSubscription::TestClientSubscription(DumUserAgent* ua)
   : TestUsage(ua)
{
}

CommonAction* 
TestClientSubscription::acceptUpdate(int statusCode, const char* reason)
{
   return new CommonAction(mUa, "acceptUpdate", 
                           std::bind(&ClientSubscription::acceptUpdateCommand, std::bind<ClientSubscription*>(static_cast<ClientSubscription*(ClientSubscriptionHandle::*)()>(&ClientSubscriptionHandle::get), std::ref(mHandle)), 
                                       statusCode, reason));
}

CommonAction* 
TestClientSubscription::rejectUpdate(int statusCode, const Data& reasonPhrase)
{
   return new CommonAction(mUa, "rejectUpdate", 
                           std::bind(&ClientSubscription::rejectUpdateCommand, std::bind<ClientSubscription*>(static_cast<ClientSubscription*(ClientSubscriptionHandle::*)()>(&ClientSubscriptionHandle::get), std::ref(mHandle)),
                                       statusCode, reasonPhrase));
}

CommonAction* 
TestClientSubscription::requestRefresh(int expires)
{
   return new CommonAction(mUa, "requestRefresh", 
                           std::bind(&ClientSubscription::requestRefreshCommand, std::bind<ClientSubscription*>(static_cast<ClientSubscription*(ClientSubscriptionHandle::*)()>(&ClientSubscriptionHandle::get), std::ref(mHandle)),
                                       expires));
}

CommonAction* 
TestClientSubscription::end()
{
   return new CommonAction(mUa, "end", std::bind(&ClientSubscription::endCommand, std::bind<ClientSubscription*>(static_cast<ClientSubscription*(ClientSubscriptionHandle::*)()>(&ClientSubscriptionHandle::get), std::ref(mHandle)), false));
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

