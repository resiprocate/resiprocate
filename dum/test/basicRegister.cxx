#include "resiprocate/SipStack.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"
#include "resiprocate/dum/ClientRegistration.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/RegistrationHandler.hxx"
#include "resiprocate/os/Log.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Subsystem.hxx"
#include "resiprocate/dum/AppDialogSet.hxx"
#include "resiprocate/dum/AppDialog.hxx"


#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;

class RegisterAppDialogSet : public AppDialogSet
{
   public:
      RegisterAppDialogSet(DialogUsageManager& dum) : AppDialogSet(dum) 
      {}      
};

   

class Client : public ClientRegistrationHandler
{
   public:
      Client() : done(false) {}

      virtual void onSuccess(ClientRegistrationHandle h, const SipMessage& response)
      {
#ifdef WIN32
         Sleep(2000);
#else
         sleep(5);
#endif
          InfoLog( << "Client::Success: " << endl << response );
          h->removeAll();
          done = true;
      }

      virtual void onFailure(ClientRegistrationHandle, const SipMessage& response)
      {
          InfoLog ( << "Client::Failure: " << response );
          done = true;
      }

      bool done;
};

/*
class RegistrationServer : public ServerRegistrationHandler
{
   public:
      RegistrationServer() : done(false) {}
      virtual void onRefresh(ServerRegistrationHandle, const SipMessage& reg)=0;
      
      /// called when one of the contacts is removed
      virtual void onRemoveOne(ServerRegistrationHandle, const SipMessage& reg)=0;
      
      /// Called when all the contacts are removed 
      virtual void onRemoveAll(ServerRegistrationHandle, const SipMessage& reg)=0;
      
      /// Called when a new contact is added. This is after authentication has
      /// all sucseeded
      virtual void onAdd(ServerRegistrationHandle, const SipMessage& reg)=0;

      /// Called when an registration expires 
      virtual void onExpired(ServerRegistrationHandle, const NameAddr& contact)=0;

   private:
      bool done;
};
*/

int 
main (int argc, char** argv)
{
    int level=(int)Log::Debug;
    if (argc >1 ) level = atoi(argv[1]);

    Log::initialize(Log::Cout, (resip::Log::Level)level, argv[0]);

   SipStack clientStack;
   clientStack.addTransport(UDP, 15060);
   NameAddr aor("sip:502@jasomi.com");

   Client client;
   Profile profile;   
   ClientAuthManager clientAuth(profile);   

   DialogUsageManager clientDum(clientStack);
   clientDum.setProfile(&profile);

   clientDum.setClientRegistrationHandler(&client);
   clientDum.setClientAuthManager(&clientAuth);
   clientDum.getProfile()->setDefaultRegistrationTime(70);
   clientDum.getProfile()->setDefaultAor(aor);

   profile.addDigestCredential( "sip.jasomi.com", "502", "resiprocate" );

   SipMessage & regMessage = clientDum.makeRegistration(new RegisterAppDialogSet(clientDum), aor);

   InfoLog( << regMessage << "Generated register: " << endl << regMessage );
   clientDum.send( regMessage );

   int n = 0;
   while ( !client.done )

   {
     FdSet fdset;

     // Should these be buildFdSet on the DUM?
     clientStack.buildFdSet(fdset);
     int err = fdset.selectMilliSeconds(100);
     assert ( err != -1 );

     clientDum.process(fdset);
     if (!(n++ % 10)) cerr << "|/-\\"[(n/10)%4] << '\b';

   }   

}
