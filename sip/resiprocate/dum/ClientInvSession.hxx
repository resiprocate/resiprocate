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

      void rejectOffer(int statusCode);
      void setOffer(SdpContents* offer);
      void sendOfferInAnyMessage();
      void setAnswer(SdpContents* answer);
      void sendAnswerInAnyMessage();
      void start();
      void end();
      
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
