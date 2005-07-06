#if !defined(RESIP_OUTOFDIALOGREQCREATOR_HXX)
#define RESIP_OUTOFDIALOGREQCREATOR_HXX

#include "resiprocate/dum/BaseCreator.hxx"

namespace resip
{

class Uri;
class SipMessage;

class OutOfDialogReqCreator: public BaseCreator
{
   public:
      OutOfDialogReqCreator(DialogUsageManager& dum, MethodTypes method, const NameAddr& target, const NameAddr& from);
      virtual void dispatch(SipMessage& msg);
};
 
}

#endif
