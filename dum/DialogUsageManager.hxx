#if !defined(RESIP_DIALOGUSAGEMANAGER_HXX)
#define RESIP_DIALOGUSAGEMANAGER_HXX

#include <list>

#include "resiprocate/sam/DialogSet.hxx"

namespace resip 
{

class SipStack;
class Profile;
class RedirectManager;
class ClientAuthManager;
class ServerAuthManager;
class ClientRegistrationHandler;
class ServerRegistrationHandler;
class InviteSessionHandler;
class ClientSubscriptionHandler;
class ServerSubscriptionHandler;
class ClientPublicationHandler;
class ServerPublicationHandler;
class OutOfDialogHandler;



class DialogUsageManager
{
   public:
      class Exception
      {
      };
      
      DialogUsageManager(SipStack& stack);
      ~DialogUsageManager();
      
      void setProfile(Profile* profile);

      void setManager(RedirectManager* redirect);
      void setManager(ClientAuthManager* client);
      void setManager(ServerAuthManager* client);

      void setClientRegistrationHandler(ClientRegistrationHandler*);
      void setServerRegistrationHandler(ServerRegistrationHandler*);
      void setInviteSessionHandler(InviteSessionHandler*);
      
      void addHandler(const Data& eventType, ClientSubscriptionHandler*);
      void addHandler(const Data& eventType, ServerSubscriptionHandler*);

      void addHandler(const Data& eventType, ClientPublicationHandler*);
      void addHandler(const Data& eventType, ServerPublicationHandler*);

      void addHandler(MethodTypes&, OutOfDialogHandler*);
      
      SipMessage* makeInviteSession(const Uri& target);
      SipMessage* makeSubscription(const Uri& aor, const Data& eventType);
      SipMessage* makeRefer(const Uri& aor, const H_ReferTo::Type& referTo);
      SipMessage* makePublication(const Uri& aor, const Data& eventType);
      SipMessage* makeRegistration(const Uri& aor);
      SipMessage* makeOutOfDialogRequest(const Uri& aor, const MethodTypes& meth);

      SipMessage* makeInviteSession(DialogId, const Uri& target);
      SipMessage* makeSubscription(DialogId, const Uri& aor, const Data& eventType);
      SipMessage* makeRefer(DialogId, const Uri& aor, const H_ReferTo::Type& referTo);
      SipMessage* makePublication(DialogId, const Uri& aor, const Data& eventType);
      SipMessage* makeRegistration(DialogId, const Uri& aor);
      SipMessage* makeOutOfDialogRequest(DialogId, const Uri& aor, const MethodTypes& meth);
      
      void cancel(DialogIdSet invSessionId);
      DialogSetId send(SipMessage* newClientRequest);

      void process();
      
      DialogIdSet findAllDialogs();
      UsageSet    findAllUsages();
      
      BaseUsage& findInvSession( DialogId id );
      UsageSet   findSubscriptions( DialogId id );
      BaseUsage& findRegistration( DialogId id );
      BaseUsage& findPublication( DialogId id );
      UsageSet   findOutOfDialogs( DialogId id );
      
   private:
      bool validateRequest(const SipMessage& request);
      bool validateTo(const SipMessage& request);
      bool mergeRequest(const SipMessage& request);
      
      Dialog&    findOrCreateDialog(SipMessage* msg);
      Dialog&    findDialog(DialogId id);
      DialogSet& findDialogSet( DialogSetId id );

      
      BaseCreator& findCreator(DialogId id);
      
      HashMap<DialogSetId, DialogSet > mDialogSetMap;

      Profile* mProfile;
      RedirectManager* mRedirectManager;
      ClientAuthManager* mClientAuthManager;
      ServerAuthManager* mServerAuthManager;  
    
      InviteSessionHandler* mInviteSessionHandler;
      ClientRegistrationHandler* mClientRegistrationHandler;
      ServerRegistrationHandler* mServerRegistrationHandler;      

      std::list<ClientSubscriptionHandler*> mClientSubscriptionHandler;
      std::list<ServerSubscriptionHandler*> mServerSubscriptionHandler;      
      std::list<ClientPublicationHandler*> mClientPublicationHandler;
      std::list<ServerPublicationHandler*> mServerPublicationHandler;      
      OutOfDialogHandler* mOutOfDialogHandler;
	  
      SipStack& mStack;
};

}

#endif
