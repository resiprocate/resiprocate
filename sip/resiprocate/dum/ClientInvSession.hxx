#if !defined(RESIP_CLIENTINVSESSION_HXX)
#define RESIP_CLIENTINVSESSION_HXX

namespace resip
{

class ClientInvSession : public BaseUsage
{
   public:
      
      class Handle
      {
      };

      void offer(SdpContents* offer);
      void answer(SdpContents* answer);
      void end();
      
   private:
      SdpContents* mLocalSdp;
      SdpContents* mRemoteSdp;
      SdpContents* mOutstandingOffer;
};

 
}

#endif
