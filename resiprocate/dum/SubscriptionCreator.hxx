#if !defined(RESIP_SUBSCRIPTIONCREATOR_HXX)
#define RESIP_SUBSCRIPTIONCREATOR_HXX

namespace resip
{

class SubscriptionCreator : public BaseCreator 
{
  public:
   
    //probably want to have things like the Accept list here too
    SubscriptionCreator(const Data& event);

  private:
    Data mEvent; 

};
 
}

#endif
