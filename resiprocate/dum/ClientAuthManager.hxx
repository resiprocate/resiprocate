#if !defined(RESIP_CLIENTAUTHMANAGER_HXX)
#define RESIP_CLIENTAUTHMANAGER_HXX

namespace resip
{

class SipMessage;
class Profile;

class ClientAuthManager
{
   public:
      ClientAuthManager(Profile& profile);

      // return true if request is authorized
      bool handle(SipMessage& origRequest, const SipMessage& response);
      
   private:
      Profile& mProfile;
};
 
}

#endif
