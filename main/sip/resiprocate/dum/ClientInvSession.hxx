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

      void setOffer(SdpContents* offer);
      void sendOfferInAnyMessage();
      void setAnswer(SdpContents* answer);
      void sendAnswerInAnyMessage();
      void start();
      void end();
      void rejectOffer(int statusCode);
      
      const SdpContents* getLocalSdp();
      const SdpContents* getRemoteSdp();
      
   private:
      SdpContents* mLocalSdp;
      SdpContents* mRemoteSdp;
      SdpContents* mMyNextOffer;
      SdpContents* mPendingReceivedOffer;
};
 
}

#endif
