
class DialogSetId
{
   public:
      DialogSetId( SipMessages& msg );
      DialogSetId( Data& callId, Data& senderRequestFromTag );
      
   private:
      Data& mId;
};

   
