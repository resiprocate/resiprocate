#ifndef __REGISTRATION_MANAGER_HXX__
#define __REGISTRATION_MANAGER_HXX__

#include <map>
#include "ConversationProfile.hxx"

namespace resip
{
   class DialogUsageManager;
   class Uri;
}

namespace recon
{
   class UserAgentRegistration;
   class ConversationManager;
   class BasicUserAgent;

   /**
    */
   class RegistrationManager
   {
   public:
      RegistrationManager();
      virtual ~RegistrationManager();

      void unregisterAll();

      /**
       * If true, recon will manage a registration session for the user agent;
       * i.e. it will send a REGISTER request when the conversation profile is added.
       * Applications that manage their own registration sessions for the user agent
       * should implement this and set it to 'false'.
       */
      virtual bool manageRegistrations() { return true; }

      virtual void makeRegistration(ConversationProfileHandle profile);
      virtual void destroyRegistration(ConversationProfileHandle profile);

   private:
      friend class UserAgentRegistration;
      friend class BasicUserAgent;

      void setConversationManager(ConversationManager* convManager);
      void registerRegistration(UserAgentRegistration *);
      void unregisterRegistration(UserAgentRegistration *);

      resip::DialogUsageManager* mDum;
      ConversationManager* mConversationManager;

      typedef std::map<ConversationProfileHandle, UserAgentRegistration*> RegistrationMap;
      RegistrationMap mRegistrations;
   };
}

#endif // __REGISTRATION_MANAGER_HXX__