#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <string.h>
#include <iomanip>
#include <algorithm>
#include <memory>

#include "resip/stack/Auth.hxx"
#include "resip/stack/BasicNonceHelper.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/NonceHelper.hxx"
#include "rutil/Coders.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "rutil/Timer.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/MD5Stream.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/compat.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/TransportType.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Pkcs7Contents.hxx"
#include "resip/stack/MultipartSignedContents.hxx"
#include "resip/stack/MultipartMixedContents.hxx"
#include "resip/stack/MultipartAlternativeContents.hxx"
#include "rutil/WinLeakCheck.hxx"

#ifdef USE_SSL
#include "resip/stack/ssl/Security.hxx"
#endif

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

const int Helper::tagSize = 4;

// !jf! this should be settable by the application in case a group of apps
// (e.g. proxies) want to share the same secret
Helper::NonceHelperPtr Helper::mNonceHelperPtr;

void Helper::integer2hex(char* _d, unsigned int _s, bool _l)
{
   int i;
   unsigned char j;
   int k = 0;
   char* s;

   _s = htonl(_s);
   s = (char*)&_s;

   for (i = 0; i < 4; i++) 
   {
      j = (s[i] >> 4) & 0xf;
      if (j <= 9) 
      {
         if(_l || j != 0 || k != 0)
         {
            _d[k++] = (j + '0');
         }
      }
      else 
      {
         _d[k++] = (j + 'a' - 10);
      }

      j = s[i] & 0xf;
      if (j <= 9) 
      {
         if(_l || j != 0 || k != 0)
         {
            _d[k++] = (j + '0');
         }
      }
      else 
      {
         _d[k++] = (j + 'a' - 10);
      }
   }
}

unsigned int Helper::hex2integer(const char* _s)
{
   unsigned int i, res = 0;

   for(i = 0; i < 8; i++) 
   {
      if ((_s[i] >= '0') && (_s[i] <= '9')) 
      {
         res *= 16;
         res += _s[i] - '0';
      }
      else if ((_s[i] >= 'a') && (_s[i] <= 'f')) 
      {
         res *= 16;
         res += _s[i] - 'a' + 10;
      } 
      else if ((_s[i] >= 'A') && (_s[i] <= 'F')) 
      {
         res *= 16;
         res += _s[i] - 'A' + 10;
      }
      else 
      {
         return res;
      }
   }

   return res;
}

SipMessage*
Helper::makeRequest(const NameAddr& target, const NameAddr& from, const NameAddr& contact, MethodTypes method)
{
   std::auto_ptr<SipMessage> request(new SipMessage);
   RequestLine rLine(method);
   rLine.uri() = target.uri();
   request->header(h_To) = target;
   request->header(h_RequestLine) = rLine;
   request->header(h_MaxForwards).value() = 70;
   request->header(h_CSeq).method() = method;
   request->header(h_CSeq).sequence() = 1;
   request->header(h_From) = from;
   request->header(h_From).param(p_tag) = Helper::computeTag(Helper::tagSize);
   request->header(h_Contacts).push_back(contact);
   request->header(h_CallId).value() = Helper::computeCallId();
   //request->header(h_ContentLength).value() = 0;
   
   Via via;
   request->header(h_Vias).push_back(via);
   
   return request.release();
}

SipMessage*
Helper::makeRequest(const NameAddr& target, const NameAddr& from, MethodTypes method)
{
   NameAddr contact;
   return makeRequest(target, from, contact, method);
}

SipMessage*
Helper::makeRegister(const NameAddr& to, const NameAddr& from)
{
   NameAddr contact;
   return makeRegister(to, from, contact);
}

SipMessage*
Helper::makeRegister(const NameAddr& to, const NameAddr& from, const NameAddr& contact)
{
   std::auto_ptr<SipMessage> request(new SipMessage);
   RequestLine rLine(REGISTER);

   rLine.uri().scheme() = to.uri().scheme();
   rLine.uri().host() = to.uri().host();
   rLine.uri().port() = to.uri().port();
   if (to.uri().exists(p_transport))
   {
      rLine.uri().param(p_transport) = to.uri().param(p_transport);
   }

   request->header(h_To) = to;
   request->header(h_RequestLine) = rLine;
   request->header(h_MaxForwards).value() = 70;
   request->header(h_CSeq).method() = REGISTER;
   request->header(h_CSeq).sequence() = 1;
   request->header(h_From) = from;
   request->header(h_From).param(p_tag) = Helper::computeTag(Helper::tagSize);
   request->header(h_CallId).value() = Helper::computeCallId();
   resip_assert(!request->exists(h_Contacts) || request->header(h_Contacts).empty());
   request->header(h_Contacts).push_back( contact );
   
   Via via;
   request->header(h_Vias).push_back(via);
   
   return request.release();
}

SipMessage*
Helper::makeRegister(const NameAddr& to,const Data& transport)
{
   NameAddr contact;
   return makeRegister(to, transport, contact);
   
}

SipMessage*
Helper::makeRegister(const NameAddr& to, const Data& transport, const NameAddr& contact)
{
   std::auto_ptr<SipMessage> request(new SipMessage);
   RequestLine rLine(REGISTER);

   rLine.uri().scheme() = to.uri().scheme();
   rLine.uri().host() = to.uri().host();
   rLine.uri().port() = to.uri().port();
   if (!transport.empty())
   {
      rLine.uri().param(p_transport) = transport;
   }

   request->header(h_To) = to;
   request->header(h_RequestLine) = rLine;
   request->header(h_MaxForwards).value() = 70;
   request->header(h_CSeq).method() = REGISTER;
   request->header(h_CSeq).sequence() = 1;
   request->header(h_From) = to;
   request->header(h_From).param(p_tag) = Helper::computeTag(Helper::tagSize);
   request->header(h_CallId).value() = Helper::computeCallId();
   resip_assert(!request->exists(h_Contacts) || request->header(h_Contacts).empty());
   request->header(h_Contacts).push_back( contact );
   
   Via via;
   request->header(h_Vias).push_back(via);
   
   return request.release();
}


SipMessage*
Helper::makePublish(const NameAddr& target, const NameAddr& from)
{
   NameAddr contact;
   return makePublish(target, from, contact);
}

SipMessage*
Helper::makePublish(const NameAddr& target, const NameAddr& from, const NameAddr& contact)
{
   std::auto_ptr<SipMessage> request(new SipMessage);
   RequestLine rLine(PUBLISH);
   rLine.uri() = target.uri();

   request->header(h_To) = target;
   request->header(h_RequestLine) = rLine;
   request->header(h_MaxForwards).value() = 70;
   request->header(h_CSeq).method() = PUBLISH;
   request->header(h_CSeq).sequence() = 1;
   request->header(h_From) = from;
   request->header(h_From).param(p_tag) = Helper::computeTag(Helper::tagSize);
   request->header(h_CallId).value() = Helper::computeCallId();
   resip_assert(!request->exists(h_Contacts) || request->header(h_Contacts).empty());
   request->header(h_Contacts).push_back( contact );
   Via via;
   request->header(h_Vias).push_back(via);
   
   return request.release();
}

SipMessage*
Helper::makeMessage(const NameAddr& target, const NameAddr& from)
{
   NameAddr contact;
   return makeMessage(target, from, contact);
}

SipMessage*
Helper::makeMessage(const NameAddr& target, const NameAddr& from, const NameAddr& contact)
{
   std::auto_ptr<SipMessage> request(new SipMessage);
   RequestLine rLine(MESSAGE);
   rLine.uri() = target.uri();

   request->header(h_To) = target;
   request->header(h_RequestLine) = rLine;
   request->header(h_MaxForwards).value() = 70;
   request->header(h_CSeq).method() = MESSAGE;
   request->header(h_CSeq).sequence() = 1;
   request->header(h_From) = from;
   request->header(h_From).param(p_tag) = Helper::computeTag(Helper::tagSize);
   request->header(h_CallId).value() = Helper::computeCallId();
   resip_assert(!request->exists(h_Contacts) || request->header(h_Contacts).empty());
   request->header(h_Contacts).push_back( contact );
   Via via;
   request->header(h_Vias).push_back(via);
   
   return request.release();
}


SipMessage*
Helper::makeSubscribe(const NameAddr& target, const NameAddr& from)
{
   NameAddr contact;
   return makeSubscribe(target, from, contact);
}

