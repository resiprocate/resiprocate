#include <resip/stack/Uri.hxx>
#include <resip/stack/Headers.hxx>

#include "RegistrationManager.hxx"
#include "UserAgentRegistration.hxx"

using namespace recon;

RegistrationManager::RegistrationManager()
{

}

RegistrationManager::~RegistrationManager()
{

}

void
RegistrationManager::registerRegistration(UserAgentRegistration *registration)
{
   mRegistrations[registration->getConversationProfileHandle()] = registration;
}

void 
RegistrationManager::unregisterRegistration(UserAgentRegistration *registration)
{
   mRegistrations.erase(registration->getConversationProfileHandle());
}

void
RegistrationManager::unregisterAll()
{
   // Create copy for safety, since ending Subscriptions can immediately
   // remove themselves from map
   RegistrationMap tempRegs = mRegistrations;
   RegistrationMap::iterator j;
   for(j = tempRegs.begin(); j != tempRegs.end(); j++)
   {
      j->second->end();
   }
}

void recon::RegistrationManager::removeRegistration( ConversationProfileHandle cph )
{
   RegistrationMap::iterator it = mRegistrations.find(cph);
   if(it != mRegistrations.end())
   {
      it->second->end();
   }
}

ConversationProfileHandle RegistrationManager::findRegistration( const resip::Uri& requestURI )
{
   // Check if request uri matches registration contact
   RegistrationMap::iterator regIt;
   for(regIt = mRegistrations.begin(); regIt != mRegistrations.end(); ++regIt)
   {
      const resip::NameAddrs& contacts = regIt->second->getContactAddresses();
      resip::NameAddrs::const_iterator naIt;
      for(naIt = contacts.begin(); naIt != contacts.end(); ++naIt)
      {
         //InfoLog( << "getIncomingConversationProfile: comparing requestURI=" << requestURI << " to contactUri=" << (*naIt).uri());
         if((*naIt).uri() == requestURI)
            return regIt->first;
      }
   }

   return 0;
}