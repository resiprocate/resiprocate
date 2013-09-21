
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

      if(!sipMessage->header(h_From).isWellFormed() ||
         sipMessage->header(h_From).isAllContacts() )
      {
         InfoLog(<<"Malformed From header: cannot verify against cookie. Rejecting.");
         rc.sendResponse(*auto_ptr<SipMessage>
                         (Helper::makeResponse(*sipMessage, 400, "Malformed From header")));
         return SkipAllChains;
      }

      const CookieList &cookieList = sipMessage->getWsCookies();
      const WsCookieContext &wsCookieContext = sipMessage->getWsCookieContext();
      if (proxy.isMyDomain(sipMessage->header(h_From).uri().host()))
      {
         if(cookieList.empty())
            return Continue;
         if(authorizedForThisIdentity(wsCookieContext, sipMessage->header(h_From).uri(), sipMessage->header(h_To).uri()))
         {
            return Continue;
         }
         rc.sendResponse(*auto_ptr<SipMessage>
                        (Helper::makeResponse(*sipMessage, 403, "Authentication against cookie failed")));
         return SkipAllChains;
      }
      else
      {
         if(cookieList.empty())
            return Continue;
         rc.sendResponse(*auto_ptr<SipMessage>
                            (Helper::makeResponse(*sipMessage, 403, "Authentication against cookie failed")));
         return SkipAllChains;
      }
   }

   return Continue;
}

bool
CookieAuthenticator::authorizedForThisIdentity(const WsCookieContext& wsCookieContext,
                                                resip::Uri &fromUri,
                                                resip::Uri &toUri)
{
   if(difftime(time(NULL), wsCookieContext.getExpiresTime()) < 0)
   {
      WarningLog(<< "Received expired cookie");
      return false;
   }

   return true;

   Uri wsFromUri = wsCookieContext.getWsFromUri();
   Uri wsDestUri = wsCookieContext.getWsDestUri();

   if(isEqualNoCase(wsFromUri.user(), fromUri.user()) && isEqualNoCase(wsFromUri.host(), fromUri.host()))
   {
      DebugLog(<< "Matched cookie source URI field" << wsFromUri << " against request From header field URI " << fromUri);
      if(isEqualNoCase(wsDestUri.user(), toUri.user()) && isEqualNoCase(wsDestUri.host(), toUri.host()))
         {
            DebugLog(<< "Matched cookie destination URI field" << wsDestUri << " against request To header field URI " << toUri);
            return true;
         }
   }

   // catch-all: access denied
   return false;
}

void
CookieAuthenticator::dump(EncodeStream &os) const
{
   os << "CookieAuthentication monkey" << std::endl;
}
