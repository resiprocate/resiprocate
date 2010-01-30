#include <resip/stack/Uri.hxx>
#include <resip/stack/Headers.hxx>

#include "RegistrationManager.hxx"
#include "UserAgentRegistration.hxx"
#include "ConversationManagerCmds.hxx"

using namespace recon;

RegistrationManager::RegistrationManager()
: mDum(NULL),
  mConversationManager(NULL)
{
}

RegistrationManager::~RegistrationManager()
{
}

void
RegistrationManager::setConversationManager(ConversationManager* convManager)
{
   mConversationManager = convManager;
   mDum = mConversationManager->mDum;
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
   if (manageRegistrations())
   {
      // Create copy for safety, since ending Registrations can immediately
      // remove themselves from map
      RegistrationMap tempRegs = mRegistrations;
      RegistrationMap::iterator j;
      for(j = tempRegs.begin(); j != tempRegs.end(); j++)
      {
         j->second->end();
      }
   }
}

void
RegistrationManager::makeRegistration(ConversationProfileHandle profile)
{
   class MakeRegistrationCmd : public DumCommandStub
   {
   public:
      MakeRegistrationCmd(RegistrationManager& regMan, ConversationProfileHandle cp)
         : mRegMan(regMan), mProfile(cp) {}

      virtual void executeCommand()
      {
         ConversationManager* convMan = mRegMan.mConversationManager;
         resip::DialogUsageManager* dum = mRegMan.mDum;
         resip::SharedPtr<ConversationProfile> profilePtr = convMan->getConversationProfile(mProfile);
         // Register new profile
         if(mRegMan.manageRegistrations() && (profilePtr->getDefaultRegistrationTime() != 0))
         {
            convMan->mMapDefaultIncomingConvProfile[profilePtr->getDefaultFrom().uri()] = mProfile;

            UserAgentRegistration *registration = new UserAgentRegistration(mRegMan, *dum, mProfile);
            dum->send(dum->makeRegistration(profilePtr->getDefaultFrom(), profilePtr, registration));
         }
      }
   private:
      RegistrationManager& mRegMan;
      ConversationProfileHandle mProfile;
   };
   mDum->post(new MakeRegistrationCmd(*this, profile));
}

void 
recon::RegistrationManager::destroyRegistration(ConversationProfileHandle profile)
{
   class DestroyRegistrationCmd : public DumCommandStub
   {
   public:
      DestroyRegistrationCmd(RegistrationManager& regMan, ConversationProfileHandle cp)
         : mRegMan(regMan), mProfile(cp) {}

      virtual void executeCommand()
      {
         RegistrationManager::RegistrationMap::const_iterator it = mRegMan.mRegistrations.find(mProfile);
         if (it != mRegMan.mRegistrations.end())
         {
            UserAgentRegistration* reg = it->second;
            reg->end();
         }
      }
   private:
      RegistrationManager& mRegMan;
      ConversationProfileHandle mProfile;
   };
   mDum->post(new DestroyRegistrationCmd(*this, profile));
}