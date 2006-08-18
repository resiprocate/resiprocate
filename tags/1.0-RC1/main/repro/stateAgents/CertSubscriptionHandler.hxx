#if !defined(Repro_CertSubscriptionHandler_hxx)
#define Repro_CertSubscriptionHandler_hxx

#if defined(USE_SSL)

#include "resip/dum/ServerPublication.hxx"
#include "resip/dum/ServerSubscription.hxx"
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

class CertSubscriptionHandler : public resip::ServerSubscriptionHandler
{
   public:
      CertSubscriptionHandler(resip::Security& security);
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
