#include <ClientSubscription.hxx>

bool
ClientSubscription::matches(const SipMessage& subOrNotify)
{
   return  (subOrNotify.exists(h_Event) && 
            subOrNotify.header(h_Event).value() == mEventType && 
            ( !subOrNotify.header(h_Event).exists(p_id) || 
              subOrNotify.header(h_Event).param(p_id) == mSubscriptionId));
}
