#include "resip/dum/DialogUsageManager.hxx"
#include "tfm/TestEndPoint.hxx"
#include "tfm/CommonAction.hxx"
#include "TestClientRegistration.hxx"
#include "DumUserAgent.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "DumExpect.hxx"

#include "resip/dum/Handles.hxx"

#include <functional>

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
                           std::bind(static_cast<void(ClientRegistration::*)(const NameAddr& contact)>(&ClientRegistration::addBinding), std::bind<ClientRegistration*>(static_cast<ClientRegistration*(ClientRegistrationHandle::*)()>(&ClientRegistrationHandle::get), std::ref(mHandle)),
                                       contact));
}

CommonAction*
TestClientRegistration::addBinding(const resip::NameAddr& contact, uint32_t registrationTime)
{
   return new CommonAction(mUa, "addBinding",
                           std::bind(static_cast<void(ClientRegistration::*)(const NameAddr& contact, uint32_t registrationTime)>(&ClientRegistration::addBinding), std::bind<ClientRegistration*>(static_cast<ClientRegistration*(ClientRegistrationHandle::*)()>(&ClientRegistrationHandle::get), std::ref(mHandle)),
                                       contact, registrationTime));
}

CommonAction*
TestClientRegistration::removeBinding(const resip::NameAddr& contact)
{
   return new CommonAction(mUa, "removeBinding",
                           std::bind(&ClientRegistration::removeBinding, std::bind<ClientRegistration*>(static_cast<ClientRegistration*(ClientRegistrationHandle::*)()>(&ClientRegistrationHandle::get), std::ref(mHandle)),
                                       contact));
}

CommonAction* 
TestClientRegistration::removeAll(bool stopRegisteringWhenDone)
{
   return new CommonAction(mUa, "removeAll", 
                           std::bind(&ClientRegistration::removeAll, std::bind<ClientRegistration*>(static_cast<ClientRegistration*(ClientRegistrationHandle::*)()>(&ClientRegistrationHandle::get), std::ref(mHandle)),
                                       stopRegisteringWhenDone));
}

CommonAction* 
TestClientRegistration::removeMyBindings(bool stopRegisteringWhenDone)
{
   return new CommonAction(mUa, "removeMyBindings", 
                           std::bind(&ClientRegistration::removeMyBindings, std::bind<ClientRegistration*>(static_cast<ClientRegistration*(ClientRegistrationHandle::*)()>(&ClientRegistrationHandle::get), std::ref(mHandle)),
                                       stopRegisteringWhenDone));
}

CommonAction* 
TestClientRegistration::requestRefresh(int expires)
{
   return new CommonAction(mUa, "requestRefresh", 
                           std::bind(&ClientRegistration::requestRefresh, std::bind<ClientRegistration*>(static_cast<ClientRegistration*(ClientRegistrationHandle::*)()>(&ClientRegistrationHandle::get), std::ref(mHandle)),
                                       expires));
}

CommonAction* 
TestClientRegistration::stopRegistering()
{
   return new CommonAction(mUa, "stopRegistering", 
                           std::bind(&ClientRegistration::stopRegistering, std::bind<ClientRegistration*>(static_cast<ClientRegistration*(ClientRegistrationHandle::*)()>(&ClientRegistrationHandle::get), std::ref(mHandle))));
}

CommonAction* 
TestClientRegistration::end()
{
   return new CommonAction(mUa, "end", 
                           std::bind(&ClientRegistration::end, std::bind<ClientRegistration*>(static_cast<ClientRegistration*(ClientRegistrationHandle::*)()>(&ClientRegistrationHandle::get), std::ref(mHandle))));
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
