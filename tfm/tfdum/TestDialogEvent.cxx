#include "resip/dum/DialogUsageManager.hxx"
#include "tfm/TestEndPoint.hxx"
#include "tfm/CommonAction.hxx"
#include "TestDialogEvent.hxx"
#include "DumUserAgent.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "DumExpect.hxx"
#include "tfm/SipEvent.hxx"
#include "DialogEventExpect.hxx"

#include "resip/dum/Handles.hxx"

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;

DialogEventPred::DialogEventPred(const resip::DialogEventInfo& dialogEventInfo)
   : mDialogEventInfo(dialogEventInfo),
     mMismatchExplanation("")
{}

const resip::Data& 
DialogEventPred::explainMismatch() const
{
   return mMismatchExplanation;
}

bool
DialogEventPred::passes(boost::shared_ptr<Event> event)
{
   DialogEventHandlerEvent* dehEvent = dynamic_cast<DialogEventHandlerEvent*>(event.get());
   resip_assert(dehEvent);

   bool passesIdCheck = (dehEvent->getDialogEventInfo().getDialogEventId() == mDialogEventInfo.getDialogEventId()) || 
                        (mDialogEventInfo.getDialogEventId() == "*");

   if (!passesIdCheck)
   {
      mMismatchExplanation = "DialogEventIds do not match!";
      return false;
   }

   if (!(mDialogEventInfo.getCallId() == "*"))
   {
      if (!(dehEvent->getDialogEventInfo().getCallId() == mDialogEventInfo.getCallId()))
      {
         mMismatchExplanation = "CallIds do not match!";
         return false;
      }
   }

   if (!(mDialogEventInfo.getLocalTag() == "*"))
   {
      if (!(dehEvent->getDialogEventInfo().getLocalTag() == mDialogEventInfo.getLocalTag() || 
            mDialogEventInfo.getLocalTag() == "*"))
      {
         mMismatchExplanation = "Local Tags do not match!";
         return false;
      }
   }

   if (!(mDialogEventInfo.getRemoteTag() == "*"))
   {
      if (!(dehEvent->getDialogEventInfo().getRemoteTag() == mDialogEventInfo.getRemoteTag()))
      {
         mMismatchExplanation = "Remote tags do not match!";
         return false;
      }
   }

   if (!(dehEvent->getDialogEventInfo().getLocalIdentity() == mDialogEventInfo.getLocalIdentity()))
   {
      mMismatchExplanation = "LocalIdentities do not match!";
      return false;
   }

   //if (!(dehEvent->getDialogEventInfo().getLocalSdp().getBodyData() == mDialogEventInfo.getLocalSdp().getBodyData()))
   //{
   //   mMismatchExplanation = "LocalSdp does not match!";
   //   return false;
   //}

   //if (dehEvent->getDialogEventInfo().hasLocalTarget() && mDialogEventInfo.hasLocalTarget())
   {
      if (!(dehEvent->getDialogEventInfo().getLocalTarget() == mDialogEventInfo.getLocalTarget() ||
            mDialogEventInfo.getLocalTarget() == resip::Uri("sip:anyone@anywhere.com")))
      {
         mMismatchExplanation = "Local targets do not match!";
         return false;
      }
   }

   if (!(dehEvent->getDialogEventInfo().getRemoteIdentity() == mDialogEventInfo.getRemoteIdentity()))
   {
      resip::DataStream mmds(mMismatchExplanation);
      mmds << "Remote identities do not match! expected " << mDialogEventInfo.getRemoteIdentity() << " got " << 
         dehEvent->getDialogEventInfo().getRemoteIdentity();
      return false;
   }

   if (dehEvent->getDialogEventInfo().hasRemoteTarget() != mDialogEventInfo.hasRemoteTarget())
   {
      mMismatchExplanation = "Remote targets do not match!";
      return false;
   }

   if (dehEvent->getDialogEventInfo().hasRemoteTarget())
   {
      if (!(dehEvent->getDialogEventInfo().getRemoteTarget() == mDialogEventInfo.getRemoteTarget()))
      {
         resip::DataStream mmds(mMismatchExplanation);
         mmds << "Remote targets do not match! expected " << mDialogEventInfo.getRemoteTarget() <<
              " got " << dehEvent->getDialogEventInfo().getRemoteTarget();
         return false;
      }
   }

   if (dehEvent->getDialogEventInfo().hasRefferedBy() != mDialogEventInfo.hasRefferedBy())
   {
      mMismatchExplanation = "hasRefferedBy() does not match!";
      return false;
   }

   if (dehEvent->getDialogEventInfo().hasRefferedBy())
   {
      if (!(dehEvent->getDialogEventInfo().getRefferredBy() == mDialogEventInfo.getRefferredBy()))
      {
         resip::DataStream mmds(mMismatchExplanation);
         mmds << "Referred By headers do not match! got "
              << dehEvent->getDialogEventInfo().getRefferredBy() 
              << " expected " 
              << mDialogEventInfo.getRefferredBy();
         return false;
      }
   }

   if (dehEvent->getDialogEventInfo().hasReplacesId() != mDialogEventInfo.hasReplacesId())
   {
      mMismatchExplanation = "Missing Replaces Id in the expected or in the actual DialogEventInfo!";
      return false;
   }
   if (mDialogEventInfo.hasReplacesId())
   {
      if (mDialogEventInfo.getReplacesId().getCallId() == "*")
      {
         if (!dehEvent->getDialogEventInfo().hasReplacesId())
         {
            mMismatchExplanation = "Missing Replaces Id in the actual DialogEventInfo!";
            return false;
         }
      }
      else if (!(dehEvent->getDialogEventInfo().getReplacesId() == mDialogEventInfo.getReplacesId()))
      {
         mMismatchExplanation = "Replaces Id headers do not match!";
         return false;
      }
   }

   if (!(dehEvent->getDialogEventInfo().getState() == mDialogEventInfo.getState()))
   {
      mMismatchExplanation = "States do not match!";
      return false;
   }
   return true;
}

TestDialogEvent::TestDialogEvent(DumUserAgent* dumUa) : TestUsage(dumUa)
{
}

bool 
TestDialogEvent::isMyEvent(Event* e)
{
   DialogEventHandlerEvent* dev = dynamic_cast<DialogEventHandlerEvent*>(e);
   return (dev != 0);
}

TestDialogEvent::ExpectBase* 
TestDialogEvent::expect(DialogEventHandlerEvent::Type t,
                        DialogEventPred& pred,
                               int timeoutMs, 
                               ActionBase* expectAction)
{
   return new DialogEventExpect(*this,
                        new DialogEventMatcher(t),
                        pred,
                        //new DumEventMatcherSpecific<DialogEventHandlerEvent>(t),
                        timeoutMs,
                        expectAction);
}

TestDialogEvent::ExpectBase* 
TestDialogEvent::expect(DialogEventHandlerEvent::Type t,
                     DialogEventPred& pred,
                     resip::InviteSessionHandler::TerminatedReason reason,
                     int timeoutMs,
                     ActionBase* expectAction)
{
   return new DialogEventExpect(*this,
      new DialogEventMatcher(t),
      pred,
      reason,
      timeoutMs,
      expectAction);
}




