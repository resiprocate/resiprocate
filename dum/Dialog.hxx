#if !defined(RESIP_CLIENTDIALOG_HXX)
#define RESIP_CLIENTDIALOG_HXX

namespace resip
{


class Dialog 
{
   public:

      DialogId getId() const;
      
      SipMessage* makeInviteSession(const Uri& target);
      SipMessage* makeSubscription(const Uri& aor, const Data& eventType);
      SipMessage* makeRefer(const Uri& aor, const H_ReferTo::Type& referTo);
      SipMessage* makePublication(const Uri& aor, const Data& eventType);
      SipMessage* makeRegistration(const Uri& aor);
      SipMessage* makeOutOfDialogRequest(const Uri& aor, const MethodTypes& meth);

      BaseUsage& findInvSession();
      UsageSet   findSubscriptions();
      BaseUsage& findRegistration();
      BaseUsage& findPublication();
      UsageSet   findOutOfDialogs();
      
      bool shouldMerge(const SipMessage& request);
      void processNotify(const SipMessage& notify);
      
   private:
      std::list<BaseUsage*> mUsages;
      DialogId mid;  
};
 
}


#endif
