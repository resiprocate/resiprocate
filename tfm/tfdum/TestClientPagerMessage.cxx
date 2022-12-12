#include "resip/dum/DialogUsageManager.hxx"
#include "tfm/TestEndPoint.hxx"
#include "tfm/CommonAction.hxx"
#include "TestClientPagerMessage.hxx"
#include "DumUserAgent.hxx"
#include "resip/dum/ClientPagerMessage.hxx"
#include "DumExpect.hxx"

#include "resip/dum/Handles.hxx"

#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;

TestClientPagerMessage::TestClientPagerMessage(DumUserAgent* ua, ClientPagerMessageHandle h)
   : TestUsage(ua),
     mHandle(h)
{
   resip_assert(mHandle.isValid());
}

CommonAction* 
TestClientPagerMessage::page(std::unique_ptr<resip::Contents>& contents, resip::DialogUsageManager::EncryptionLevel level)
{
   return new CommonAction(mUa, "page", [=, &contents] { mHandle->page(std::move(contents), level); });
}

CommonAction* 
TestClientPagerMessage::end()
{
   return new CommonAction(mUa, "end", [this] { mHandle->end(); });
}

bool 
TestClientPagerMessage::isMyEvent(Event* e)
{
   ClientPagerMessageEvent* sub = dynamic_cast<ClientPagerMessageEvent*>(e);
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
      StackLog(<< "not a ClientPagerMessageEvent");
   }
   return false;
}

TestClientPagerMessage::ExpectBase* 
TestClientPagerMessage::expect(ClientPagerMessageEvent::Type t, 
                               MessageMatcher* matcher, 
                               ExpectPreCon& pred,
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientPagerMessageEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction);
}

TestClientPagerMessage::ExpectBase* 
TestClientPagerMessage::expect(ClientPagerMessageEvent::Type t, 
                               ExpectPreCon& pred, 
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientPagerMessageEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction);
}

TestClientPagerMessage::ExpectBase* 
TestClientPagerMessage::expect(ClientPagerMessageEvent::Type t, 
                               MessageMatcher* matcher, 
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ClientPagerMessageEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction);
}

