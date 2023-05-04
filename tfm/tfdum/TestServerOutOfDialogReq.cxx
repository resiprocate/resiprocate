#include "resip/dum/DialogUsageManager.hxx"
#include "tfm/TestEndPoint.hxx"
#include "tfm/CommonAction.hxx"
#include "DumUserAgent.hxx"
#include "DumExpect.hxx"
#include "DumUaAction.hxx"
#include "TestServerOutOfDialogReq.hxx"

#include "resip/dum/Handles.hxx"

#include <functional>

#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;

TestServerOutOfDialogReq::TestServerOutOfDialogReq(DumUserAgent* ua)
   : TestUsage(ua)
{
   mUa->add(this);
}

SendingAction<ServerOutOfDialogReqHandle>*
TestServerOutOfDialogReq::accept(int statusCode)
{
   return new SendingAction<ServerOutOfDialogReqHandle>(mUa, mHandle, "accept", 
                                                        std::bind(&ServerOutOfDialogReq::accept, 
                                                                    std::bind<ServerOutOfDialogReq*>(static_cast<ServerOutOfDialogReq*(ServerOutOfDialogReqHandle::*)()>(&ServerOutOfDialogReqHandle::get), std::ref(mHandle)), statusCode), 
                                                        NoAdornment::instance());
}

SendingAction<ServerOutOfDialogReqHandle>*
TestServerOutOfDialogReq::reject(int responseCode)
{
   return new SendingAction<ServerOutOfDialogReqHandle>(mUa, mHandle, "reject",
                                                        std::bind(&ServerOutOfDialogReq::reject, 
                                                                    std::bind<ServerOutOfDialogReq*>(static_cast<ServerOutOfDialogReq*(ServerOutOfDialogReqHandle::*)()>(&ServerOutOfDialogReqHandle::get), std::ref(mHandle)), 
                                                                    responseCode),
                                                        NoAdornment::instance());
}

SendingAction<ServerOutOfDialogReqHandle>*
TestServerOutOfDialogReq::answerOptions()
{
   return new SendingAction<ServerOutOfDialogReqHandle>(mUa, mHandle, "answerOptions",
                                                        std::bind(&ServerOutOfDialogReq::answerOptions, 
                                                                    std::bind<ServerOutOfDialogReq*>(static_cast<ServerOutOfDialogReq*(ServerOutOfDialogReqHandle::*)()>(&ServerOutOfDialogReqHandle::get), std::ref(mHandle))),
                                                        NoAdornment::instance());
}

CommonAction* 
TestServerOutOfDialogReq::end()
{
   return new CommonAction(mUa, "end", std::bind(&ServerOutOfDialogReq::end, std::bind<ServerOutOfDialogReq*>(static_cast<ServerOutOfDialogReq*(ServerOutOfDialogReqHandle::*)()>(&ServerOutOfDialogReqHandle::get), std::ref(mHandle))));
}
 
CommonAction* 
TestServerOutOfDialogReq::send(std::shared_ptr<SipMessage> msg)
{
   return new CommonAction(mUa, "send", 
                           std::bind(&ServerOutOfDialogReq::send, std::bind<ServerOutOfDialogReq*>(static_cast<ServerOutOfDialogReq*(ServerOutOfDialogReqHandle::*)()>(&ServerOutOfDialogReqHandle::get), std::ref(mHandle)), 
                                       msg));
}

bool 
TestServerOutOfDialogReq::isMyEvent(Event* e)
{
   ServerOutOfDialogReqEvent* sub = dynamic_cast<ServerOutOfDialogReqEvent*>(e);
   if (sub)
   {
      DebugLog(<< "My handle id: " << mHandle.getId());
      DebugLog(<< "Compared handle id: " << sub->getHandle().getId());
      return sub->getHandle() == mHandle;
   }
   else
   {
      DebugLog(<< "not a ServerOutOfDialogReqEvent");
   }
   return false;
}

TestServerOutOfDialogReq::ExpectBase* 
TestServerOutOfDialogReq::expect(ServerOutOfDialogReqEvent::Type t, 
                                 MessageMatcher* matcher, 
                                 ExpectPreCon& pred,
                                 int timeoutMs, 
                                 ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerOutOfDialogReqEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction);
}

TestServerOutOfDialogReq::ExpectBase* 
TestServerOutOfDialogReq::expect(ServerOutOfDialogReqEvent::Type t, 
                                 ExpectPreCon& pred, 
                                 int timeoutMs, 
                                 ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerOutOfDialogReqEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction);
}

TestServerOutOfDialogReq::ExpectBase* 
TestServerOutOfDialogReq::expect(ServerOutOfDialogReqEvent::Type t, 
                                 MessageMatcher* matcher, 
                                 int timeoutMs, 
                                 ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<ServerOutOfDialogReqEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction);
}

