#if !defined(RESIP_DIALOGSETID_HXX)
#define RESIP_DIALOGSETID_HXX

namespace resip
{

    class SipMessage;
    class Data;

class DialogSetId
{
   public:
      DialogSetId( SipMessage& msg );
      DialogSetId( Data& callId, Data& senderRequestFromTag );
      
   private:
      Data& mId;
};

}
#endif
   
