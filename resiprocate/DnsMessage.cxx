#include <sipstack/DnsMessage.hxx>

using namespace Vocal2;


Data 
DnsMessage::brief() const 
{ 
   return mTransactionId + " DnsMessage.";
}

std::ostream& 
DnsMessage::encode(std::ostream& strm) const
{
   strm << brief();
   return strm;
}
