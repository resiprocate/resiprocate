#ifndef Helper_hxx
#define Helper_hxx

include <Vocal2/SipMessage.hxx>

namespace Vocal2
{
class Helper
{
      static Contact convertNameAddrToContact(const NameAddr& nameAddr);
      static SipMessage makeResponse(SipMessage& request, int responseCode);
      
};
 
}

#endif
