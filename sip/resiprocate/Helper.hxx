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

      //to, requestLine& cseq method set
      static SipMessage makeRequest(Url& target, MethodTypes method);

      // copy the values from Url into rline (with sip-uri parameters?)
      static void setUri(RequestLineComponent& rLine, Url& url);
      static void setUri(ResponeLineComponent& rLine, Url& url);
      
      static SipMessage makeAck(SipMessage& request, SipMessage& response);

      static Data computeUniqueBranch();
      static Data computeProxyBranch();
};
 
}

#endif
