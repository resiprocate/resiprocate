#include <sipstack/Message.hxx>

std::ostream& 
Vocal2::operator<<(std::ostream& strm, const Vocal2::Message& msg)
{
   return msg.dump(strm);
}
