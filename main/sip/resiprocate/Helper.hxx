#ifndef Helper_hxx
#define Helper_hxx

include <Vocal2/SipMessage.hxx>

namespace Vocal2
{
class Helper
{
      static Contact convertNameAddrToContact(NameAddr& nameAddr);
      static SipMessage makeResponse(SipMessage& request, int responseCode);
      static void setNameAddr(RequestLineComponent& rLine, NameAddr& nameAddr);
};
 
}

#endif
