class Profile;
class RedirectManager;

class SAManager
{
   public:
      SAManager(SipStack& stack);
      ~SAManager();
      
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

      InvSessionCreator makeInvSession(BaseSession&, const Uri& aor);
      SubscriptionCreator makeSubscription(BaseSession&, const Uri& aor, const Data& eventType);
      SubscriptionCreator makeRefer(BaseSession&, const Uri& aor);
      PublicationCreator makePublication(BaseSession&, const Uri& aor, const Data& eventType);
      RegistrationCreator makeRegistration(BaseSession&, const Uri& aor);
      
   private:
      DialogImpl& findOrCreateDialog(SipMessage* msg);
      DialogImpl& findOrCreateDialog(DialogId id);

      Profile* mProfile;
      RedirectManager* mRedirectManager;
      ClientAuthManager* mClientAuthManager;
      ServerAuthManager* mServerAuthManager;      
      ClientInviteSessionHandler* mClientInviteSessionHandler;
      ServerInviteSessionHandler* ClientInviteSessionHandler;      
      ClientRegistrationHandler* mClientRegistrationHandler;
      ServerRegistrationHandler* ClientRegistrationHandler;      
      ClientSubscriptionHandler* mClientSubscriptionHandler;
      ServerSubscriptionHandler* ClientSubscriptionHandler;      
      ClientPublicationHandler* mClientPublicationHandler;
      ServerPublicationHandler* mServerPublicationHandler;      
      OutOfDialogHandler* mOutOfDialogHandler;
      
      HashMap<CreatorId, BaseCreatorImpl*> mBaseCreatorMap; 
      HashMap<CreatorId, std::list<BaseSessionImpl*> > mBaseCreatorMap;
      HashMap<DialogId, std::list<BaseSessionImpl*> > mBaseCreatorMap;

      SipStack& mStack;
};


   
