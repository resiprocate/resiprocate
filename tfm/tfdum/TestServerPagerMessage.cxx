#include "resip/dum/DialogUsageManager.hxx"
#include "tfm/TestEndPoint.hxx"
#include "tfm/CommonAction.hxx"
#include "TestServerPagerMessage.hxx"
#include "DumUserAgent.hxx"
#include "resip/dum/ServerPagerMessage.hxx"
#include "DumExpect.hxx"

#include "resip/dum/Handles.hxx"

#include <functional>

#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;

TestServerPagerMessage::TestServerPagerMessage(DumUserAgent* ua)
   : TestUsage(ua)
{
}

SendingAction<ServerPagerMessageHandle>*
TestServerPagerMessage::accept(int statusCode)
{
   return new SendingAction<ServerPagerMessageHandle>(mUa, mHandle, "accept", 
                                                      std::bind(&ServerPagerMessage::accept, 
                                                                  std::bind<ServerPagerMessage*>(static_cast<ServerPagerMessage*(ServerPagerMessageHandle::*)()>(&ServerPagerMessageHandle::get), std::ref(mHandle)), statusCode), 
                                                      NoAdornment::instance());
}

SendingAction<ServerPagerMessageHandle>*
TestServerPagerMessage::reject(int responseCode)
{
   return new SendingAction<ServerPagerMessageHandle>(mUa, mHandle, "reject",
                                                      std::bind(&ServerPagerMessage::reject, 
                                                                  std::bind<ServerPagerMessage*>(static_cast<ServerPagerMessage*(ServerPagerMessageHandle::*)()>(&ServerPagerMessageHandle::get), std::ref(mHandle)), 
                                                                  responseCode),
                                                      NoAdornment::instance());
}

CommonAction* 
TestServerPagerMessage::end()
{
   return new CommonAction(mUa, "end", std::bind(&ServerPagerMessage::endCommand, std::bind<ServerPagerMessage*>(static_cast<ServerPagerMessage*(ServerPagerMessageHandle::*)()>(&ServerPagerMessageHandle::get), std::ref(mHandle))));
}
 
CommonAction* 
TestServerPagerMessage::send(std::shared_ptr<SipMessage> msg)
{
   return new CommonAction(mUa, "send", 
                           std::bind(&ServerPagerMessage::sendCommand, std::bind<ServerPagerMessage*>(static_cast<ServerPagerMessage*(ServerPagerMessageHandle::*)()>(&ServerPagerMessageHandle::get), std::ref(mHandle)), 
                                       msg));
}

bool 
TestServerPagerMessage::isMyEvent(Event* e)
{
   ServerPagerMessageEvent* sub = dynamic_cast<ServerPagerMessageEvent*>(e);
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
      DebugLog(<< "not a ServerPagerMessageEvent");
   }
   return false;
}

TestServerPagerMessage::ExpectBase* 
TestServerPagerMessage::expect(ServerPagerMessageEvent::Type t, 
                               MessageMatcher* matcher, 
                               ExpectPreCon& pred,
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerPagerMessageEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction);
}

TestServerPagerMessage::ExpectBase* 
TestServerPagerMessage::expect(ServerPagerMessageEvent::Type t, 
                               ExpectPreCon& pred, 
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerPagerMessageEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction);
}

TestServerPagerMessage::ExpectBase* 
TestServerPagerMessage::expect(ServerPagerMessageEvent::Type t, 
                               MessageMatcher* matcher, 
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerPagerMessageEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction);
}