SipMessage*
Helper::makeSubscribe(const NameAddr& target, const NameAddr& from, const NameAddr& contact)
{
   std::auto_ptr<SipMessage> request(new SipMessage);
   RequestLine rLine(SUBSCRIBE);
   rLine.uri() = target.uri();

   request->header(h_To) = target;
   request->header(h_RequestLine) = rLine;
   request->header(h_MaxForwards).value() = 70;
   request->header(h_CSeq).method() = SUBSCRIBE;
   request->header(h_CSeq).sequence() = 1;
   request->header(h_From) = from;
   request->header(h_From).param(p_tag) = Helper::computeTag(Helper::tagSize);
   request->header(h_CallId).value() = Helper::computeCallId();
   resip_assert(!request->exists(h_Contacts) || request->header(h_Contacts).empty());
   request->header(h_Contacts).push_front( contact );
   Via via;
   request->header(h_Vias).push_front(via);
   
   return request.release();
}

int
Helper::jitterValue(int input, int lowerPercentage, int upperPercentage, int minimum)
{
   resip_assert(upperPercentage >= lowerPercentage);
   if (input < minimum)
   {
      return input;
   }
   else if (lowerPercentage == 100 && upperPercentage == 100)
   {
      return input;
   }
   else
   {
      const int rnd = Random::getRandom() % (upperPercentage-lowerPercentage) + lowerPercentage;
      return (input * rnd) / 100;
   }
}

SipMessage*
Helper::makeInvite(const NameAddr& target, const NameAddr& from)
{
   return Helper::makeRequest(target, from, INVITE);
}

SipMessage*
Helper::makeInvite(const NameAddr& target, const NameAddr& from, const NameAddr& contact)
{
   return Helper::makeRequest(target, from, contact, INVITE);
}


void
Helper::makeResponse(SipMessage& response, 
                     const SipMessage& request, 
                     int responseCode, 
                     const NameAddr& myContact, 
                     const Data& reason,
                     const Data& hostname,
                     const Data& warning)
{
   makeResponse(response,request, responseCode, reason,hostname, warning);
   // in general, this should not create a Contact header since only requests
   // that create a dialog (or REGISTER requests) should produce a response with
   // a contact(s). 
   response.header(h_Contacts).clear();
   response.header(h_Contacts).push_back(myContact);
}


void
Helper::makeResponse(SipMessage& response, 
                     const SipMessage& request, 
                     int responseCode, 
                     const Data& reason,
                     const Data& hostname,
                     const Data& warning)
{
   DebugLog(<< "Helper::makeResponse(" << request.brief() << " code=" << responseCode << " reason=" << reason);
   response.header(h_StatusLine).responseCode() = responseCode;
   response.header(h_From) = request.header(h_From);
   response.header(h_To) = request.header(h_To);
   response.header(h_CallId) = request.header(h_CallId);
   response.header(h_CSeq) = request.header(h_CSeq);
   response.header(h_Vias) = request.header(h_Vias);

   if (!warning.empty())
   {
      WarningCategory warn;
      warn.code() = 399;
      warn.hostname() = hostname;
      warn.text() = warning;
      response.header(h_Warnings).push_back(warn);
   }

   if(responseCode > 100 &&
      response.const_header(h_To).isWellFormed() &&
      !response.const_header(h_To).exists(p_tag))
   {
      // Only generate a To: tag if one doesn't exist.  Think Re-INVITE.   
      // No totag for failure responses or 100s   
      // ?bwc? Should we be generating to-tags for failure responses or not?
      // The comments say no, but the code says yes. Which is it?
      response.header(h_To).param(p_tag) = Helper::computeTag(Helper::tagSize);
   }

   // .bwc. This will only throw if the topmost Via is malformed, and that 
   // should have been caught at the transport level.
   response.setRFC2543TransactionId(request.getRFC2543TransactionId());
   
   //response.header(h_ContentLength).value() = 0;
   
   if (responseCode >= 180 && responseCode < 300 && request.exists(h_RecordRoutes))
   {
      response.header(h_RecordRoutes) = request.header(h_RecordRoutes);
   }

   // .bwc. If CSeq is malformed, basicCheck would have already attempted to
   // parse it, meaning we won't throw here (we never try to parse the same
   // thing twice, see LazyParser::checkParsed())
   if (responseCode/100 == 2 &&
         !response.exists(h_Contacts) &&
         !(response.const_header(h_CSeq).method()==CANCEL) )
   {
      // in general, this should not create a Contact header since only requests
      // that create a dialog (or REGISTER requests) should produce a response with
      // a contact(s). 
      
      NameAddr contact;
      response.header(h_Contacts).push_back(contact);
   }

   if (request.isExternal())
   {
       response.setFromTU();
   }
   else
   {
       // This makes a response to an internally generated request look like it's 
       // external even though it isn't
       response.setFromExternal();
   }

   if (reason.size())
   {
      response.header(h_StatusLine).reason() = reason;
   }
   else
   {
      getResponseCodeReason(responseCode, response.header(h_StatusLine).reason());
   }
}

SipMessage*
Helper::makeResponse(const SipMessage& request, 
                     int responseCode, 
                     const NameAddr& myContact, 
                     const Data& reason, 
                     const Data& hostname, 
                     const Data& warning)
{
   // .bwc. Exception safety. Catch/rethrow is dicey because we can't rethrow
   // resip::BaseException, since it is abstract.
   std::auto_ptr<SipMessage> response(new SipMessage);

   makeResponse(*response, request, responseCode, reason, hostname, warning);

   // in general, this should not create a Contact header since only requests
   // that create a dialog (or REGISTER requests) should produce a response with
   // a contact(s). 
   response->header(h_Contacts).clear();
   response->header(h_Contacts).push_back(myContact);
   return response.release();
}


SipMessage*
Helper::makeResponse(const SipMessage& request, 
                     int responseCode, 
                     const Data& reason, 
                     const Data& hostname, 
                     const Data& warning)
{
   // .bwc. Exception safety. Catch/rethrow is dicey because we can't rethrow
   // resip::BaseException, since it is abstract.
   std::auto_ptr<SipMessage> response(new SipMessage);
   
   makeResponse(*response, request, responseCode, reason, hostname, warning);
   return response.release();
}

void
Helper::makeRawResponse(Data& raw,
                        const SipMessage& msg, 
                        int responseCode,
                        const Data& additionalHeaders,
                        const Data& body)
{
   raw.reserve(256);
   {
      DataStream encodeStream(raw);
      encodeStream << "SIP/2.0 " << responseCode << " ";
      Data reason;
      getResponseCodeReason(responseCode, reason);
      encodeStream << reason;
      msg.encodeSingleHeader(Headers::Via,encodeStream);
      msg.encodeSingleHeader(Headers::To,encodeStream);
      msg.encodeSingleHeader(Headers::From,encodeStream);
      msg.encodeSingleHeader(Headers::CallID,encodeStream);
      msg.encodeSingleHeader(Headers::CSeq,encodeStream);
      encodeStream << additionalHeaders;
      encodeStream << "Content-Length: " << body.size() << "\r\n\r\n";
   }
}

void   
Helper::getResponseCodeReason(int responseCode, Data& reason)
{
   switch (responseCode)
   {
      case 100: reason = "Trying"; break;
      case 180: reason = "Ringing"; break;
      case 181: reason = "Call Is Being Forwarded"; break;
      case 182: reason = "Queued"; break;
      case 183: reason = "Session Progress"; break;
      case 200: reason = "OK"; break;
      case 202: reason = "Accepted"; break;
      case 300: reason = "Multiple Choices"; break;
      case 301: reason = "Moved Permanently"; break;
      case 302: reason = "Moved Temporarily"; break;
      case 305: reason = "Use Proxy"; break;
      case 380: reason = "Alternative Service"; break;
      case 400: reason = "Bad Request"; break;
      case 401: reason = "Unauthorized"; break;
      case 402: reason = "Payment Required"; break;
      case 403: reason = "Forbidden"; break;
      case 404: reason = "Not Found"; break;
      case 405: reason = "Method Not Allowed"; break;
      case 406: reason = "Not Acceptable"; break;
      case 407: reason = "Proxy Authentication Required"; break;
      case 408: reason = "Request Timeout"; break;
      case 410: reason = "Gone"; break;
      case 412: reason = "Precondition Failed"; break;
      case 413: reason = "Request Entity Too Large"; break;
      case 414: reason = "Request-URI Too Long"; break;
      case 415: reason = "Unsupported Media Type"; break;
      case 416: reason = "Unsupported URI Scheme"; break;
      case 420: reason = "Bad Extension"; break;
      case 421: reason = "Extension Required"; break;
      case 422: reason = "Session Interval Too Small"; break;
      case 423: reason = "Interval Too Brief"; break;
      case 430: reason = "Flow failed"; break;
      case 439: reason = "First Hop Lacks Outbound Support"; break;
      case 480: reason = "Temporarily Unavailable"; break;
      case 481: reason = "Call/Transaction Does Not Exist"; break;
      case 482: reason = "Loop Detected"; break;
      case 483: reason = "Too Many Hops"; break;
      case 484: reason = "Address Incomplete"; break;
      case 485: reason = "Ambiguous"; break;
      case 486: reason = "Busy Here"; break;
      case 487: reason = "Request Terminated"; break;
      case 488: reason = "Not Acceptable Here"; break;
      case 489: reason = "Event Package Not Supported"; break;
      case 491: reason = "Request Pending"; break;
      case 493: reason = "Undecipherable"; break;
      case 500: reason = "Server Internal Error"; break;
      case 501: reason = "Not Implemented"; break;
      case 502: reason = "Bad Gateway"; break;
      case 503: reason = "Service Unavailable"; break;
      case 504: reason = "Server Time-out"; break;
      case 505: reason = "Version Not Supported"; break;
      case 513: reason = "Message Too Large"; break;
      case 600: reason = "Busy Everywhere"; break;
      case 603: reason = "Decline"; break;
      case 604: reason = "Does Not Exist Anywhere"; break;
      case 606: reason = "Not Acceptable"; break;
   }
}

