#ifndef Helper_hxx
#define Helper_hxx


#include <sipstack/SipMessage.hxx>
#include <sipstack/Symbols.hxx>

namespace Vocal2
{

class Helper
{
   public:

      //to, maxforwards=70, requestLine& cseq method set, cseq sequence is 1
      static SipMessage makeInvite(Url& target);

      static SipMessage makeResponse(SipMessage& request, int responseCode);
      static SipMessage makeResponse(SipMessage& request, int responseCode, Url& myContact);

      //to, maxforwards=70, requestLine& cseq method set, cseq sequence is 1
      static SipMessage makeRequest(Url& target, MethodTypes method);

      static SipMessage makeRequest(Url& target, 
                                    MethodTypes method,
                                    Url& from);  //will create a tag
                                                 //automatically

      //creates to, from with tag, cseq method set, cseq sequence is 1
      static SipMessage makeRegister(Url& proxy,
                                     Url& aor);

      //should default proto-version, anything else defaulted/passed in?
      static Via makeVia(Url& source);

      // copy the values from Url into rline (with sip-uri parameters?)
      static void setUri(RequestLine& rLine, Url& url);
      static void setUri(StatusLine& rLine, Url& url);
      
      static SipMessage makeAck(SipMessage& request, SipMessage& response);

      static Data computeUniqueBranch();
      static Data computeProxyBranch();

      static Data computeCallId();
};
 
}

#endif
