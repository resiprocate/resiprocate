#if !defined(RESIP_SUBSCRIPTIONCREATOR_HXX)
#define RESIP_SUBSCRIPTIONCREATOR_HXX

#include "BaseCreator.hxx"

namespace resip
{

class SubscriptionCreator : public BaseCreator 
{
  public:
   
      //probably want to have things like the Accept list here too
      SubscriptionCreator(DialogUsageManager& dum, const Data& event);
      
      void makeNewSubscription(const SipMessage& notify);

      virtual void dispatch(SipMessage& msg);
      
  private:
    Data mEvent; 

};
 
}

#endif
