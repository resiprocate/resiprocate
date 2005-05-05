#include "resiprocate/HttpGetMessage.hxx"

using namespace resip;


HttpGetMessage::HttpGetMessage(const Data& tid, 
                               bool success, 
                               const Data& body,
                               const Mime& type)) :
   mTransactionId(tid),
   mSuccess(success),
   mBody(body),
   mType(type)
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
