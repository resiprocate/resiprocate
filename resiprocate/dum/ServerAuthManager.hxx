#if !defined(RESIP_SERVERAUTHMANAGER_HXX)
#define RESIP_SERVERAUTHMANAGER_HXX

#include "resiprocate/dum/Identity.hxx"

namespace resip
{
class SipMessage;
class Profile;

class ServerAuthManager
{
   public:
      ServerAuthManager();

      // return true if request is authorized
      bool handle(Identity& identity, const SipMessage& request);
      
   private:
};

 
}

#endif
