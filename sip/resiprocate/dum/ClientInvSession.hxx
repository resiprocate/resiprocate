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
      
      /// Called to set the offer that will be used in the next messages that
      /// sends and offer. Does not send an offer 
      void setOffer(SdpContents* offer);
      
      /// Sends an offer in whatever messages is approperate to send one at
      /// this point in the dialog. Must call setOffer before this.
      void sendOfferInAnyMessage();
      
      /// Called to set the answer that will be used in the next messages that
      /// sends and offer. Does not send an answer
      void setAnswer(SdpContents* answer);

      /// Sends an offer in whatever messages is approperate to send one at
      /// this point in the dialog. Must call setAnswer before this. 
      void sendAnswerInAnyMessage();

      /// Sends an inital INVITE. If this should contain an offer, must call
      /// setOffer before calling this. 
      void start();

      /// Makes the dialog end. Depending ont eh current state, this might
      /// results in BYE or CANCEL being sent.
      void end();

      /// Rejects an offer at the SIP level. So this can send a 487 to a
      /// reINVITE or and UPDATE
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