SipMessage*
Helper::makeCancel(const SipMessage& request)
{
   resip_assert(request.isRequest());
   resip_assert(request.header(h_RequestLine).getMethod() == INVITE);
   std::auto_ptr<SipMessage> cancel(new SipMessage);

   RequestLine rLine(CANCEL, request.header(h_RequestLine).getSipVersion());
   rLine.uri() = request.header(h_RequestLine).uri();
   cancel->header(h_RequestLine) = rLine;
   cancel->header(h_MaxForwards).value() = 70;
   cancel->header(h_To) = request.header(h_To);
   cancel->header(h_From) = request.header(h_From);
   cancel->header(h_CallId) = request.header(h_CallId);
   if (request.exists(h_ProxyAuthorizations))
   {
      cancel->header(h_ProxyAuthorizations) = request.header(h_ProxyAuthorizations);
   }
   if (request.exists(h_Authorizations))
   {
      cancel->header(h_Authorizations) = request.header(h_Authorizations);
   }
   
   if (request.exists(h_Routes))
   {
      cancel->header(h_Routes) = request.header(h_Routes);
   }
   
   cancel->header(h_CSeq) = request.header(h_CSeq);
   cancel->header(h_CSeq).method() = CANCEL;
   cancel->header(h_Vias).push_back(request.header(h_Vias).front());

   return cancel.release();
}


SipMessage*
Helper::makeFailureAck(const SipMessage& request, const SipMessage& response)
{
   resip_assert (request.header(h_Vias).size() >= 1);
   resip_assert (request.header(h_RequestLine).getMethod() == INVITE);
   
   std::auto_ptr<SipMessage> ack(new SipMessage);

   RequestLine rLine(ACK, request.header(h_RequestLine).getSipVersion());
   rLine.uri() = request.header(h_RequestLine).uri();
   ack->header(h_RequestLine) = rLine;
   ack->header(h_MaxForwards).value() = 70;
   ack->header(h_CallId) = request.header(h_CallId);
   ack->header(h_From) = request.header(h_From);
   ack->header(h_To) = response.header(h_To); // to get to-tag
   ack->header(h_Vias).push_back(request.header(h_Vias).front());
   ack->header(h_CSeq) = request.header(h_CSeq);
   ack->header(h_CSeq).method() = ACK;
   if (request.exists(h_Routes))
   {
      ack->header(h_Routes) = request.header(h_Routes);
   }
   
   return ack.release();
}


static const Data cookie("z9hG4bK"); // magic cookie per rfc3261   
Data 
Helper::computeUniqueBranch()
{
   Data result(16, Data::Preallocate);
   result += cookie;
   result += Random::getRandomHex(4);
   result += "C1";
   result += Random::getRandomHex(2);
   return result;
}

Data
Helper::computeCallId()
{
   Data hostAndSalt(DnsUtil::getLocalHostName() + Random::getRandomHex(16));
#ifndef USE_SSL // .bwc. None of this is neccessary if we're using openssl
#if defined(__linux__) || defined(__APPLE__)
   pid_t pid = getpid();
   hostAndSalt.append((char*)&pid,sizeof(pid));
#endif
#ifdef __APPLE__
   pthread_t thread = pthread_self();
   hostAndSalt.append((char*)&thread,sizeof(thread));
#endif
#ifdef WIN32
   DWORD proccessId = ::GetCurrentProcessId();
   DWORD threadId = ::GetCurrentThreadId();
   hostAndSalt.append((char*)&proccessId,sizeof(proccessId));
   hostAndSalt.append((char*)&threadId,sizeof(threadId));
#endif
#endif // of USE_SSL
   return hostAndSalt.md5(Data::BASE64);
}

Data
Helper::computeTag(int numBytes)
{
   return Random::getRandomHex(numBytes);
}

void
Helper::setNonceHelper(NonceHelper *nonceHelper)
{
   mNonceHelperPtr.mNonceHelper = nonceHelper;
}

NonceHelper* 
Helper::getNonceHelper()
{
   if (mNonceHelperPtr.mNonceHelper == 0)
   {
      mNonceHelperPtr.mNonceHelper = new BasicNonceHelper();
   }
   return mNonceHelperPtr.mNonceHelper;
}


Data
Helper::makeNonce(const SipMessage& request, const Data& timestamp)
{
   return getNonceHelper()->makeNonce(request, timestamp);
}

static Data noBody = MD5Stream().getHex();
Data 
Helper::makeResponseMD5WithA1(const Data& a1,
                              const Data& method, const Data& digestUri, const Data& nonce,
                              const Data& qop, const Data& cnonce, const Data& cnonceCount,
                              const Contents* entityBody)
{
   MD5Stream a2;
   a2 << method
      << Symbols::COLON
      << digestUri;

   if (qop == Symbols::authInt)
   {
      if (entityBody)
      {
         MD5Stream eStream;
         eStream << *entityBody;
         a2 << Symbols::COLON << eStream.getHex();
      }
      else
      {
         a2 << Symbols::COLON << noBody;
      }
   }
   
   MD5Stream r;
   r << a1
     << Symbols::COLON
     << nonce
     << Symbols::COLON;

   if (!qop.empty())
   {
      r << cnonceCount
        << Symbols::COLON
        << cnonce
        << Symbols::COLON
        << qop
        << Symbols::COLON;
   }
   r << a2.getHex();

   return r.getHex();
}

//RFC 2617 3.2.2.1
Data 
Helper::makeResponseMD5(const Data& username, const Data& password, const Data& realm, 
                        const Data& method, const Data& digestUri, const Data& nonce,
                        const Data& qop, const Data& cnonce, const Data& cnonceCount,
                        const Contents *entity)
{
   MD5Stream a1;
   a1 << username
      << Symbols::COLON
      << realm
      << Symbols::COLON
      << password;
 
   return makeResponseMD5WithA1(a1.getHex(), method, digestUri, nonce, qop, 
                                cnonce, cnonceCount, entity);
}

