#if !defined(RESIP_CLIENTDIALOG_HXX)
#define RESIP_CLIENTDIALOG_HXX

#include "resiprocate/sam/DialogId.hxx"

namespace resip
{


    
    class BaseUsage;
    class UsageSet;
    class SipMessage;

    typedef std::list<DialogId> DialogIdSet;

class Dialog 
{
   public:

      DialogId getId() const;
      
      SipMessage* makeInviteSession();
      SipMessage* makeSubscription();
      SipMessage* makeRefer();
      SipMessage* makePublication();
      SipMessage* makeRegistration();
      SipMessage* makeOutOfDialogRequest();

      BaseUsage& findInvSession();
      UsageSet   findSubscriptions();
      BaseUsage& findRegistration();
      BaseUsage& findPublication();
      UsageSet   findOutOfDialogs();
      
      bool shouldMerge(const SipMessage& request);
      void processNotify(const SipMessage& notify);
      
   private:
      std::list<BaseUsage*> mUsages;
      DialogId mId;  
};
 
}


#endif
