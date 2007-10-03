#ifndef SUBSCRIPTIONSTATE
#define SUBSCRIPTIONSTATE

using namespace resip;

class SubscriptionState{
  public:
    Data & event() {return mEvent;}
    Data & eventId() {return mEventId;}
    Data & resource() {return mResource; }
    time_t & expires() {return mExpires;}
  private:
    Data mEvent;
    Data mEventId;
    Data mResource;
    time_t  mExpires;
};

#endif
