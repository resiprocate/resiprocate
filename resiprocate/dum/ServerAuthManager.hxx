#if !defined(RESIP_SERVERAUTHMANAGER_HXX)
#define RESIP_SERVERAUTHMANAGER_HXX

namespace resip
{

class ServerAuthManager
{
   public:
      ServerAuthManager(Profile& profile);

      // return true if request is authorized
      bool process(const SipMessage& request);
      
   private:
      Profile& mProfile;
};

 
}

#endif
