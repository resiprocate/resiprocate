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
      
      /// called when one or more specified contacts is removed
      virtual void onRemove(ServerRegistrationHandle, const SipMessage& reg)=0;
      
      /// Called when all the contacts are removed using "Contact: *"
      virtual void onRemoveAll(ServerRegistrationHandle, const SipMessage& reg)=0;
      
      /** Called when one or more contacts are added. This is after 
          authentication has all succeeded */
      virtual void onAdd(ServerRegistrationHandle, const SipMessage& reg)=0;

      /// Called when a client queries for the list of current registrations
      virtual void onQuery(ServerRegistrationHandle, const SipMessage& reg)=0;
};

}

#endif
