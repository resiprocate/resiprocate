
class DialogId
{
   public:
      DialogId( SipMessages& msg );
      DialogId( Data& callId, Data& senderRequestFromTag, Data& otherTag );
      DialogId( DialogSetId id, Data& otherTag );
      
      DialogSetId getDialogSetId() const;
      
  private:
      Data& mId;
};

   
