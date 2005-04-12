#include "resiprocate/TransactionUserMessage.hxx"

using namespace resip;

TransactionUserMessage::TransactionUserMessage(Type type, TransactionUser* tu) :
   mType(type),
   mTU(tu)
{
}

Data 
TransactionUserMessage::brief() const
{ 
   return ("TransactionUserMessage");
}

std::ostream& 
TransactionUserMessage::encode(std::ostream& strm) const
{
   return strm << brief(); 
}

const Data& 
TransactionUserMessage::getTransactionId() const
{
   assert(0);
   return Data::Empty;
}

bool 
TransactionUserMessage::isClientTransaction() const
{
   assert(0);
   return false;
}

