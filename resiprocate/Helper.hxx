#ifndef Helper_hxx
#define Helper_hxx

include <Vocal2/SipMessage.hxx>

namespace Vocal2
{
class Helper
{
      static SipMessage makeInvite(Url& target);

      static SipMessage makeResponse(SipMessage& request, int responseCode);
      static SipMessage makeResponse(SipMessage& request, int responseCode, NameAddr& myContact);

      // copy the values from Url into rline (with sip-uri parameters?)
      static void setURI(RequestLineComponent& rLine, Url& url);
      static void setURI(ResponeLineComponent& rLine, Url& url);
      
      static SipMessage makeAck(SipMessage& request, SipMessage& response);

      static Data computeUniqueBranch();
      static Data computeProxyBranch();
};
 
}

#endif
