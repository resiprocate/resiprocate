#if !defined(RESIP_REGISTRATIONHANDLER_HXX)
#define RESIP_REGISTRATIONHANDLER_HXX

namespace resip
{

class RegistrationHandler
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
