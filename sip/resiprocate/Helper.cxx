#include <sipstack/Helper.hxx>

using namespace Vocal2;

const int Helper::tagSize = 4;

SipMessage 
Helper::makeRequest(const Url& target, 
                    const Url& from,
                    const Url& contact,
                    MethodTypes method)
{
   SipMessage request;
   RequestLine rLine(method);
   Helper::setUri(rLine, target);
   request.header(h_To) = target;
   request.header(h_RequestLine) = rLine;
   request.header(h_MaxForwards).value() = 70;
   request.header(h_CSeq).method() = method;
   request.header(h_CSeq).sequence() = 1;
   request.header(h_From) = from;
   request.header(h_From)[p_tag] = Helper::computeTag(Helper::tagSize);
   request.header(h_Contacts).push_front(contact);
   request.header(h_CallId).value() = Helper::computeCallId();
   return request;
}

SipMessage 
Helper::makeInvite(const Url& target,
                   const Url& from,
                   const Url& contact)
{
   return Helper::makeRequest(target, from, contact, INVITE);
}

SipMessage 
Helper::makeResponse(const SipMessage& request, int responseCode)
{
   SipMessage response;
   response.header(h_StatusLine).responseCode() = responseCode;
   response.header(h_From) = request.header(h_From);
   response.header(h_To) = request.header(h_To);
   response.header(h_CallId) = request.header(h_CallId);
   response.header(h_CSeq) = request.header(h_CSeq);
   response.header(h_Vias) = request.header(h_Vias);

   if (responseCode > 100 && responseCode < 500)
   {
      if (!response.header(h_To).exists(p_tag))
      {
         response.header(h_To)[p_tag] = Helper::computeTag(Helper::tagSize);
      }
   }
   return response;
}

SipMessage 
Helper::makeResponse(const SipMessage& request, int responseCode, const Url& contact)
{
   SipMessage response = Helper::makeResponse(request, responseCode);
   response.header(h_Contacts).push_front(contact);
   return response;
}


//to, requestLine& cseq method set
SipMessage 
Helper::makeRequest(const Url& target, MethodTypes method)
{
   assert(0);
}


// copy the values from Url into rline (with sip-uri parameters?)
void 
Helper::setUri(RequestLine& rLine, const Url& url)
{
   assert(0);
}

void 
Helper::setUri(StatusLine& rLine, const Url& url)
{
   assert(0);
}


SipMessage 
Helper::makeAck(const SipMessage& request, const SipMessage& response)
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

Data
Helper::computeCallId()
{
   assert(0);
}


Data
Helper::computeTag(int numBytes)
{
   assert(0);
}


