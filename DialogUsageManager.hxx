#if !defined(RESIP_DIALOAGUSAGEMANAGER_HXX)
#define RESIP_DIALOAGUSAGEMANAGER_HXX

namespace resip 
{

class SipStack;
class Profile;
class RedirectManager;


class DialogUsageManager
{
   public:
      DialogUsageManager(SipStack& stack);
      ~DialogUsageManager();
      
      void setProfile(Profile* profile);

      void setManager(RedirectManager* redirect);
      void setManager(ClientAuthManager* client);
      void setManager(ServerAuthManager* client);

      void setHandler(ClientRegistrationHandler*);
      void setHandler(ServerRegistrationHandler*);
      
      void setHandler(ClientInvSessionHandler*);
      void setHandler(ServerInvSessionHandler*);
      
      void addHandler(const Data& eventType, ClientSubscriptionHandler*);
      void addHandler(const Data& eventType, ServerSubscriptionHandler*);

      void addHandler(const Data& eventType, ClientPublicationHandler*);
      void addHandler(const Data& eventType, ServerPublicationHandler*);

      void addHandler(MethodTypes&, OutOfDialogHandler*);
      
      InvSessionCreator makeInvSession(const Uri& aor, SdpContents* initial);
      SubscriptionCreator makeSubscription(const Uri& aor, const Data& eventType);
      SubscriptionCreator makeRefer(const Uri& aor, const H_ReferTo::Type& referTo);
      PublicationCreator makePublication(const Uri& aor, const Data& eventType);
      RegistrationCreator makeRegistration(const Uri& aor);
      OutOfDialogRequestCreator makeOutOfDialogRequest(const Uri& aor, const MethodTypes& meth);

      InvSessionCreator makeInvSession(BaseUsage&, const Uri& aor);
      SubscriptionCreator makeSubscription(BaseUsage&, const Uri& aor, const Data& eventType);
      SubscriptionCreator makeRefer(BaseUsage&, const Uri& aor);
      PublicationCreator makePublication(BaseUsage&, const Uri& aor, const Data& eventType);
      RegistrationCreator makeRegistration(BaseUsage&, const Uri& aor);
      
      DialogIdSet findAllDialogs();
      UsageSet    findAllUsages();
      
      UsageSet findInvSessions( DialogId id );
      UsageSet findSubscriptions( DialogId id );
      UsageSet findRegistrations( DialogId id );
      UsageSet findPublications( DialogId id );
      UsageSet findOutOfDialogs( DialogId id );
      
   private:
      Dialog& findOrCreateDialog(SipMessage* msg);
      Dialog& findDialog(DialogId id);

      HashMap<DialogSetId, DialogSet > mDialogSetMap;

      Profile* mProfile;
      RedirectManager* mRedirectManager;
      ClientAuthManager* mClientAuthManager;
      ServerAuthManager* mServerAuthManager;  
    
      ClientInviteSessionHandler* mClientInviteSessionHandler;
      ServerInviteSessionHandler* mServerInviteSessionHandler;      
      ClientRegistrationHandler* mClientRegistrationHandler;
      ServerRegistrationHandler* mServerRegistrationHandler;      
      ClientSubscriptionHandler* mClientSubscriptionHandler;
      ServerSubscriptionHandler* mServerSubscriptionHandler;      
      ClientPublicationHandler* mClientPublicationHandler;
      ServerPublicationHandler* mServerPublicationHandler;  
      OutOfDialogHandler* mClientOutOfDialogHandler;
      OutOfDialogHandler* mServerOutOfDialogHandler;
      
      SipStack& mStack;
};

}

#endif
