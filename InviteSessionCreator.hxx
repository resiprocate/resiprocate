#if !defined(RESIP_INVITESESSIONCREATOR_HXX)
#define RESIP_INVITESESSIONCREATOR_HXX

#include "resiprocate/dum/BaseCreator.hxx"
#include "resiprocate/dum/Handles.hxx"

namespace resip
{

class DialogUsageManager;
class Uri;
class SdpContents;

class InviteSessionCreator : public BaseCreator
{
   public:
      InviteSessionCreator(DialogUsageManager& dum, 
                           const NameAddr& target,
                           UserProfile& userProfile,
                           const SdpContents* initial, 
                           ServerSubscriptionHandle serverSub = ServerSubscriptionHandle::NotValid());      

	  virtual ~InviteSessionCreator();
      void end();

      virtual void dispatch(const SipMessage& msg);
      const SdpContents* getInitialOffer() const;
      
      ServerSubscriptionHandle getServerSubscription() { return mServerSub; }
      
   private:
      typedef enum
      {
         Initialized,
         Trying, 
         Proceeding
      } State;
      
      State mState;
      SdpContents* mInitialOffer;
      ServerSubscriptionHandle mServerSub;
};

}

#endif
