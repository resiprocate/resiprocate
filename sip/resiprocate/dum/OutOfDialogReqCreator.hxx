#if !defined(RESIP_OUTOFDIALOGREQCREATOR_HXX)
#define RESIP_OUTOFDIALOGREQCREATOR_HXX

#include "resiprocate/dum/BaseCreator.hxx"

namespace resip
{

class Uri;
class SipMessage;

/** @file OutOfDialogReqCreator.hxx
 *   @todo This file is empty
 */

class OutOfDialogReqCreator: public BaseCreator
{
   public:
      OutOfDialogReqCreator(DialogUsageManager& dum, MethodTypes method, const Uri&);
      virtual void dispatch(SipMessage& msg);
};
 
}

#endif
