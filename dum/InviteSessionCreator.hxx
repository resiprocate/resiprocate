#if !defined(RESIP_INVITESESSIONCREATOR_HXX)
#define RESIP_INVITESESSIONCREATOR_HXX

namespace resip
{

class InviteSessionCreator : public BaseCreator
{
   public:
      InviteSessionCreator(const Uri& aor, const SdpContents* initial);
      void end();
      
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
