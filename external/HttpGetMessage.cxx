#include "resiprocate/external/HttpGetMessage.hxx"

using namespace resip;


HttpGetMessage::HttpGetMessage(const Data& tid, 
                               bool success, 
                               const Data& body,
                               const Mime& type) :
   mTid(tid),
   mSuccess(success),
   mBody(body),
   mType(type)
{
}

Data 
HttpGetMessage::brief() const
{ 
   return ("HttpGetMessage: ") << mTid << " " << mType;   
}

std::ostream& 
HttpGetMessage::encode(std::ostream& strm) const
{
   return strm << brief() << "body: " << mBody;   
}

Message* 
HttpGetMessage::clone() const 
{ 
   return new HttpGetMessage(mTid, mSuccess, mBody, mType); 
}
