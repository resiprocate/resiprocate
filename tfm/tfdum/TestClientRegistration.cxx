#include "resip/dum/DialogUsageManager.hxx"
#include "tfm/TestEndPoint.hxx"
#include "tfm/CommonAction.hxx"
#include "TestClientRegistration.hxx"
#include "DumUserAgent.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "DumExpect.hxx"

#include "resip/dum/Handles.hxx"

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;

TestClientRegistration::TestClientRegistration(DumUserAgent* ua)
   : TestUsage(ua)
{
}

CommonAction*
TestClientRegistration::addBinding(const resip::NameAddr& contact)
{
   return new CommonAction(mUa, "addBinding",
                           boost::bind(&ClientRegistration::addBinding, boost::bind<ClientRegistration*>(&ClientRegistrationHandle::get, boost::ref(mHandle)),
                                       contact));
}

CommonAction*
TestClientRegistration::addBinding(const resip::NameAddr& contact, int registrationTime)
{
   return new CommonAction(mUa, "addBinding",
                           boost::bind(&ClientRegistration::addBinding, boost::bind<ClientRegistration*>(&ClientRegistrationHandle::get, boost::ref(mHandle)),
                                       contact, registrationTime));
}

CommonAction*
TestClientRegistration::removeBinding(const resip::NameAddr& contact)
{
   return new CommonAction(mUa, "removeBinding",
                           boost::bind(&ClientRegistration::removeBinding, boost::bind<ClientRegistration*>(&ClientRegistrationHandle::get, boost::ref(mHandle)),
                                       contact));
}

CommonAction* 
TestClientRegistration::removeAll(bool stopRegisteringWhenDone)
{
   return new CommonAction(mUa, "removeAll", 
                           boost::bind(&ClientRegistration::removeAll, boost::bind<ClientRegistration*>(&ClientRegistrationHandle::get, boost::ref(mHandle)),
                                       stopRegisteringWhenDone));
}

CommonAction* 
TestClientRegistration::removeMyBindings(bool stopRegisteringWhenDone)
{
   return new CommonAction(mUa, "removeMyBindings", 
                           boost::bind(&ClientRegistration::removeMyBindings, boost::bind<ClientRegistration*>(&ClientRegistrationHandle::get, boost::ref(mHandle)),
                                       stopRegisteringWhenDone));
}

CommonAction* 
TestClientRegistration::requestRefresh(int expires)
{
   return new CommonAction(mUa, "requestRefresh", 
                           boost::bind(&ClientRegistration::requestRefresh, boost::bind<ClientRegistration*>(&ClientRegistrationHandle::get, boost::ref(mHandle)),
                                       expires));
}

CommonAction* 
TestClientRegistration::stopRegistering()
{
   return new CommonAction(mUa, "stopRegistering", 
                           boost::bind(&ClientRegistration::stopRegistering, boost::bind<ClientRegistration*>(&ClientRegistrationHandle::get, boost::ref(mHandle))));
}

CommonAction* 
TestClientRegistration::end()
{
   return new CommonAction(mUa, "end", 
                           boost::bind(&ClientRegistration::end, boost::bind<ClientRegistration*>(&ClientRegistrationHandle::get, boost::ref(mHandle))));
}

bool 
TestClientRegistration::isMyEvent(Event* e)
{
   ClientRegistrationEvent* reg = dynamic_cast<ClientRegistrationEvent*>(e);
   if (reg)
   {
      StackLog(<< "My handle id: " << mHandle.getId());
      StackLog(<< "Compared handle id: " << reg->getHandle().getId());
      return reg->getHandle() == mHandle;
   }
   else
   {
      StackLog(<< "not a ClientRegistrationEvent");
   }
   return false;
}

TestClientRegistration::ExpectBase* 
TestClientRegistration::expect(ClientRegistrationEvent::Type t, 
                               MessageMatcher* matcher, 
                               ExpectPreCon& pred,
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientRegistrationEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction);
}

TestClientRegistration::ExpectBase* 
TestClientRegistration::expect(ClientRegistrationEvent::Type t, 
                               ExpectPreCon& pred, 
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientRegistrationEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction);
}

TestClientRegistration::ExpectBase* 
TestClientRegistration::expect(ClientRegistrationEvent::Type t, 
                               MessageMatcher* matcher, 
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientRegistrationEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction);
}
