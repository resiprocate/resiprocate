#include "resip/stack/StunMessage.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

StunMessage::StunMessage(const Transport* fromWire)
   : TransactionMessage(fromWire)
{
}

StunMessage::StunMessage(const StunMessage& from)
{
   *this = from;
}

Message*
StunMessage::clone() const
{
   return new StunMessage(*this);
}

StunMessage& 
StunMessage::operator=(const StunMessage& rhs)
{
   if (this != &rhs)
   {
      TransactionMessage::operator=(rhs);

      mStunMessageStruct = rhs.mStunMessageStruct;
   }

   return *this;
}

StunMessage::~StunMessage()
{
}

void
StunMessage::cleanUp()
{   
   for (vector<char*>::iterator i = mBufferList.begin();
        i != mBufferList.end(); i++)
   {
      delete [] *i;
   }
   mBufferList.clear();
}

const Data& 
StunMessage::getTransactionId() const
{
   // return Stun Tid
   return Data::Empty;
}

