#if !defined(RESIP_INVITESESSIONCREATOR_HXX)
#define RESIP_INVITESESSIONCREATOR_HXX

namespace resip
{

class InviteSessionCreator : public BaseCreator
{
   public:
      InviteSessionCreator(const Uri& aor, SdpContents* initial);
      void end();
      
   private:
      typedef enum
      {
         Trying, 
         Proceeding
      } State;
      
      State mState;

      SdpContents* initialOffer;
      
};

}

#endif
