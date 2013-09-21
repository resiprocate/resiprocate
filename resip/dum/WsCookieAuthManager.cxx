#include <cassert>

#include "resip/dum/DumFeature.hxx"
#include "resip/dum/DumFeatureChain.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/TargetCommand.hxx"
#include "resip/dum/WsCookieAuthManager.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

WsCookieAuthManager::WsCookieAuthManager(DialogUsageManager& dum, TargetCommand::Target& target) :
   DumFeature(dum, target)
{
}

WsCookieAuthManager::~WsCookieAuthManager()
{
   InfoLog(<< "~WsCookieAuthManager");
}

// !bwc! We absolutely, positively, MUST NOT throw here. This is because in
// DialogUsageManager::process(), we do not know if a DumFeature has taken
// ownership of msg until we get a return. If we throw, the ownership of msg
// is unknown. This is unacceptable.
DumFeature::ProcessingResult
WsCookieAuthManager::process(Message* msg)
{
   SipMessage* sipMessage = dynamic_cast<SipMessage*>(msg);

   if (sipMessage)
   {
      //!dcm! -- unecessary happens in handle
      switch ( handle(sipMessage) )
      {
         case WsCookieAuthManager::Rejected:
            InfoLog(<< "WsCookieAuth rejected request " << sipMessage->brief());
            return DumFeature::ChainDoneAndEventDone;
         default:   // includes Authorized, Skipped
            return DumFeature::FeatureDone;
      }
   }

   // Catch-all (handles something that was not a SipMessage)
   return FeatureDone;
}

bool
WsCookieAuthManager::authorizedForThisIdentity(
   const CookieList &cookieList,
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

// return true if request has been consumed
WsCookieAuthManager::Result
WsCookieAuthManager::handle(SipMessage* sipMessage)
{
   //InfoLog( << "trying to do auth" );
   if (!sipMessage->isRequest() ||
       sipMessage->header(h_RequestLine).method() == ACK ||
       sipMessage->header(h_RequestLine).method() == CANCEL)
   {
      // Do not inspect ACKs or CANCELs
      return Skipped;
   }

   if(!sipMessage->header(h_From).isWellFormed() ||
      sipMessage->header(h_From).isAllContacts() )
   {
      InfoLog(<<"Malformed From header: cannot verify against cookie. Rejecting.");
      SharedPtr<SipMessage> response(new SipMessage);
      Helper::makeResponse(*response, *sipMessage, 400, "Malformed From header");
      mDum.send(response);
      return Rejected;
   }

   const CookieList &cookieList = sipMessage->getWsCookies();
   if (mDum.isMyDomain(sipMessage->header(h_From).uri().host()))
   {
      if (requiresAuthorization(*sipMessage) && !cookieList.empty())
      {
         if(authorizedForThisIdentity(cookieList, sipMessage->header(h_From).uri(), sipMessage->header(h_To).uri()))
            return Authorized;
         SharedPtr<SipMessage> response(new SipMessage);
         Helper::makeResponse(*response, *sipMessage, 403, "Authorization failed");
         mDum.send(response);
         return Rejected;
      }
      else
         return Skipped;
   }
   else
   {
      if(cookieList.empty())
      {
            return Skipped;
      }
      if(authorizedForThisIdentity(cookieList, sipMessage->header(h_From).uri(), sipMessage->header(h_To).uri()))
         return Authorized;
      SharedPtr<SipMessage> response(new SipMessage);
      Helper::makeResponse(*response, *sipMessage, 403, "Authorization failed");
      mDum.send(response);
      return Rejected;
   }

   InfoLog(<< "Skipping some message that we didn't explicitly handle");
   return Skipped;
}

bool
WsCookieAuthManager::requiresAuthorization(const SipMessage& msg)
{
   // everything must be authorized, over-ride this method
   // to implement some other policy
   return true;
}
