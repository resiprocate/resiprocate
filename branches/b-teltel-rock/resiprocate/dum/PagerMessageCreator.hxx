#if !defined(RESIP_PAGERMESSAGECREATOR_HXX)
#define RESIP_PAGERMESSAGECREATOR_HXX

#include "resiprocate/dum/BaseCreator.hxx"

namespace resip
{

class Uri;
class SipMessage;

class PagerMessageCreator: public BaseCreator
{
   public:
      PagerMessageCreator(DialogUsageManager& dum, const NameAddr& target, UserProfile& userProfile);
};
 
}

#endif
