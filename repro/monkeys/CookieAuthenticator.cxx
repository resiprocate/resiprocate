
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "rutil/DnsUtil.hxx"
#include "resip/stack/Message.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Auth.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/stun/Stun.hxx"

#include "repro/monkeys/CookieAuthenticator.hxx"
#include "repro/monkeys/IsTrustedNode.hxx"
#include "repro/RequestContext.hxx"
#include "repro/Proxy.hxx"
#include "repro/UserInfoMessage.hxx"
#include "repro/UserStore.hxx"
#include "repro/Dispatcher.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

#include <time.h>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;


CookieAuthenticator::CookieAuthenticator(const Data& wsCookieAuthSharedSecret,
                                         resip::SipStack* stack) :
   Processor("CookieAuthenticator")
{
}

CookieAuthenticator::~CookieAuthenticator()
{
}

repro::Processor::processor_action_t
CookieAuthenticator::process(repro::RequestContext &rc)
{
   DebugLog(<< "Monkey handling request: " << *this << "; reqcontext = " << rc);

   Message *message = rc.getCurrentEvent();

   SipMessage *sipMessage = dynamic_cast<SipMessage*>(message);
   Proxy &proxy = rc.getProxy();

   if (sipMessage)
   {
      if (sipMessage->method() == ACK ||
            sipMessage->method() == BYE)
      {
         return Continue;
      }

      // if there was no Proxy-Auth header already, and the request is purportedly From
      // one of our domains, send a challenge, unless this is from a trusted node in one
      // of "our" domains (ex: from a gateway).
      //
      // Note that other monkeys can still challenge the request later if needed
      // for other reasons (for example, the StaticRoute monkey)
      if(!sipMessage->header(h_From).isWellFormed() ||
         sipMessage->header(h_From).isAllContacts() )
      {
         InfoLog(<<"Malformed From header: cannot verify against any certificate. Rejecting.");
         rc.sendResponse(*auto_ptr<SipMessage>
                         (Helper::makeResponse(*sipMessage, 400, "Malformed From header")));
         return SkipAllChains;
      }

      const CookieList &cookieList = sipMessage->getWsCookies();
      if (proxy.isMyDomain(sipMessage->header(h_From).uri().host()))
      {
         if (!rc.getKeyValueStore().getBoolValue(IsTrustedNode::mFromTrustedNodeKey))
         {
            // no cookies, skip to next processor
            if(cookieList.empty())
               return Continue;
            if(authorizedForThisIdentity(cookieList, sipMessage->header(h_From).uri(), sipMessage->header(h_To).uri()))
            {
               return Continue;
            }
            rc.sendResponse(*auto_ptr<SipMessage>
                            (Helper::makeResponse(*sipMessage, 403, "Authentication against cookie failed")));
            return SkipAllChains;
         }
         else
            return Continue;
      }
      else
      {
         // no cookies, skip to next processor
         if(cookieList.empty())
         {
               return Continue;
         }
         if(authorizedForThisIdentity(cookieList, sipMessage->header(h_From).uri(), sipMessage->header(h_To).uri()))
         {
            return Continue;
         }
         rc.sendResponse(*auto_ptr<SipMessage>
                            (Helper::makeResponse(*sipMessage, 403, "Authentication against cookie failed")));
         return SkipAllChains;
      }
   }

   return Continue;
}

bool
CookieAuthenticator::authorizedForThisIdentity(const CookieList& cookieList,
                                                resip::Uri &fromUri,
                                                resip::Uri &toUri)
{
   Data wsSessionInfo;
   Data wsSessionExtra;
   Data wsSessionMAC;

   for (CookieList::const_iterator it = cookieList.begin(); it != cookieList.end(); ++it)
   {
      if ((*it).name() == "WSSessionInfo")
      {
         wsSessionInfo = (*it).value();
      }
      else if ((*it).name() == "WSSessionExtra")
      {
         wsSessionExtra = (*it).value();
      }
      else if ((*it).name() == "WSSessionMAC")
      {
         wsSessionMAC = (*it).value();
         ;
      }
   }

   ParseBuffer pb(wsSessionInfo);
   pb.skipToChar(':');
   pb.skipChar(':');
   time_t expires = (time_t) pb.uInt64();

   if (difftime(time(NULL), expires) < 0)
   {
      WarningLog(<< "Cookie has expired");
      return false;
   }

   const char* anchor;
   Data uriString;
   Uri wsFromUri;
   Uri wsDestUri;

   pb.skipToChar(':');
   pb.skipChar(':');
   anchor = pb.position();
   pb.skipToChar(':');
   pb.data(uriString, anchor);
   wsFromUri = Uri("sip:" + uriString);

   pb.skipChar(':');
   anchor = pb.position();
   pb.skipToChar(':');
   pb.data(uriString, anchor);
   wsDestUri = Uri("sip:" + uriString);

   if(wsFromUri.user() == fromUri.user() && wsFromUri.host() == fromUri.host())
   {
      DebugLog(<< "Matched cookie source URI field" << wsFromUri << " against request From header field URI " << fromUri);
      return true;
   }

   if(wsDestUri.user() == toUri.user() && wsDestUri.host() == toUri.host())
   {
      DebugLog(<< "Matched cookie destination URI field" << wsDestUri << " against request To header field URI " << toUri);
      return true;
   }

   // catch-all: access denied
   return false;
}

void
CookieAuthenticator::dump(EncodeStream &os) const
{
   os << "CookieAuthentication monkey" << std::endl;
}
