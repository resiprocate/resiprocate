#if !defined(RESIP_CLIENTINVITESESSION_HXX)
#define RESIP_CLIENTINVITESESSION_HXX

#include "resiprocate/sam/InviteSession.hxx"

namespace resip
{

    class SdpContents;

class ClientInviteSession : public InviteSession
{
   public:
      /// Called to set the offer that will be used in the next messages that
      /// sends and offer. Does not send an offer 
      virtual void setOffer(SdpContents* offer);
      
      /// Sends an offer in whatever messages is approperate to send one at
      /// this point in the dialog. Must call setOffer before this.
      virtual void sendOfferInAnyMessage();
      
      /// Called to set the answer that will be used in the next messages that
      /// sends and offer. Does not send an answer
      virtual void setAnswer(SdpContents* answer);

      /// Sends an offer in whatever messages is approperate to send one at
      /// this point in the dialog. Must call setAnswer before this. 
      virtual void sendAnswerInAnyMessage();

      /// Makes the dialog end. Depending ont eh current state, this might
      /// results in BYE or CANCEL being sent.
      virtual void end();

      /// Rejects an offer at the SIP level. So this can send a 487 to a
      /// reINVITE or and UPDATE
      virtual void rejectOffer(int statusCode);
};
 
}

#endif
