#if !defined(RESIP_INVITESESSIONCREATOR_HXX)
#define RESIP_INVITESESSIONCREATOR_HXX

#include "BaseCreator.hxx"

namespace resip
{

class InviteSessionCreator : public BaseCreator
{
   public:
      InviteSessionCreator(const Uri& aor, const SdpContents* initial);
      void end();

      virtual void dispatch(SipMessage& msg);
      
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
