#ifndef Helper_hxx
#define Helper_hxx

include <Vocal2/SipMessage.hxx>

namespace Vocal2
{
class Helper
{
      static Contact convertNameAddrToContact(NameAddr& nameAddr);  //may go away

      static SipMessage makeResponse(SipMessage& request, int responseCode);
      static SipMessage makeResponse(SipMessage& request, int responseCode, NameAddr& myContact);

      static void setNameAddr(RequestLineComponent& rLine, NameAddr& nameAddr);

      static SipMessage makeAck(SipMessage& request, SipMessage& response);
      
      // copy the values from nameAddr into rline (with sip-uri parameters)
      static void setRequestUri(RequestLineComponent& rLine, NameAddr& nameAddr);

      static Data computeUniqueBranch();
      static Data computeProxyBranch();
};
 
}

#endif
