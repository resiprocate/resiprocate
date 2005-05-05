#include "resiprocate/HttpGetMessage.hxx"

using namespace resip;


HttpGetMessage::HttpGetMessage(const Data& tid, bool success, const Data& x509) :
   mTransactionId(tid),
   mSuccess(success),
   mX509Blob(x509)
{
}

Data 
HttpGetMessage::brief() const
{ 
   return ("HttpGetMessage");
}

std::ostream& 
HttpGetMessage::encode(std::ostream& strm) const
{
   return strm << brief() << mTransactionId; 
}

Message* 
HttpGetMessage::clone() const 
{ 
   return new HttpGetMessage(mTid, mSuccess, mX509Blob); 
}
