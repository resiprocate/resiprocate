#if !defined(RESIP_REGISTRATIONHANDLER_HXX)
#define RESIP_REGISTRATIONHANDLER_HXX

namespace resip
{

class ClientRegistrationHandler
{
   public:
      virtual void onSuccess(ClientRegistration::Handle, const SipMessage& response)=0;
      virtual void onFailure(ClientRegistration::Handle, const SipMessage& response)=0;
};

class ServerRegistrationHandler
{
   public:
      virtual void onRefresh(ServerRegistration::Handle, const SipMessage& reg)=0;
      virtual void onRemoveOne(ServerRegistration::Handle, const SipMessage& reg)=0;
      virtual void onRemoveAll(ServerRegistration::Handle, const SipMessage& reg)=0;
      virtual void onAdd(ServerRegistration::Handle, const SipMessage& reg)=0;
      virtual void onExpired(ServerRegistration::Handle, const NameAddr& contact)=0;
};

}

#endif
