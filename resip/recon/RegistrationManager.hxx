#ifndef __REGISTRATION_MANAGER_HXX__
#define __REGISTRATION_MANAGER_HXX__

#include <map>
#include "ConversationProfile.hxx"

namespace resip
{
   class Uri;
}

namespace recon
{
   class UserAgentRegistration;

   /**
    * The RegistrationManager maintains a mapping between conversation
    * profile handles, and actual user agent registration objects. For
    * the time being, we use the conversation profile handle as the key
    * because it is roughly analogous to a registration handle.
    */
   class RegistrationManager
   {
   public:
      RegistrationManager();
      virtual ~RegistrationManager();

      void unregisterAll();
      void removeRegistration(ConversationProfileHandle cph);

      /**
       * This method will attempt to find a registration whose contact
       * address matches the SIP URI which is passed as an argument.
       * If a match could not be found, it will return 0.
       *
       * @param requestURI the URI to use for matching
       * @return a matching ConversationProfileHandle, or 0 if not found.
       */
      ConversationProfileHandle findRegistration( const resip::Uri& requestURI );

   private:
      typedef std::map<ConversationProfileHandle, UserAgentRegistration*> RegistrationMap;
      RegistrationMap mRegistrations;

      friend class UserAgentRegistration;
      void registerRegistration(UserAgentRegistration *);
      void unregisterRegistration(UserAgentRegistration *);
   };
}

#endif // __REGISTRATION_MANAGER_HXX__