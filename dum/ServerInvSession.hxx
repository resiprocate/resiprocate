#if !defined(RESIP_SERVERINVSESSION_HXX)
#define RESIP_SERVERINVSESSION_HXX

namespace resip
{

/** @file ServerInvSession.hxx
 *   @todo This file is empty
 */

class ServerInvSession: public BaseUsage
{
   public:
      class Handle
      {
      };
      
      void setOffer(SdpContents* offer);
      void sendOfferInAnyMessage();
      void setAnswer(SdpContents* answer);
      void sendAnswerInAnyMessage();
      void accept();
      void provisional(int statusCode);
      void reject(int statusCode);
      void rejectOffer(int statusCode);
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
