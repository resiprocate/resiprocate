#ifndef Helper_hxx
#define Helper_hxx

#include <sipstack/SipMessage.hxx>

namespace Vocal2
{

class Helper
{
   public:
      static SipMessage makeInvite(Url& target);

      static SipMessage makeResponse(SipMessage& request, int responseCode);
      static SipMessage makeResponse(SipMessage& request, int responseCode, Url& myContact);

      //to, requestLine& cseq method set
      static SipMessage makeRequest(Url& target, MethodTypes method);

      // copy the values from Url into rline (with sip-uri parameters?)
      static void setUri(RequestLine& rLine, Url& url);
      static void setUri(StatusLine& rLine, Url& url);
      
      static SipMessage makeAck(SipMessage& request, SipMessage& response);

      static Data computeUniqueBranch();
      static Data computeProxyBranch();
};
 
}

#endif