static Data digest("digest");
std::pair<Helper::AuthResult,Data>
Helper::advancedAuthenticateRequest(const SipMessage& request, 
                                    const Data& realm,
                                    const Data& a1,
                                    int expiresDelta,
                                    bool proxyAuthorization)
{
   Data username;
   DebugLog(<< "Authenticating: realm=" << realm << " expires=" << expiresDelta);
   //DebugLog(<< request);
   
   const ParserContainer<Auth>* auths = 0;
   if(proxyAuthorization)
   {
      if(request.exists(h_ProxyAuthorizations))
      {
         auths = &request.header(h_ProxyAuthorizations);
      }
   }
   else
   {
      if(request.exists(h_Authorizations))
      {
         auths = &request.header(h_Authorizations);
      }
   }

   if (auths)
   {
      for (ParserContainer<Auth>::const_iterator i = auths->begin(); i != auths->end(); i++)
      {
         if (i->exists(p_realm) && 
             i->exists(p_nonce) &&
             i->exists(p_response) &&
             i->param(p_realm) == realm)
         {
            if(!isEqualNoCase(i->scheme(),digest))
            {
               DebugLog(<< "Scheme must be Digest");
               continue;
            }
            /* ParseBuffer pb(i->param(p_nonce).data(), i->param(p_nonce).size());
            if (!pb.eof() && !isdigit(*pb.position()))
            {
               DebugLog(<< "Invalid nonce; expected timestamp.");
               return make_pair(BadlyFormed,username);
            }
            const char* anchor = pb.position();
            pb.skipToChar(Symbols::COLON[0]);

            if (pb.eof())
            {
               DebugLog(<< "Invalid nonce; expected timestamp terminator.");
               return make_pair(BadlyFormed,username);
            }

            Data then;
            pb.data(then, anchor); 
            if (expiresDelta > 0)
            {
               unsigned int now = (unsigned int)(Timer::getTimeMs()/1000);
               if ((unsigned int)then.convertUInt64() + expiresDelta < now)
               {
                  DebugLog(<< "Nonce has expired.");
                  return make_pair(Expired,username);
               }
            } */

            NonceHelper::Nonce x_nonce = getNonceHelper()->parseNonce(i->param(p_nonce)); 
            if(x_nonce.getCreationTime() == 0) 
               return make_pair(BadlyFormed,username);

            if (expiresDelta > 0)
            {
               UInt64 now = Timer::getTimeSecs();
               if (x_nonce.getCreationTime() + expiresDelta < now)
               {
                  DebugLog(<< "Nonce has expired.");
                  return make_pair(Expired,username);
               }
            }

            Data then(x_nonce.getCreationTime());
            if (i->param(p_nonce) != makeNonce(request, then))
            {
               InfoLog(<< "Not my nonce. expected=" << makeNonce(request, then) 
                       << " received=" << i->param(p_nonce)
                       << " then=" << then);
               
               return make_pair(BadlyFormed,username);
            }
         
            if (i->exists(p_qop))
            {
               if (i->param(p_qop) == Symbols::auth || i->param(p_qop)  == Symbols::authInt)
               {
                  if(i->exists(p_uri) && i->exists(p_cnonce) && i->exists(p_nc))
                  {
                     if (i->param(p_response) == makeResponseMD5WithA1(a1,
                                                               getMethodName(request.header(h_RequestLine).getMethod()),
                                                               i->param(p_uri),
                                                               i->param(p_nonce),
                                                               i->param(p_qop),
                                                               i->param(p_cnonce),
                                                               i->param(p_nc),
                                                               request.getContents()))
                     {
                        if(i->exists(p_username))
                        {
                           username = i->param(p_username);
                        }
                        return make_pair(Authenticated,username);
                     }
                     else
                     {
                        return make_pair(Failed,username);
                     }
                  }
               }
               else
               {
                  InfoLog (<< "Unsupported qop=" << i->param(p_qop));
                  return make_pair(Failed,username);
               }
            }
            else if(i->exists(p_uri))
            {
               if (i->param(p_response) == makeResponseMD5WithA1(a1,
                                                               getMethodName(request.header(h_RequestLine).getMethod()),
                                                               i->param(p_uri),
                                                               i->param(p_nonce)))
               {
                  if(i->exists(p_username))
                  {
                     username = i->param(p_username);
                  }
                  return make_pair(Authenticated,username);
               }
               else
               {
                  return make_pair(Failed,username);
               }
            }
         }
         else
         {
            return make_pair(BadlyFormed,username);
         }
      }
      return make_pair(BadlyFormed,username);
   }
   DebugLog (<< "No authentication headers. Failing request.");
   return make_pair(Failed,username);
}

Helper::AuthResult
Helper::authenticateRequest(const SipMessage& request, 
                            const Data& realm,
                            const Data& password,
                            int expiresDelta)
{
   DebugLog(<< "Authenticating: realm=" << realm << " expires=" << expiresDelta);
   //DebugLog(<< request);

   // !bwc! Somewhat inefficient. Maybe optimize later.
   ParserContainer<Auth> auths;

   if(request.exists(h_ProxyAuthorizations))
   {
      auths.append(request.header(h_ProxyAuthorizations));
   }
   
   if(request.exists(h_Authorizations))
   {
      auths.append(request.header(h_Authorizations));
   }

   if(auths.empty())
   {
      DebugLog (<< "No authentication headers. Failing request.");
      return Failed;
   }

   // ?bwc? Why is const_iterator& operator=(const iterator& rhs)
   // not working properly?
   //ParserContainer<Auth>::const_iterator i = auths.begin();
   
   ParserContainer<Auth>::iterator i = auths.begin();
         
   for (; i != auths.end(); i++)
   {
      if (i->exists(p_realm) && 
          i->exists(p_nonce) &&
          i->exists(p_response) &&
          i->param(p_realm) == realm)
      {
         if(!isEqualNoCase(i->scheme(),digest))
         {
            DebugLog(<< "Scheme must be Digest");
            continue;
         }
         /*
         ParseBuffer pb(i->param(p_nonce).data(), i->param(p_nonce).size());
         if (!pb.eof() && !isdigit(*pb.position()))
         {
            DebugLog(<< "Invalid nonce; expected timestamp.");
            return BadlyFormed;
         }
         const char* anchor = pb.position();
         pb.skipToChar(Symbols::COLON[0]);

         if (pb.eof())
         {
            DebugLog(<< "Invalid nonce; expected timestamp terminator.");
            return BadlyFormed;
         }

         Data then;
         pb.data(then, anchor);
         */
         NonceHelper::Nonce x_nonce = getNonceHelper()->parseNonce(i->param(p_nonce));
         if(x_nonce.getCreationTime() == 0)
            return BadlyFormed;


         
         
         
         if (expiresDelta > 0)
         {
            UInt64 now = Timer::getTimeSecs();
            if (x_nonce.getCreationTime() + expiresDelta < now)
            {
               DebugLog(<< "Nonce has expired.");
               return Expired;
            }
         }

         Data then(x_nonce.getCreationTime());
         if (i->param(p_nonce) != makeNonce(request, then))
         {
            InfoLog(<< "Not my nonce.");
            return Failed;
         }
      
         InfoLog (<< " username=" << (i->param(p_username))
                  << " password=" << password
                  << " realm=" << realm
                  << " method=" << getMethodName(request.header(h_RequestLine).getMethod())
                  << " uri=" << i->param(p_uri)
                  << " nonce=" << i->param(p_nonce));
         
         if (i->exists(p_qop))
         {
            if (i->param(p_qop) == Symbols::auth || i->param(p_qop) == Symbols::authInt)
            {
               if(i->exists(p_uri) && i->exists(p_cnonce) && i->exists(p_nc))
               {
                  if (i->param(p_response) == makeResponseMD5(i->param(p_username), 
                                                            password,
                                                            realm, 
                                                            getMethodName(request.header(h_RequestLine).getMethod()),
                                                            i->param(p_uri),
                                                            i->param(p_nonce),
                                                            i->param(p_qop),
                                                            i->param(p_cnonce),
                                                            i->param(p_nc),
                                                            request.getContents()))
                  {
                     return Authenticated;
                  }
                  else
                  {
                     return Failed;
                  }
               }
            }
            else
            {
               InfoLog (<< "Unsupported qop=" << i->param(p_qop));
               return Failed;
            }
         }
         else if(i->exists(p_uri))
         {
         
            if (i->param(p_response) == makeResponseMD5(i->param(p_username), 
                                                            password,
                                                            realm, 
                                                            getMethodName(request.header(h_RequestLine).getMethod()),
                                                            i->param(p_uri),
                                                            i->param(p_nonce)))
            {
               return Authenticated;
            }
            else
            {
               return Failed;
            }
         }
      }
      else
      {
         return BadlyFormed;
      }
   }

   return BadlyFormed;

}

