#if !defined(RESIP_INVITESESSION_HXX)
#define RESIP_INVITESESSION_HXX

#include "resiprocate/sam/BaseUsage.hxx"

namespace resip
{



    class SdpContents;
class DialogUsageManager;


class InviteSession : public BaseUsage
{
   public:
      class Handle
      {
      };

      InviteSession(DialogUsageManager& dum);
      
      /// Called to set the offer that will be used in the next messages that
      /// sends and offer. Does not send an offer 
      virtual void setOffer(SdpContents* offer)=0;
      
      /// Sends an offer in whatever messages is approperate to send one at
      /// this point in the dialog. Must call setOffer before this.
      virtual void sendOfferInAnyMessage()=0;
      
      /// Called to set the answer that will be used in the next messages that
      /// sends and offer. Does not send an answer
      virtual void setAnswer(SdpContents* answer)=0;

      /// Sends an offer in whatever messages is approperate to send one at
      /// this point in the dialog. Must call setAnswer before this. 
      virtual void sendAnswerInAnyMessage()=0;

      /// Makes the dialog end. Depending ont eh current state, this might
      /// results in BYE or CANCEL being sent.
      virtual void end()=0;

      /// Rejects an offer at the SIP level. So this can send a 487 to a
      /// reINVITE or and UPDATE
      virtual void rejectOffer(int statusCode)=0;
      
      const SdpContents* getLocalSdp();
      const SdpContents* getRemoteSdp();

      void process(const SipMessage& msg);

   protected:
      DialogUsageManager& mDum;
      SdpContents* mLocalSdp;
      SdpContents* mRemoteSdp;
      SdpContents* mMyNextOffer;
      SdpContents* mPendingReceivedOffer;
};

}

#endif
