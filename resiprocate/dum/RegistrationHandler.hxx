#if !defined(RESIP_REGISTRATIONHANDLER_HXX)
#define RESIP_REGISTRATIONHANDLER_HXX

namespace resip
{

class ClientRegistrationHandler
{
   public:
      /// Called when registraion succeeds or each time it is sucessfully
      /// refreshed. 
      virtual void onSuccess(ClientRegistration::Handle, const SipMessage& response)=0;

      /// Called if registration fails. 
      virtual void onFailure(ClientRegistration::Handle, const SipMessage& response)=0;
};

class ServerRegistrationHandler
{
   public:
      /// Called when registration is refreshed
      virtual void onRefresh(ServerRegistration::Handle, const SipMessage& reg)=0;
      
      /// called when one of the contacts is removed
      virtual void onRemoveOne(ServerRegistration::Handle, const SipMessage& reg)=0;
      
      /// Called when all the contacts are removed 
      virtual void onRemoveAll(ServerRegistration::Handle, const SipMessage& reg)=0;
      
      /// Called when a new contact is added. This is after authentication has
      /// all sucseeded
      virtual void onAdd(ServerRegistration::Handle, const SipMessage& reg)=0;

      /// Called when an registration expires 
      virtual void onExpired(ServerRegistration::Handle, const NameAddr& contact)=0;
};

}

#endif