Helper::AuthResult
Helper::authenticateRequestWithA1(const SipMessage& request, 
                                  const Data& realm,
                                  const Data& hA1,
                                  int expiresDelta)
{
   DebugLog(<< "Authenticating with HA1: realm=" << realm << " expires=" << expiresDelta);
   //DebugLog(<< request);

   // !bwc! Somewhat inefficient. Maybe optimize later.
   ParserContainer<Auth> auths;
   
   if(request.exists(h_ProxyAuthorizations))
   {
      auths.append(request.header(h_ProxyAuthorizations));
   }
   
   if(request.exists(h_Authorizations))
   {
      auths.append(request.header(h_Authorizations));
   }

   if(auths.empty())
   {
      DebugLog (<< "No authentication headers. Failing request.");
      return Failed;
   }

   // ?bwc? Why is const_iterator& operator=(const iterator& rhs)
   // not working properly?
   //ParserContainer<Auth>::const_iterator i = auths.begin();
   
   ParserContainer<Auth>::iterator i = auths.begin();
      
   for (;i != auths.end(); i++) 
   {
      if (i->exists(p_realm) && 
          i->exists(p_nonce) &&
          i->exists(p_response) &&
          i->param(p_realm) == realm)
      {
         if(!isEqualNoCase(i->scheme(),digest))
         {
            DebugLog(<< "Scheme must be Digest");
            continue;
         }
         /*
         ParseBuffer pb(i->param(p_nonce).data(), i->param(p_nonce).size());
         if (!pb.eof() && !isdigit(*pb.position()))
         {
            DebugLog(<< "Invalid nonce; expected timestamp.");
            return BadlyFormed;
         }
         const char* anchor = pb.position();
         pb.skipToChar(Symbols::COLON[0]);

         if (pb.eof())
         {
            DebugLog(<< "Invalid nonce; expected timestamp terminator.");
            return BadlyFormed;
         }

         Data then;
         pb.data(then, anchor);
         */

         NonceHelper::Nonce x_nonce = getNonceHelper()->parseNonce(i->param(p_nonce));
         if(x_nonce.getCreationTime() == 0)
            return BadlyFormed;



         if (expiresDelta > 0)
         {
            UInt64 now = Timer::getTimeSecs();
            if (x_nonce.getCreationTime() + expiresDelta < now)
            {
               DebugLog(<< "Nonce has expired.");
               return Expired;
            }
         }

         Data then(x_nonce.getCreationTime());

         if (i->param(p_nonce) != makeNonce(request, then))
         {
            InfoLog(<< "Not my nonce.");
            return Failed;
         }
      
         InfoLog (<< " username=" << (i->param(p_username))
                  << " H(A1)=" << hA1
                  << " realm=" << realm
                  << " method=" << getMethodName(request.header(h_RequestLine).getMethod())
                  << " uri=" << i->param(p_uri)
                  << " nonce=" << i->param(p_nonce));
         
         if (i->exists(p_qop))
         {
            if (i->param(p_qop) == Symbols::auth || i->param(p_qop) == Symbols::authInt)
            {
               if(i->exists(p_uri) && i->exists(p_cnonce) && i->exists(p_nc))
               {
                  if (i->param(p_response) == makeResponseMD5WithA1(hA1, 
                                                                  getMethodName(request.header(h_RequestLine).getMethod()),
                                                                  i->param(p_uri),
                                                                  i->param(p_nonce),
                                                                  i->param(p_qop),
                                                                  i->param(p_cnonce),
                                                                  i->param(p_nc),
                                                                  request.getContents()))
                  {
                     return Authenticated;
                  }
                  else
                  {
                     return Failed;
                  }
               }
            }
            else
            {
               InfoLog (<< "Unsupported qop=" << i->param(p_qop));
               return Failed;
            }
         }
         else if(i->exists(p_uri))
         {
            if (i->param(p_response) == makeResponseMD5WithA1(hA1,
                                                                  getMethodName(request.header(h_RequestLine).getMethod()),
                                                                  i->param(p_uri),
                                                                  i->param(p_nonce)))
            {
               return Authenticated;
            }
            else
            {
               return Failed;
            }
         }
      }
      else
      {
         return BadlyFormed;
      }
   }

   return BadlyFormed;

}

SipMessage*
Helper::make405(const SipMessage& request,
                const int* allowedMethods,
                int len )
{
    SipMessage* resp = Helper::makeResponse(request, 405);

    if (len < 0)
    {
        int upperBound = static_cast<int>(MAX_METHODS);

        // The UNKNOWN method name is the first in the enum
        for (int i = 1 ; i < upperBound; i ++)
        {
            int last = 0;

            // ENUMS must be contiguous in order for this to work.
            resip_assert( i - last <= 1);
            Token t;
            t.value() = getMethodName(static_cast<resip::MethodTypes>(i));
            resp->header(h_Allows).push_back(t);
            last = i;
        }
    }
    else
    {
        // use user's list
        for ( int i = 0 ; i < len ; i++)
        {
            Token t;
            t.value() = getMethodName(static_cast<resip::MethodTypes>(allowedMethods[i]));
            resp->header(h_Allows).push_back(t);
        }
    }
    return resp;
}


SipMessage*
Helper::makeProxyChallenge(const SipMessage& request, const Data& realm, bool useAuth, bool stale)
{
   return makeChallenge(request,realm,useAuth,stale,true);
}

SipMessage*
Helper::makeWWWChallenge(const SipMessage& request, const Data& realm, bool useAuth, bool stale)
{
   return makeChallenge(request,realm,useAuth,stale,false);
}

SipMessage*
Helper::makeChallenge(const SipMessage& request, const Data& realm, bool useAuth, bool stale, bool proxy)
{
   Auth auth;
   auth.scheme() = Symbols::Digest;
   Data timestamp(Timer::getTimeSecs());
   auth.param(p_nonce) = makeNonce(request, timestamp);
   auth.param(p_algorithm) = "MD5";
   auth.param(p_realm) = realm;
   if (useAuth)
   {
      auth.param(p_qopOptions) = "auth,auth-int";
   }
   if (stale)
   {
      auth.param(p_stale) = "true";
   }
   SipMessage *response;
   if(proxy)
   {
      response = Helper::makeResponse(request, 407);
      response->header(h_ProxyAuthenticates).push_back(auth);
   }
   else
   {
      response = Helper::makeResponse(request, 401);
      response->header(h_WWWAuthenticates).push_back(auth);
   }
   return response;
}

void 
Helper::updateNonceCount(unsigned int& nonceCount, Data& nonceCountString)
{
   if (!nonceCountString.empty())
   {
      return;
   }
   nonceCount++;
   {
      //DataStream s(nonceCountString);
      
      //s << std::setw(8) << std::setfill('0') << std::hex << nonceCount;
	   char buf[128];
	   *buf = 0;

#if (defined(_MSC_VER) && _MSC_VER >= 1400)
	   sprintf_s(buf,128,"%08x",nonceCount);
#else
	   sprintf(buf,"%08x",nonceCount);
#endif
	   nonceCountString = buf;
   }
   DebugLog(<< "nonceCount is now: [" << nonceCountString << "]");
}


Auth 
Helper::makeChallengeResponseAuth(const SipMessage& request,
                                  const Data& username,
                                  const Data& password,
                                  const Auth& challenge,
                                  const Data& cnonce,
                                  unsigned int& nonceCount,
                                  Data& nonceCountString)
{
   Auth auth;
   Data authQop = qopOption(challenge);
   if(!authQop.empty())
   {
       updateNonceCount(nonceCount, nonceCountString);
   }
   makeChallengeResponseAuth(request, username, password, challenge, cnonce, authQop, nonceCountString, auth);
   return auth;
}

void
Helper::makeChallengeResponseAuth(const SipMessage& request,
                                  const Data& username,
                                  const Data& password,
                                  const Auth& challenge,
                                  const Data& cnonce,
                                  const Data& authQop,
                                  const Data& nonceCountString,
                                  Auth& auth)
{
   auth.scheme() = Symbols::Digest;
   auth.param(p_username) = username;
   resip_assert(challenge.exists(p_realm));
   auth.param(p_realm) = challenge.param(p_realm);
   resip_assert(challenge.exists(p_nonce));
   auth.param(p_nonce) = challenge.param(p_nonce);
   Data digestUri;
   {
      DataStream s(digestUri);
      //s << request.header(h_RequestLine).uri().host(); // wrong 
      s << request.header(h_RequestLine).uri(); // right 
   }
   auth.param(p_uri) = digestUri;

   if (!authQop.empty())
   {
      auth.param(p_response) = Helper::makeResponseMD5(username, 
                                                       password,
                                                       challenge.param(p_realm), 
                                                       getMethodName(request.header(h_RequestLine).getMethod()), 
                                                       digestUri, 
                                                       challenge.param(p_nonce),
                                                       authQop,
                                                       cnonce,
                                                       nonceCountString,
                                                       request.getContents());
      auth.param(p_cnonce) = cnonce;
      auth.param(p_nc) = nonceCountString;
      auth.param(p_qop) = authQop;
   }
   else
   {
      resip_assert(challenge.exists(p_realm));
      auth.param(p_response) = Helper::makeResponseMD5(username, 
                                                       password,
                                                       challenge.param(p_realm), 
                                                       getMethodName(request.header(h_RequestLine).getMethod()),
                                                       digestUri, 
                                                       challenge.param(p_nonce));
   }
   
   if (challenge.exists(p_algorithm))
   {
      auth.param(p_algorithm) = challenge.param(p_algorithm);
   }
   else
   {
      auth.param(p_algorithm) = "MD5";
   }

   if (challenge.exists(p_opaque) && challenge.param(p_opaque).size() > 0)
   {
      auth.param(p_opaque) = challenge.param(p_opaque);
   }
}

