
using namespace std;

namespace resip
{

class SipMessage;
class Data;
class DialogSetId;

class DialogId
{
   public:
      DialogId(const SipMessage& msg );
      DialogId(const Data& callId, const Data& senderRequestFromTag, const Data& otherTag );
      DialogId(const DialogSetId id, const Data& otherTag );
      
      const DialogSetId& getDialogSetId() const;
      
   private:
      Data& mId;
};

}
   
