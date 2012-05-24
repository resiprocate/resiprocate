#include "resip/dum/DialogUsageManager.hxx"
#include "tfm/TestEndPoint.hxx"
#include "tfm/CommonAction.hxx"
#include "TestClientPublication.hxx"
#include "DumUserAgent.hxx"
#include "resip/dum/ClientPublication.hxx"
#include "DumExpect.hxx"

#include "resip/dum/Handles.hxx"

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;

TestClientPublication::TestClientPublication(DumUserAgent* ua)
   : TestUsage(ua)
{
   DebugLog(<<" TestClientPublication::TestClientPublication: " << this);   
}

TestClientPublication::~TestClientPublication()
{
   DebugLog(<<"TestClientPublication::~TestClientPublication: " << this);   
}

CommonAction*
TestClientPublication::refresh(unsigned int expiration)
{
   return new CommonAction(mUa, "refresh", 
                           boost::bind(&ClientPublication::refresh, boost::bind<ClientPublication*>(&ClientPublicationHandle::get, boost::ref(mHandle)), expiration));
}

CommonAction*
TestClientPublication::update(const resip::Contents* body)
{
   return new CommonAction(mUa, "update", 
                           boost::bind(&ClientPublication::update, boost::bind<ClientPublication*>(&ClientPublicationHandle::get, boost::ref(mHandle)), body));

}


CommonAction* 
TestClientPublication::end()
{
   return new CommonAction(mUa, "end", boost::bind(&ClientPublication::end, boost::bind<ClientPublication*>(&ClientPublicationHandle::get, boost::ref(mHandle))));
}

bool 
TestClientPublication::isMyEvent(Event* e)
{
   ClientPublicationEvent* sub = dynamic_cast<ClientPublicationEvent*>(e);
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
      StackLog(<< "not a ClientPublicationEvent");
   }
   return false;
}

TestClientPublication::ExpectBase* 
TestClientPublication::expect(ClientPublicationEvent::Type t, 
                               MessageMatcher* matcher, 
                               ExpectPreCon& pred,
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientPublicationEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction);
}

TestClientPublication::ExpectBase* 
TestClientPublication::expect(ClientPublicationEvent::Type t, 
                               ExpectPreCon& pred, 
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientPublicationEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction);
}

TestClientPublication::ExpectBase* 
TestClientPublication::expect(ClientPublicationEvent::Type t, 
                               MessageMatcher* matcher, 
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientPublicationEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction);
}

