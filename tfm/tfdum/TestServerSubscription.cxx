#include "resip/dum/DialogUsageManager.hxx"
#include "tfm/TestEndPoint.hxx"
#include "tfm/CommonAction.hxx"
#include "TestServerSubscription.hxx"
#include "DumUserAgent.hxx"
#include "DumExpect.hxx"
#include "DumUaAction.hxx"

#include "resip/dum/Handles.hxx"

#include <functional>

#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;

TestServerSubscription::TestServerSubscription(DumUserAgent* ua)
   : TestUsage(ua)
{
}

SendingAction<ServerSubscriptionHandle>*
TestServerSubscription::accept(int statusCode)
{
   return new SendingAction<ServerSubscriptionHandle>(mUa, mHandle, "accept", 
                                                      std::bind(&ServerSubscription::accept, 
                                                                  std::bind<ServerSubscription*>(static_cast<ServerSubscription*(ServerSubscriptionHandle::*)()>(&ServerSubscriptionHandle::get), std::ref(mHandle)), statusCode), 
                                                      NoAdornment::instance());
}

SendingAction<ServerSubscriptionHandle>*
TestServerSubscription::reject(int responseCode)
{
   return new SendingAction<ServerSubscriptionHandle>(mUa, mHandle, "reject",
                                                      std::bind(&ServerSubscription::reject, 
                                                                  std::bind<ServerSubscription*>(static_cast<ServerSubscription*(ServerSubscriptionHandle::*)()>(&ServerSubscriptionHandle::get), std::ref(mHandle)), 
                                                                  responseCode),
                                                      NoAdornment::instance());
}

SendingAction<ServerSubscriptionHandle>*
TestServerSubscription::neutralNotify()
{
   return new SendingAction<ServerSubscriptionHandle>(mUa, mHandle, "neutralNotify",
                                                      std::bind(&ServerSubscription::neutralNotify, 
                                                                  std::bind<ServerSubscription*>(static_cast<ServerSubscription*(ServerSubscriptionHandle::*)()>(&ServerSubscriptionHandle::get), std::ref(mHandle))), 
                                                      NoAdornment::instance());
}
                                                                                    
SendingAction<ServerSubscriptionHandle>* 
TestServerSubscription::update(const resip::Contents* document)
{
   return new SendingAction<ServerSubscriptionHandle>(mUa, mHandle, "update",
                                                      std::bind(&ServerSubscription::update, 
                                                                  std::bind<ServerSubscription*>(static_cast<ServerSubscription*(ServerSubscriptionHandle::*)()>(&ServerSubscriptionHandle::get), std::ref(mHandle)), document), 
                                                      NoAdornment::instance());
}

CommonAction* 
TestServerSubscription::setSubscriptionState(resip::SubscriptionState state)
{
   return new CommonAction(mUa, "setSubscriptionState", 
                           std::bind(&ServerSubscription::setSubscriptionState, 
                                       std::bind<ServerSubscription*>(static_cast<ServerSubscription*(ServerSubscriptionHandle::*)()>(&ServerSubscriptionHandle::get), std::ref(mHandle)), 
                                       state));
}

CommonAction* 
TestServerSubscription::end()
{
   return new CommonAction(mUa, "end", std::bind(&ServerSubscription::end, std::bind<ServerSubscription*>(static_cast<ServerSubscription*(ServerSubscriptionHandle::*)()>(&ServerSubscriptionHandle::get), std::ref(mHandle))));
}
 
CommonAction* 
TestServerSubscription::send(std::shared_ptr<SipMessage> msg)
{
   return new CommonAction(mUa, "send", 
                           std::bind(&ServerSubscription::send, std::bind<ServerSubscription*>(static_cast<ServerSubscription*(ServerSubscriptionHandle::*)()>(&ServerSubscriptionHandle::get), std::ref(mHandle)), 
                                       msg));
}

bool 
TestServerSubscription::isMyEvent(Event* e)
{
   ServerSubscriptionEvent* sub = dynamic_cast<ServerSubscriptionEvent*>(e);
   if (sub)
   {
      if (mHandle.isValid())
      {
         DebugLog(<< "My handle id: " << mHandle.getId() << " " 
                 << "Compared handle id: " << sub->getHandle().getId() << " " << *e);
         return sub->getHandle() == mHandle;
      }
      else
      {
         DebugLog(<< "Handle has not been bound yet: " << *e);
         
         return true; //!dcm! -- not ideal, may have to suffice until dumv2
      }
   }
   else
   {
      DebugLog(<< "not a ServerSubscriptionEvent");
   }
   return false;
}

TestServerSubscription::ExpectBase* 
TestServerSubscription::expect(ServerSubscriptionEvent::Type t, 
                               MessageMatcher* matcher, 
                               ExpectPreCon& pred,
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerSubscriptionEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction);
}

TestServerSubscription::ExpectBase* 
TestServerSubscription::expect(ServerSubscriptionEvent::Type t, 
                               ExpectPreCon& pred, 
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerSubscriptionEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction);
}

TestServerSubscription::ExpectBase* 
TestServerSubscription::expect(ServerSubscriptionEvent::Type t, 
                               MessageMatcher* matcher, 
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerSubscriptionEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction);
}

