#if !defined(RESIP_BASEUSAGE_HXX)
#define RESIP_BASEUSAGE_HXX

#include "resiprocate/sam/DialogUsageManager.hxx"

namespace resip
{
class BaseUsage
{
   public:
      class Exception : BaseException
      {
      };

      BaseUsage(DialogUsageManager& dum);

      SipMessage* makeInviteSession(const Uri& target);
      SipMessage* makeSubscription(const Uri& aor, const Data& eventType);
      SipMessage* makeRefer(const Uri& aor, const H_ReferTo::Type& referTo);
      SipMessage* makePublication(const Uri& aor, const Data& eventType);
      SipMessage* makeRegistration(const Uri& aor);
      SipMessage* makeOutOfDialogRequest(const Uri& aor, const MethodTypes& meth);
      
      DialogUsageManager& dum();
      Dialog& dialog();
      
    virtual void end();

   private:
      DialogUsageManager& mDUM;
      DialogImpl& mDialog;
};
 
}

#endif