// priority-order list of preferred qop tokens
static Data preferredTokens[] = 
{
   "auth-int",
   "auth"
};
static size_t pTokenSize=sizeof(preferredTokens)/sizeof(*preferredTokens);
Data
Helper::qopOption(const Auth& challenge)
{
   bool found = false;
   size_t index = pTokenSize;
   if (challenge.exists(p_qopOptions) && !challenge.param(p_qopOptions).empty())
   {
      ParseBuffer pb(challenge.param(p_qopOptions).data(), challenge.param(p_qopOptions).size());
      do
      {
         const char* anchor = pb.skipWhitespace();
         pb.skipToChar(Symbols::COMMA[0]);
         Data q;
         pb.data(q, anchor);
         if (!pb.eof())
            pb.skipChar();
         for (size_t i=0; i < pTokenSize; i++) 
         {
            if (q == preferredTokens[i]) 
            {
               // found a preferred token; is it higher priority?
               if (i < index) 
               {
                  found = true;
                  index = i;
               }
            }
         }
      }
      while(!pb.eof());
   }

   if (found)
      return preferredTokens[index];

   return Data::Empty;
   
}
   

Auth 
Helper::makeChallengeResponseAuthWithA1(const SipMessage& request,
                                        const Data& username,
                                        const Data& passwordHashA1,
                                        const Auth& challenge,
                                        const Data& cnonce,
                                        unsigned int& nonceCount,
                                        Data& nonceCountString)
{
   Auth auth;
   Data authQop = qopOption(challenge);
   if(!authQop.empty())
   {
       updateNonceCount(nonceCount, nonceCountString);
   }
   makeChallengeResponseAuthWithA1(request, username, passwordHashA1, challenge, cnonce, authQop, nonceCountString, auth);
   return auth;
}

void
Helper::makeChallengeResponseAuthWithA1(const SipMessage& request,
                                        const Data& username,
                                        const Data& passwordHashA1,
                                        const Auth& challenge,
                                        const Data& cnonce,
                                        const Data& authQop,
                                        const Data& nonceCountString,
                                        Auth& auth)
{
   auth.scheme() = Symbols::Digest;
   auth.param(p_username) = username;
   resip_assert(challenge.exists(p_realm));
   auth.param(p_realm) = challenge.param(p_realm);
   resip_assert(challenge.exists(p_nonce));
   auth.param(p_nonce) = challenge.param(p_nonce);
   Data digestUri;
   {
      DataStream s(digestUri);
      //s << request.const_header(h_RequestLine).uri().host(); // wrong 
      s << request.const_header(h_RequestLine).uri(); // right 
   }
   auth.param(p_uri) = digestUri;

   if (!authQop.empty())
   {
      auth.param(p_response) = Helper::makeResponseMD5WithA1(passwordHashA1,
                                                             getMethodName(request.header(h_RequestLine).getMethod()), 
                                                             digestUri, 
                                                             challenge.param(p_nonce),
                                                             authQop,
                                                             cnonce,
                                                             nonceCountString,
                                                             request.getContents());
      auth.param(p_cnonce) = cnonce;
      auth.param(p_nc) = nonceCountString;
      auth.param(p_qop) = authQop;
   }
   else
   {
      resip_assert(challenge.exists(p_realm));
      auth.param(p_response) = Helper::makeResponseMD5WithA1(passwordHashA1,
                                                             getMethodName(request.header(h_RequestLine).getMethod()),
                                                             digestUri, 
                                                             challenge.param(p_nonce));
   }
   
   if (challenge.exists(p_algorithm))
   {
      auth.param(p_algorithm) = challenge.param(p_algorithm);
   }
   else
   {
      auth.param(p_algorithm) = "MD5";
   }

   if (challenge.exists(p_opaque) && challenge.param(p_opaque).size() > 0)
   {
      auth.param(p_opaque) = challenge.param(p_opaque);
   }
}
   
//.dcm. all the auth stuff should be yanked out of helper and
//architected for algorithm independance
bool 
Helper::algorithmAndQopSupported(const Auth& challenge)
{
   if ( !(challenge.exists(p_nonce) && challenge.exists(p_realm)))
   {
      return false;
   }
   return ((!challenge.exists(p_algorithm) 
            || isEqualNoCase(challenge.param(p_algorithm), "MD5"))
           && (!challenge.exists(p_qop) 
               || isEqualNoCase(challenge.param(p_qop), Symbols::auth)
               || isEqualNoCase(challenge.param(p_qop), Symbols::authInt)));
}

SipMessage& 
Helper::addAuthorization(SipMessage& request,
                         const SipMessage& challenge,
                         const Data& username,
                         const Data& password,
                         const Data& cnonce,
                         unsigned int& nonceCount)
{
   Data nonceCountString = Data::Empty;
   
   resip_assert(challenge.isResponse());
   resip_assert(challenge.header(h_StatusLine).responseCode() == 401 ||
          challenge.header(h_StatusLine).responseCode() == 407);

   if (challenge.exists(h_ProxyAuthenticates))
   {
      const ParserContainer<Auth>& auths = challenge.header(h_ProxyAuthenticates);
      for (ParserContainer<Auth>::const_iterator i = auths.begin();
           i != auths.end(); i++)
      {
         request.header(h_ProxyAuthorizations).push_back(makeChallengeResponseAuth(request, username, password, *i, 
                                                                                    cnonce, nonceCount, nonceCountString));
      }
   }
   if (challenge.exists(h_WWWAuthenticates))
   {
      const ParserContainer<Auth>& auths = challenge.header(h_WWWAuthenticates);
      for (ParserContainer<Auth>::const_iterator i = auths.begin();
           i != auths.end(); i++)
      {
         request.header(h_Authorizations).push_back(makeChallengeResponseAuth(request, username, password, *i,
                                                                               cnonce, nonceCount, nonceCountString));
      }
   }
   return request;
}
      
Uri
Helper::makeUri(const Data& aor, const Data& scheme)
{
   resip_assert(!aor.prefix("sip:"));
   resip_assert(!aor.prefix("sips:"));
   
   Data tmp(aor.size() + scheme.size() + 1, Data::Preallocate);
   tmp += scheme;
   tmp += Symbols::COLON;
   tmp += aor;
   Uri uri(tmp);
   return uri;
}

void
Helper::processStrictRoute(SipMessage& request)
{
   if (request.exists(h_Routes) && 
       !request.const_header(h_Routes).empty() &&
       !request.const_header(h_Routes).front().uri().exists(p_lr))
   {
      // The next hop is a strict router.  Move the next hop into the
      // Request-URI and move the ultimate destination to the end of the
      // route list.  Force the message target to be the next hop router.
      request.header(h_Routes).push_back(NameAddr(request.const_header(h_RequestLine).uri()));
      request.header(h_RequestLine).uri() = request.const_header(h_Routes).front().uri();
      request.header(h_Routes).pop_front(); // !jf!
      resip_assert(!request.hasForceTarget());
      request.setForceTarget(request.const_header(h_RequestLine).uri());
   }
}

void
Helper::massageRoute(const SipMessage& request, NameAddr& rt)
{
   resip_assert(request.isRequest());
   // .bwc. Let's not record-route with a tel uri or something, shall we?
   // If the topmost route header is malformed, we can get along without.
   if (!request.empty(h_Routes) && 
       request.header(h_Routes).front().isWellFormed() &&
       (request.header(h_Routes).front().uri().scheme() == "sip" ||
        request.header(h_Routes).front().uri().scheme() == "sips" ))
   {
      rt.uri().scheme() = request.header(h_Routes).front().uri().scheme();
   }
   else if(request.header(h_RequestLine).uri().scheme() == "sip" ||
           request.header(h_RequestLine).uri().scheme() == "sips")
   {
      rt.uri().scheme() = request.header(h_RequestLine).uri().scheme();
   }
   
   rt.uri().param(p_lr);
}

