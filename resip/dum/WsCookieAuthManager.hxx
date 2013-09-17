#if !defined(RESIP_WSCOOKIEAUTHMANAGER_HXX)
#define RESIP_WSCOOKIEAUTHMANAGER_HXX

#include <map>
#include <set>

#include "resip/stack/SipMessage.hxx"
#include "DumFeature.hxx"
#include "resip/stack/Cookie.hxx"

namespace resip
{
class DialogUsageManager;

typedef std::set<Data> PermittedFromAddresses;
typedef std::map<Data, PermittedFromAddresses> CommonNameMappings;

class WsCookieAuthManager : public DumFeature
{
   public:
      enum Result
      {
         Authorized,
         Skipped,
         Rejected
      };

      WsCookieAuthManager(DialogUsageManager& dum, TargetCommand::Target& target);
      virtual ~WsCookieAuthManager();

      virtual ProcessingResult process(Message* msg);

   protected:

      // can return Authorized, Rejected, Skipped
      virtual Result handle(SipMessage* sipMsg);

      /// should return true if the passed in user is authorized for the provided uri
      bool authorizedForThisIdentity(const CookieList &cookieList, resip::Uri &fromUri, resip::Uri &toUri);

      /// should return true if the request must be challenged
      /// The default is to challenge all requests - override this class to change this beviour
      virtual bool requiresAuthorization(const SipMessage& msg);
};


}

#endif
