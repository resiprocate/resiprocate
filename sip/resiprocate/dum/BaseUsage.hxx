#if !defined(RESIP_BASEUSAGE_HXX)
#define RESIP_BASEUSAGE_HXX

namespace resip
{

class DialogUsageManager;

class BaseUsage
{
   public:
      class Exception : BaseException
      {
      };

      class Handle
      {
         public:
            typedef UInt64 Id;
         protected:
            Handle(DialogUsageManager& dum);
            // throws if not found
            BaseUsage* get();
         private:
            Id mId;
            DialogUsageManager& mDum;
            friend class DialogUsageManager;
            static UInt64 getNext();
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
      //Dialog& dialog();
      
      virtual void end()=0;
      virtual void dispatch(const SipMessage& msg)=0;
      
   private:
      DialogUsageManager& mDum;
      DialogImpl& mDialog;
};
 
}

#endif
