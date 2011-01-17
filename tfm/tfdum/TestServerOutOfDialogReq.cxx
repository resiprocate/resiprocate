#include "resip/dum/DialogUsageManager.hxx"
#include "tfm/TestEndPoint.hxx"
#include "tfm/CommonAction.hxx"
#include "DumUserAgent.hxx"
#include "DumExpect.hxx"
#include "DumUaAction.hxx"
#include "TestServerOutOfDialogReq.hxx"

#include "resip/dum/Handles.hxx"

#include <boost/bind.hpp>
#include <boost/function.hpp>

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
                                                        boost::bind(&ServerOutOfDialogReq::accept, 
                                                                    boost::bind<ServerOutOfDialogReq*>(&ServerOutOfDialogReqHandle::get, boost::ref(mHandle)), statusCode), 
                                                        NoAdornment::instance());
}

SendingAction<ServerOutOfDialogReqHandle>*
TestServerOutOfDialogReq::reject(int responseCode)
{
   return new SendingAction<ServerOutOfDialogReqHandle>(mUa, mHandle, "reject",
                                                        boost::bind(&ServerOutOfDialogReq::reject, 
                                                                    boost::bind<ServerOutOfDialogReq*>(&ServerOutOfDialogReqHandle::get, boost::ref(mHandle)), 
                                                                    responseCode),
                                                        NoAdornment::instance());
}

SendingAction<ServerOutOfDialogReqHandle>*
TestServerOutOfDialogReq::answerOptions()
{
   return new SendingAction<ServerOutOfDialogReqHandle>(mUa, mHandle, "answerOptions",
                                                        boost::bind(&ServerOutOfDialogReq::answerOptions, 
                                                                    boost::bind<ServerOutOfDialogReq*>(&ServerOutOfDialogReqHandle::get, boost::ref(mHandle))),
                                                        NoAdornment::instance());
}

CommonAction* 
TestServerOutOfDialogReq::end()
{
   return new CommonAction(mUa, "end", boost::bind(&ServerOutOfDialogReq::end, boost::bind<ServerOutOfDialogReq*>(&ServerOutOfDialogReqHandle::get, boost::ref(mHandle))));
}
 
CommonAction* 
TestServerOutOfDialogReq::send(resip::SharedPtr<SipMessage> msg)
{
   return new CommonAction(mUa, "send", 
                           boost::bind(&ServerOutOfDialogReq::send, boost::bind<ServerOutOfDialogReq*>(&ServerOutOfDialogReqHandle::get, boost::ref(mHandle)), 
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

