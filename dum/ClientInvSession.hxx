class ClientInvSession : public BaseSession
{
   public:
      void offer(SdpContents* offer);
      void answer(SdpContents* answer);
      void end();
      
   private:
      SdpContents* mLocalSdp;
      SdpContents* mRemoteSdp;
      SdpContents* mOutstandingOffer;
};

   