int
Helper::getPortForReply(SipMessage& request)
{
   resip_assert(request.isRequest());
   int port = 0;
   TransportType transportType = toTransportType(
      request.const_header(h_Vias).front().transport());
   if(isReliable(transportType))
   {
      // 18.2.2 - bullet 1 and 2 
      port = request.getSource().getPort();
      if(port == 0) // .slg. not sure if it makes sense for sourcePort to be 0
      {
         port = request.const_header(h_Vias).front().sentPort();
      }
   }
   else   // unreliable transport 18.2.2 bullets 3 and 4
   {
      if (request.const_header(h_Vias).front().exists(p_rport))
      {
         port = request.getSource().getPort();
      }
      else
      {
         port = request.const_header(h_Vias).front().sentPort();
      }
   }

   // If we haven't got a valid port yet, then use the default
   if (port <= 0 || port > 65535) 
   {
      if(transportType == TLS ||
         transportType == DTLS)
      {
         port = Symbols::DefaultSipsPort;
      }
      else
      {
         port = Symbols::DefaultSipPort;
      }
   }
   return port;
}

Uri 
Helper::fromAor(const Data& aor, const Data& scheme)
{
   return makeUri(aor, scheme);
}

bool
Helper::validateMessage(const SipMessage& message,resip::Data* reason)
{
   if (message.empty(h_To) || 
       message.empty(h_From) || 
       message.empty(h_CSeq) || 
       message.empty(h_CallId) || 
       message.empty(h_Vias) ||
       message.empty(h_Vias))
   {
      InfoLog(<< "Missing mandatory header fields (To, From, CSeq, Call-Id or Via)");
      DebugLog(<< message);
      if(reason) *reason="Missing mandatory header field";
      return false;
   }
   else
   {
      if(!message.header(h_CSeq).isWellFormed())
      {
         InfoLog(<<"Malformed CSeq header");
         if(reason) *reason="Malformed CSeq header";
         return false;
      }
      
      if(!message.header(h_Vias).front().isWellFormed())
      {
         InfoLog(<<"Malformed topmost Via header");
         if(reason) *reason="Malformed topmost Via header";
         return false;
      }
      
      if (message.isRequest())
      {
         if(!message.header(h_RequestLine).isWellFormed())
         {
            InfoLog(<< "Illegal request line");
            if(reason) *reason="Malformed Request Line";
            return false;            
         }
         
         if(message.header(h_RequestLine).method()!=message.header(h_CSeq).method())
         {
            InfoLog(<< "Method mismatch btw Request Line and CSeq");
            if(reason) *reason="Method mismatch btw Request Line and CSeq";
            return false;
         }
      }
      else
      {
         if(!message.header(h_StatusLine).isWellFormed())
         {
            InfoLog(<< "Malformed status line");
            if(reason) *reason="Malformed status line";
            return false;            
         }
      }
      
      return true;
   }
}

#if defined(USE_SSL)
#include <openssl/blowfish.h>

static const Data sep("[]");
static const Data pad("\0\0\0\0\0\0\0", 7);
static const Data GRUU("_GRUU");
static const int saltBytes(16);

Data
Helper::gruuUserPart(const Data& instanceId,
                     const Data& aor,
                     const Data& key)
{
   unsigned char ivec[8];      

   ivec[0] = '\x6E';
   ivec[1] = '\xE7';
   ivec[2] = '\xB0';
   ivec[3] = '\x4A';
   ivec[4] = '\x45';
   ivec[5] = '\x93';
   ivec[6] = '\x7D';
   ivec[7] = '\x51';

   BF_KEY fish;
   BF_set_key(&fish, (int)key.size(), (const unsigned char*)key.data());

   const Data salt(resip::Random::getRandomHex(saltBytes));

   const Data token(salt + instanceId + sep + aor + '\0' +
                    pad.substr(0, (8 - ((salt.size() + 
                                         instanceId.size() + 
                                         sep.size() + 1 
                                         + aor.size() ) % 8))
                               % 8));
   auto_ptr <unsigned char> out(new unsigned char[token.size()]);
   BF_cbc_encrypt((const unsigned char*)token.data(),
                  out.get(),
                  (long)token.size(),
                  &fish,
                  ivec, 
                  BF_ENCRYPT);

   return GRUU + Data(out.get(),token.size()).base64encode(true/*safe URL*/);
}

std::pair<Data,Data> 
Helper::fromGruuUserPart(const Data& gruuUserPart,
                         const Data& key)
{
   unsigned char ivec[8];      

   ivec[0] = '\x6E';
   ivec[1] = '\xE7';
   ivec[2] = '\xB0';
   ivec[3] = '\x4A';
   ivec[4] = '\x45';
   ivec[5] = '\x93';
   ivec[6] = '\x7D';
   ivec[7] = '\x51';

   static const std::pair<Data, Data> empty;

   if (gruuUserPart.size() < GRUU.size())
   {
      return empty;
   }

   const Data gruu = gruuUserPart.substr(GRUU.size());

   BF_KEY fish;
   BF_set_key(&fish, (int)key.size(), (const unsigned char*)key.data());

   const Data decoded = gruu.base64decode();

   auto_ptr <unsigned char> out(new unsigned char[gruuUserPart.size()+1]);
   BF_cbc_encrypt((const unsigned char*)decoded.data(),
                  out.get(),
                  (long)decoded.size(),
                  &fish,
                  ivec, 
                  BF_DECRYPT);
   const Data pair(out.get(), decoded.size());

   Data::size_type pos = pair.find(sep);
   if (pos == Data::npos)
   {
      return empty;
   }

   return std::make_pair(pair.substr(2*saltBytes, pos), // strip out the salt
                         pair.substr(pos+sep.size()));
}
#endif
Helper::ContentsSecAttrs::ContentsSecAttrs()
   : mContents(0),
     mAttributes(0)
{}

Helper::ContentsSecAttrs::ContentsSecAttrs(std::auto_ptr<Contents> contents,
                                           std::auto_ptr<SecurityAttributes> attributes)
   : mContents(contents),
     mAttributes(attributes)
{}

// !!bwc!! Yikes! Destructive copy c'tor! Are we _sure_ this is the 
// intended behavior?
Helper::ContentsSecAttrs::ContentsSecAttrs(const ContentsSecAttrs& rhs)
   : mContents(rhs.mContents),
     mAttributes(rhs.mAttributes)
{}

Helper::ContentsSecAttrs& 
Helper::ContentsSecAttrs::operator=(const ContentsSecAttrs& rhs)
{
   if (&rhs != this)
   {
      // !!bwc!! Yikes! Destructive assignment operator! Are we _sure_ this is 
      // the intended behavior?
      mContents = rhs.mContents;
      mAttributes = rhs.mAttributes;
   }
   return *this;
}


Contents*
extractFromPkcs7Recurse(Contents* tree,
                        const Data& signerAor,
                        const Data& receiverAor,
                        SecurityAttributes* attributes,
                        Security& security)
{
   Pkcs7Contents* pk;
   if ((pk = dynamic_cast<Pkcs7Contents*>(tree)))
   {
      InfoLog( << "GREG1: " << *pk );
#if defined(USE_SSL)
      Contents* contents = security.decrypt(receiverAor, pk);
      if (contents)
      {
         attributes->setEncrypted();
      }
      return contents;
#else
      return 0;
#endif
   }
   MultipartSignedContents* mps;
   if ((mps = dynamic_cast<MultipartSignedContents*>(tree)))
   {
      InfoLog( << "GREG2: " << *mps );
#if defined(USE_SSL)
      Data signer;
      SignatureStatus sigStatus;
      Contents* b = extractFromPkcs7Recurse(security.checkSignature(mps, 
                                                                    &signer,
                                                                    &sigStatus),
                                            signerAor,
                                            receiverAor, attributes, security);
      attributes->setSigner(signer);
      attributes->setSignatureStatus(sigStatus);
      return b->clone();
#else
      return mps->parts().front()->clone();
#endif      
   }
   MultipartAlternativeContents* alt;
   if ((alt = dynamic_cast<MultipartAlternativeContents*>(tree)))
   {
      InfoLog( << "GREG3: " << *alt );
      for (MultipartAlternativeContents::Parts::reverse_iterator i = alt->parts().rbegin();
           i != alt->parts().rend(); ++i)
      {
         Contents* b = extractFromPkcs7Recurse(*i, signerAor, receiverAor, attributes, security);
         if (b)
         {
            return b;
         }
      }
   }

   MultipartMixedContents* mult;
   if ((mult = dynamic_cast<MultipartMixedContents*>(tree)))
   {
      InfoLog( << "GREG4: " << *mult );
      for (MultipartMixedContents::Parts::iterator i = mult->parts().begin();
           i != mult->parts().end(); ++i)
      {
         Contents* b = extractFromPkcs7Recurse(*i, signerAor, receiverAor,
                                               attributes, security);
         if (b)
         {
            return b;
         }
      };

      return 0;
   }

   return tree->clone();
}

