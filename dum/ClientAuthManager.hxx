#if !defined(RESIP_CLIENTAUTHMANAGER_HXX)
#define RESIP_CLIENTAUTHMANAGER_HXX

#include "resiprocate/dum/DialogSetId.hxx"

#include <set>

namespace resip
{

class Auth;
class SipMessage;
class Profile;

class ClientAuthManager
{
   public:
      ClientAuthManager(Profile& profile);

      // return true if request is authorized
      bool handle(SipMessage& origRequest, const SipMessage& response);
      
   private:
      bool handleAuthHeader(const Auth& auth, SipMessage& origRequest, const SipMessage& response);      
      Profile& mProfile;
      typedef std::set<DialogSetId> AttemptedAuthSet;
      AttemptedAuthSet mAttemptedAuths;      
};
 
}

#endif
