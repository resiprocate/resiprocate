#if !defined(RESIP_REGISTRATIONHANDLER_HXX)
#define RESIP_REGISTRATIONHANDLER_HXX

namespace resip
{

class RegistrationHandler
{
   public:
      virtual void onRefresh(ServerRegistration::Handle, const SipMessage& reg);
      virtual void onRemoveOne(ServerRegistration::Handle, const SipMessage& reg);
      virtual void onRemoveAll(ServerRegistration::Handle, const SipMessage& reg);
      virtual void onAdd(ServerRegistration::Handle, const SipMessage& reg);
};

}

#endif
