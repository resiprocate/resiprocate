#if !defined(RESIP_SERVERINVSESSION_HXX)
#define RESIP_SERVERINVSESSION_HXX

namespace resip
{

class ServerInvSession: public BaseUsage
{
   public:
      class Handle
      {
      };
       
      ServerInvSession(DialogUsageManager& dum, const SipMessage& msg);
      
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
      
      /// Moves the state of the call to connected and sends a 200
      void accept();
      
      /// Sends an provisional response (a 1xx but not 100). This may contain an
      /// offer or answer depending on if setOffer or setAnswer was called
      /// before this.
      void provisional(int statusCode);
      
      /// Rejects an INVITE with a response like 3xx,4xx,5xx, or 6xx. 
      void reject(int statusCode);
      
      /// Rejects an offer at the SIP level. So this can send a 487 to a
      /// reINVITE or and UPDATE
      void rejectOffer(int statusCode);
      
      /// Makes the dialog end. Depending ont eh current state, this might
      /// results in BYE or CANCEL being sent.
      void end();
      
      const SdpContents* getLocalSdp();
      const SdpContents* getRemoteSdp();
      
      void process(const SipMessage& msg);

   private:
      SdpContents* mLocalSdp;
      SdpContents* mRemoteSdp;
      SdpContents* mMyNextOffer;
      SdpContents* mPendingReceivedOffer;
      
};

 
}

#endif
