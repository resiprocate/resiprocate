#include "resiprocate/dum/BaseSubscription.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/SipMessage.hxx"

using namespace resip;

BaseSubscription::BaseSubscription(DialogUsageManager& dum, Dialog& dialog, const SipMessage& request) :
   DialogUsage(dum, dialog),
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
BaseSubscription::matches(const SipMessage& subOrNotify)
{
   return (subOrNotify.exists(h_Event) && 
           subOrNotify.header(h_Event).value() == mEventType && 
           ( !subOrNotify.header(h_Event).exists(p_id) || 
             subOrNotify.header(h_Event).param(p_id) == mSubscriptionId));
}

BaseSubscription::~BaseSubscription()
{
}

SubscriptionState 
BaseSubscription::getSubscriptionState()
{
   return mSubscriptionState;
}