Helper::ContentsSecAttrs
Helper::extractFromPkcs7(const SipMessage& message, 
                         Security& security)
{
   SecurityAttributes* attr = new SecurityAttributes;
   // .dlb. currently flattening SecurityAttributes?
   //attr->setIdentity(message.getIdentity());
   attr->setIdentity(message.header(h_From).uri().getAor());
   Contents *b = message.getContents();
   if (b) 
   {
      Data fromAor(message.header(h_From).uri().getAor());
      Data toAor(message.header(h_To).uri().getAor());
      if (message.isRequest())
      {
         b = extractFromPkcs7Recurse(b, fromAor, toAor, attr, security);
      }
      else // its a response
      {
         b = extractFromPkcs7Recurse(b, toAor, fromAor, attr, security);
      }
   }
   std::auto_ptr<Contents> c(b);
   std::auto_ptr<SecurityAttributes> a(attr);
   return ContentsSecAttrs(c, a);
}

Helper::FailureMessageEffect 
Helper::determineFailureMessageEffect(const SipMessage& response,
    const std::set<int>* additionalTransactionTerminatingResponses)
{
   resip_assert(response.isResponse());
   int code = response.header(h_StatusLine).statusCode();
   resip_assert(code >= 400);
   
   if (additionalTransactionTerminatingResponses &&
       (additionalTransactionTerminatingResponses->end() != additionalTransactionTerminatingResponses->find(code)))
   {
      return Helper::TransactionTermination;
   }

   switch(code)
   {
      case 404:
      case 410:
      case 416:
      case 480:  // but maybe not, still not quite decided:
      case 481:
      case 482: // but maybe not, still not quite decided:
      case 484:
      case 485:
      case 502:
      case 604:
         return DialogTermination;
      case 403:
      case 489: //only for only subscription
      case 408:  //again, maybe not. This seems best.
         return UsageTermination;      
      case 400:
      case 401:
      case 402:
      case 405:  //doesn't agree w/  -00 of dialogusage
      case 406:
      case 412:
      case 413:
      case 414:
      case 415:
      case 420:
      case 421:
      case 423:

      case 429: // but if this the refer creating the Subscription, no sub will be created.
      case 486:
      case 487:
      case 488:
      case 491: 
      case 493:
      case 494:
      case 500:
      case 505:
      case 513:
      case 603:
      case 606:
         return TransactionTermination;
      case 483: // who knows, gravefully terminate or just destroy dialog
      case 501:
         return ApplicationDependant;
      default:
         if (code < 600)
         {
            if (response.exists(h_RetryAfter))

            {
               return RetryAfter;
            }
            else
            {
               return OptionalRetryAfter;
            }
         }
         else
         {
            if (response.exists(h_RetryAfter))
            {
               return RetryAfter;
            }
            else
            {
               return ApplicationDependant;
            }
         }
   }
}

SdpContents* getSdpRecurse(Contents* tree)
{
   if (dynamic_cast<SdpContents*>(tree))
   {
      return static_cast<SdpContents*>(tree);
   }

   MultipartSignedContents* mps;
   if ((mps = dynamic_cast<MultipartSignedContents*>(tree)))
   {
      try
      {
         MultipartSignedContents::Parts::const_iterator it = mps->parts().begin();
         Contents* contents = getSdpRecurse(*it);
         return static_cast<SdpContents*>(contents);
      }
      catch (ParseException& e)
      {
         ErrLog(<< e.name() << endl << e.getMessage());       
      }
      catch (BaseException& e)
      {
         ErrLog(<< e.name() << endl << e.getMessage());
      }

      return 0;
   }

   MultipartAlternativeContents* alt;
   if ((alt = dynamic_cast<MultipartAlternativeContents*>(tree)))
   {
      try
      {
         for (MultipartAlternativeContents::Parts::reverse_iterator i = alt->parts().rbegin();
              i != alt->parts().rend(); ++i)
         {
            Contents* contents = getSdpRecurse(*i);
            if (contents)
            {
               return static_cast<SdpContents*>(contents);
            }
         }
      }
      catch (ParseException& e)
      {
         ErrLog(<< e.name() << endl << e.getMessage());
      }
      catch (BaseException& e)
      {
         ErrLog(<< e.name() << endl << e.getMessage());
      }

      return 0;
   }

   MultipartMixedContents* mult;
   if ((mult = dynamic_cast<MultipartMixedContents*>(tree)))
   {

      try
      {
         for (MultipartMixedContents::Parts::iterator i = mult->parts().begin();
              i != mult->parts().end(); ++i)
         {
            Contents* contents = getSdpRecurse(*i);
            if (contents)
            {
               return static_cast<SdpContents*>(contents);
            }
         }
      }
      catch (ParseException& e)
      {
         ErrLog(<< e.name() << endl << e.getMessage());
      }
      catch (BaseException& e)
      {
         ErrLog(<< e.name() << endl << e.getMessage());
      }

      return 0;
   }

   return 0;
}

static std::auto_ptr<SdpContents> emptysdp;
auto_ptr<SdpContents> Helper::getSdp(Contents* tree)
{
   if (tree) 
   {
      SdpContents* sdp = getSdpRecurse(tree);

      if (sdp)
      {
         DebugLog(<< "Got sdp" << endl);
         return auto_ptr<SdpContents>(static_cast<SdpContents*>(sdp->clone()));
      }
   }

   //DebugLog(<< "No sdp" << endl);
   return emptysdp;
}

bool 
Helper::isClientBehindNAT(const SipMessage& request, bool privateToPublicOnly)
{
   resip_assert(request.isRequest());
   resip_assert(!request.header(h_Vias).empty());

   // If received parameter is on top Via, then the source of the message doesn't match
   // the address provided in the via.  Assume this is because the sender is behind a NAT.
   // The assumption here is that this SipStack instance is the first hop in a public SIP server
   // architecture, and that clients are directly connected to this instance.
   if(request.header(h_Vias).front().exists(p_received))
   {
      if(privateToPublicOnly)
      {
         // Ensure the via host is an IP address (note: web-rtc uses hostnames here instead)
         if(DnsUtil::isIpV4Address(request.header(h_Vias).front().sentHost()) 
#ifdef USE_IPV6
             || DnsUtil::isIpV6Address(request.header(h_Vias).front().sentHost())
#endif
             )
         {
            if(Tuple(request.header(h_Vias).front().sentHost(), 0, UNKNOWN_TRANSPORT).isPrivateAddress() &&
                !Tuple(request.header(h_Vias).front().param(p_received), 0, UNKNOWN_TRANSPORT).isPrivateAddress())
            {
                return true;
            }
            else
            {
                return false;
            }
         }
         else
         {
             // In this case the via host is likely a hostname (possible with web-rtc) and we will assume the
             // client is behind a NAT, even though we don't know for sure
             return !Tuple(request.header(h_Vias).front().param(p_received), 0, UNKNOWN_TRANSPORT).isPrivateAddress();
         }
      }
      return true;
   }
   return false;
}

Tuple
Helper::getClientPublicAddress(const SipMessage& request)
{
   resip_assert(request.isRequest());
   resip_assert(!request.header(h_Vias).empty());

   // Iterate through Via's starting at the bottom (closest to the client).  Return the first
   // public address found from received parameter if present, or Via host.
   Vias::const_iterator it = request.header(h_Vias).end();
   while(true)
   {
      it--;
      if(it->exists(p_received))
      {
         // Check IP from received parameter
         Tuple address(it->param(p_received), 0, UNKNOWN_TRANSPORT);
         if(!address.isPrivateAddress())
         {
            address.setPort(it->exists(p_rport) ? it->param(p_rport).port() : it->sentPort());
            address.setType(Tuple::toTransport(it->transport()));
            return address;
         }
      }

      // Check IP from Via sentHost
      if(DnsUtil::isIpV4Address(it->sentHost())  // Ensure the via host is an IP address (note: web-rtc uses hostnames here instead)
#ifdef USE_IPV6
          || DnsUtil::isIpV6Address(it->sentHost())
#endif
          )
      {
         Tuple address(it->sentHost(), 0, UNKNOWN_TRANSPORT);
         if(!address.isPrivateAddress())
         {
            address.setPort(it->exists(p_rport) ? it->param(p_rport).port() : it->sentPort());
            address.setType(Tuple::toTransport(it->transport()));
            return address;
         }
      }

      if(it == request.header(h_Vias).begin()) break;
   }
   return Tuple();
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
