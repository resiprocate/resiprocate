#if !defined(RESIP_INVSESSIONCREATOR_HXX)
#define RESIP_INVSESSIONCREATOR_HXX

namespace resip
{

class InvSessionCreator : public BaseCreator
{
   public:
      InvSessionCreator(const Uri& aor, SdpContents* initial);
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
