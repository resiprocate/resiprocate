#include <sipstack/Message.hxx>

std::ostream& 
Vocal2::operator<<(std::ostream& strm, const Vocal2::Message& msg)
{
    msg.encode(strm);
   return strm;
}
