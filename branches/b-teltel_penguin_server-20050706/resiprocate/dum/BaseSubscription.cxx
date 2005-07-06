#include "resiprocate/dum/BaseSubscription.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/SipMessage.hxx"

using namespace resip;

BaseSubscription::BaseSubscription(DialogUsageManager& dum, Dialog& dialog, const SipMessage& request) :
   DialogUsage(dum, dialog),
   mState(Initial),
   mDocumentKey(request.header(h_RequestLine).uri().getAor()),
   mSubscriptionId(Data::Empty),
   mTimerSeq(0),
   mSubscriptionState(Invalid)
   
{
   if (request.header(h_RequestLine).method() == REFER)
   {
      mEventType = "refer";
   }
   else
   {
      mEventType = request.header(h_Event).value();
   }
   
   if (request.exists(h_Event) && request.header(h_Event).exists(p_id))
   {
      mSubscriptionId = request.header(h_Event).param(p_id);
   }

}

bool
BaseSubscription::matches(const SipMessage& msg)
{
   if (msg.isResponse() && msg.header(h_CSeq) == mLastRequest.header(h_CSeq))
   {
      return true;
   }
   else
   {
      return (msg.exists(h_Event) && 
              msg.header(h_Event).value() == mEventType && 
              ( !msg.header(h_Event).exists(p_id) || 
                msg.header(h_Event).param(p_id) == mSubscriptionId));
   }
}

BaseSubscription::~BaseSubscription()
{
}

SubscriptionState 
BaseSubscription::getSubscriptionState()
{
   return mSubscriptionState;
}

