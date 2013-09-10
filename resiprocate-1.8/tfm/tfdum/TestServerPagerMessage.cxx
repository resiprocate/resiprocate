#include "resip/dum/DialogUsageManager.hxx"
#include "tfm/TestEndPoint.hxx"
#include "tfm/CommonAction.hxx"
#include "TestServerPagerMessage.hxx"
#include "DumUserAgent.hxx"
#include "resip/dum/ServerPagerMessage.hxx"
#include "DumExpect.hxx"

#include "resip/dum/Handles.hxx"

#include <boost/bind.hpp>
#include <boost/function.hpp>

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
                                                      boost::bind(&ServerPagerMessage::accept, 
                                                                  boost::bind<ServerPagerMessage*>(&ServerPagerMessageHandle::get, boost::ref(mHandle)), statusCode), 
                                                      NoAdornment::instance());
}

SendingAction<ServerPagerMessageHandle>*
TestServerPagerMessage::reject(int responseCode)
{
   return new SendingAction<ServerPagerMessageHandle>(mUa, mHandle, "reject",
                                                      boost::bind(&ServerPagerMessage::reject, 
                                                                  boost::bind<ServerPagerMessage*>(&ServerPagerMessageHandle::get, boost::ref(mHandle)), 
                                                                  responseCode),
                                                      NoAdornment::instance());
}

CommonAction* 
TestServerPagerMessage::end()
{
   return new CommonAction(mUa, "end", boost::bind(&ServerPagerMessage::endCommand, boost::bind<ServerPagerMessage*>(&ServerPagerMessageHandle::get, boost::ref(mHandle))));
}
 
CommonAction* 
TestServerPagerMessage::send(resip::SharedPtr<SipMessage> msg)
{
   return new CommonAction(mUa, "send", 
                           boost::bind(&ServerPagerMessage::sendCommand, boost::bind<ServerPagerMessage*>(&ServerPagerMessageHandle::get, boost::ref(mHandle)), 
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

