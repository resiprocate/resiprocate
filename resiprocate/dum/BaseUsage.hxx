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

      SipMessage* makeInviteSession();
      SipMessage* makeSubscription();
      SipMessage* makeRefer();
      SipMessage* makePublication();
      SipMessage* makeRegistration();
      SipMessage* makeOutOfDialogRequest();

      // to send a request on an existing dialog (made from make... methods above)
      void send(const SipMessage& request);
      
      DialogUsageManager& dum();
      Dialog& dialog();
      
    virtual void end();

   private:
      DialogUsageManager& mDUM;
      DialogImpl& mDialog;
};
 
}

#endif
