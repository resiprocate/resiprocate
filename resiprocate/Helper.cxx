#include <sipstack/Helper.hxx>

using namespace Vocal2;

SipMessage 
Helper::makeInvite(Url& target)
{
   assert(0);
}


SipMessage 
Helper::makeResponse(SipMessage& request, int responseCode)
{
   assert(0);
}

SipMessage 
Helper::makeResponse(SipMessage& request, int responseCode, Url& myContact)
{
   assert(0);
}


//to, requestLine& cseq method set
SipMessage 
Helper::makeRequest(Url& target, MethodTypes method)
{
   assert(0);
}


// copy the values from Url into rline (with sip-uri parameters?)
void 
Helper::setUri(RequestLine& rLine, Url& url)
{
   assert(0);
}

void 
Helper::setUri(StatusLine& rLine, Url& url)
{
   assert(0);
}


SipMessage 
Helper::makeAck(SipMessage& request, SipMessage& response)
{
   assert(0);
}


Data 
Helper::computeUniqueBranch()
{
   assert(0);
}

Data 
Helper::computeProxyBranch()
{
   assert(0);
}

