#if !defined(RESIP_SUBSCRIPTIONCREATOR_HXX)
#define RESIP_SUBSCRIPTIONCREATOR_HXX

#include "resiprocate/dum/BaseCreator.hxx"

namespace resip
{

class SubscriptionCreator : public BaseCreator 
{
  public:
   
      //probably want to have things like the Accept list here too
      SubscriptionCreator(DialogUsageManager& dum, const NameAddr& target, const NameAddr& from, const Data& event);
  private:
    Data mEvent; 
};
 
}

#endif
