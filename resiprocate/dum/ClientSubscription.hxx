#if !defined(RESIP_CLIENTSUBSCRIPTION_HXX)
#define RESIP_CLIENTSUBSCRIPTION_HXX

namespace resip
{

class ClientSubscription: public BaseUsage
{
  public:
    class Handle
    {
    };

  void refreshNow();

  private:

   SubscriptionState mSubState;
   Contents   mCurrentEventDocument;
   UInt64     mExpirationTime;

};
 
}

#endif
