#if !defined(RESIP_SERVERAUTHMANAGER_HXX)
#define RESIP_SERVERAUTHMANAGER_HXX

namespace resip
{
class SipMessage;
class Profile;

class ServerAuthManager
{
   public:
      ServerAuthManager(Profile& profile);

      // return true if request is authorized
      bool handle(const SipMessage& request);
      
   private:
      Profile& mProfile;
};

 
}

#endif
