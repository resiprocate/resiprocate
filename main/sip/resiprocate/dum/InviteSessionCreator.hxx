#if !defined(RESIP_INVITESESSIONCREATOR_HXX)
#define RESIP_INVITESESSIONCREATOR_HXX

#include "BaseCreator.hxx"

namespace resip
{

class DialogUsageManager;
class Uri;
class SdpContents;

class InviteSessionCreator : public BaseCreator
{
   public:
      InviteSessionCreator(DialogUsageManager& dum, const Uri& aor, const SdpContents* initial);
      void end();

      virtual void dispatch(const SipMessage& msg);
      const SdpContents* getInitialOffer() const;
      
   private:
      typedef enum
      {
         Initialized,
         Trying, 
         Proceeding
      } State;
      
      State mState;
      SdpContents* mInitialOffer;
};

}

#endif
