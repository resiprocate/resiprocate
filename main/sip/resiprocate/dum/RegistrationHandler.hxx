#if !defined(RESIP_REGISTRATIONHANDLER_HXX)
#define RESIP_REGISTRATIONHANDLER_HXX

#include "resiprocate/dum/Handles.hxx"

namespace resip
{

class SipMessage;
class NameAddr;

class ClientRegistrationHandler
{
   public:
      /// Called when registraion succeeds or each time it is sucessfully
      /// refreshed. 
      virtual void onSuccess(ClientRegistrationHandle, const SipMessage& response)=0;

      // Called when all of my bindings have been removed
      virtual void onRemoved(ClientRegistrationHandle) = 0;
      
      /// Called if registration fails, usage will be destroyed
      virtual void onFailure(ClientRegistrationHandle, const SipMessage& response)=0;
};

class ServerRegistrationHandler
{
   public:
      /// Called when registration is refreshed
      virtual void onRefresh(ServerRegistrationHandle, const SipMessage& reg)=0;
      
      /// called when one of the contacts is removed
      virtual void onRemoveOne(ServerRegistrationHandle, const SipMessage& reg)=0;
      
      /// Called when all the contacts are removed 
      virtual void onRemoveAll(ServerRegistrationHandle, const SipMessage& reg)=0;
      
      /// Called when a new contact is added. This is after authentication has
      /// all sucseeded
      virtual void onAdd(ServerRegistrationHandle, const SipMessage& reg)=0;

      /// Called when an registration expires 
      virtual void onExpired(ServerRegistrationHandle, const NameAddr& contact)=0;
};

}

#endif
