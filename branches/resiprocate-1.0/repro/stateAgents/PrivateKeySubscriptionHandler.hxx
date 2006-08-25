#if !defined(Repro_PrivateKeySubscriptionHandler_hxx)
#define Repro_PrivateKeySubscriptionHandler_hxx

#if defined(USE_SSL)
#include "resip/dum/SubscriptionHandler.hxx"

namespace resip
{
class Security;
class SipMessage;
class SecurityAttributes;
class Data;
class Contents;
}

namespace repro
{

class PrivateKeySubscriptionHandler : public resip::ServerSubscriptionHandler
{
   public:
      PrivateKeySubscriptionHandler(resip::Security& security);
      virtual void onNewSubscription(resip::ServerSubscriptionHandle h, const resip::SipMessage& sub);
      virtual void onPublished(resip::ServerSubscriptionHandle associated, 
                               resip::ServerPublicationHandle publication, 
                               const resip::Contents* contents,
                               const resip::SecurityAttributes* attrs);
      virtual void onTerminated(resip::ServerSubscriptionHandle);
      virtual void onError(resip::ServerSubscriptionHandle, const resip::SipMessage& msg);

   private:
      resip::Security& mSecurity;
};
 
}

#endif
#endif
