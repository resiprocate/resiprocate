#include <sipstack/TimerMessage.hxx>

using namespace Vocal2;

TimerMessage::~TimerMessage()
{
}

Data 
TimerMessage::brief() const
{
   return mTransactionId;
}

std::ostream& TimerMessage::encode(std::ostream& strm) const
{
   return strm << "TimerMessage TransactionId[" << mTransactionId << "] "
               << " Type[" << mType << "]"
               << " duration[" << mDuration << "]";
}


