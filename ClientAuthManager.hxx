#if !defined(RESIP_CLIENTAUTHMANAGER_HXX)
#define RESIP_CLIENTAUTHMANAGER_HXX

namespace resip
{

class SipMessage;

class ClientAuthManager
{
   public:
      ClientAuthManager(Profile& profile);

      // return true if request is authorized
      bool handle(const SipMessage& response);
      
   private:
      Profile& mProfile;
};
 
}

#endif
