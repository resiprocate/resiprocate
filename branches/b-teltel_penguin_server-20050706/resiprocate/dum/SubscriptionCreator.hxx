#if !defined(RESIP_SUBSCRIPTIONCREATOR_HXX)
#define RESIP_SUBSCRIPTIONCREATOR_HXX

#include "resiprocate/dum/BaseCreator.hxx"

namespace resip
{

class SubscriptionCreator : public BaseCreator 
{
   public:
   
      //probably want to have things like the Accept list here too
      SubscriptionCreator(DialogUsageManager& dum, const NameAddr& target, const NameAddr& from, const Data& event, int subscriptionTime);
      SubscriptionCreator(DialogUsageManager& dum, const NameAddr& target, const NameAddr& from, const Data& event, 
                          int subscriptionTime, int refreshInterval);

      bool hasRefreshInterval() const;      
      int getRefreshInterval() const;
   private:
      int mRefreshInterval;      
      Data mEvent; 
};
 
}

#endif
