#include <sipstack/TimerMessage.hxx>

using namespace Vocal2;

TimerMessage::~TimerMessage()
{
}


Data 
TimerMessage::brief() const
{
   assert(0);
}

std::ostream& TimerMessage::dump(std::ostream& strm) const
{
   return strm << "TimerMessage[ mTransactionId["
               << mTransactionId << "] mType"
               << mType << "]]";
   
}


