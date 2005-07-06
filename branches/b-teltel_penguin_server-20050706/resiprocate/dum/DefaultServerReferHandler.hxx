#if !defined(RESIP_DEFAULTSERVERREFERHANDLER_HXX)
#define RESIP__DEFAULTSERVERREFERHANDLER_HXX

#include "resiprocate/dum/SubscriptionHandler.hxx"

namespace resip
{

class DefaultServerReferHandler : public ServerSubscriptionHandler
{
  public:   
      virtual void onNewSubscription(ServerSubscriptionHandle, const SipMessage& sub);
      virtual void onRefresh(ServerSubscriptionHandle, const SipMessage& sub);
      virtual void onTerminated(ServerSubscriptionHandle);

      static DefaultServerReferHandler* Instance();

      virtual bool hasDefaultExpires() const;
      virtual int getDefaultExpires() const;
   protected:
      DefaultServerReferHandler() {}
      virtual ~DefaultServerReferHandler() {}
};

}

#endif
