#if !defined(RESIP_SERVERAUTHMANAGER_HXX)
#define RESIP_SERVERAUTHMANAGER_HXX

#include "resiprocate/dum/UserProfile.hxx"

namespace resip
{
class SipMessage;
class Profile;

class ServerAuthManager
{
   public:
      ServerAuthManager();

      // return true if request is authorized
      bool handle(UserProfile& userProfile, const SipMessage& request);
      
   private:
};

 
}

#endif
