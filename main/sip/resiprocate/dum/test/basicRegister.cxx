#include "resiprocate/SipStack.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"
#include "resiprocate/dum/ClientRegistration.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/MasterProfile.hxx"
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
      Client() : done(false), removing(true) {}

      virtual void onSuccess(ClientRegistrationHandle h, const SipMessage& response)
      {
          InfoLog( << "Client::Success: " << endl << response );
          if (removing)
          {
             removing = false;             
#ifdef WIN32
         Sleep(2000);
#else
         sleep(5);
#endif
         ClientRegistration* foo = h.get();          
          h->removeAll();
          }
          else
          {             
             done = true;
          }          
      }

      virtual void onRemoved(ClientRegistrationHandle)
      {
          InfoLog ( << "Client::onRemoved ");
      }

      virtual void onFailure(ClientRegistrationHandle, const SipMessage& response)
      {
          InfoLog ( << "Client::onFailure: " << response );
          done = true;
      }

      bool done;
      bool removing;      
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
   int level=(int)Log::Info;
   if (argc >1 ) level = atoi(argv[1]);

   Log::initialize(Log::Cout, (resip::Log::Level)level, argv[0]);

   Client client;
   MasterProfile profile;   
   auto_ptr<ClientAuthManager> clientAuth(new ClientAuthManager);   

   DialogUsageManager clientDum;
   clientDum.addTransport(UDP, 15060);
   clientDum.setMasterProfile(&profile);

   clientDum.setClientRegistrationHandler(&client);
   clientDum.setClientAuthManager(clientAuth);
   clientDum.getMasterProfile()->setDefaultRegistrationTime(70);

   NameAddr from("sip:101@proxy.internal.xten.net");
   clientDum.getMasterProfile()->setDefaultFrom(from);


//   profile.addDigestCredential( "xten.gloo.net", "derek@xten.gloo.net", "123456" );
//   profile.addDigestCredential( "sphone.vopr.vonage.net", "13015604286", "" );

   SipMessage & regMessage = clientDum.makeRegistration(from, new RegisterAppDialogSet(clientDum));
   NameAddr contact;
//   contact.uri().user() = "13015604286";   
//   regMessage.header(h_Contacts).push_back(contact);   

   InfoLog( << regMessage << "Generated register: " << endl << regMessage );
   clientDum.send( regMessage );

   int n = 0;
   while ( !client.done )

   {
     FdSet fdset;

     // Should these be buildFdSet on the DUM?
     clientDum.buildFdSet(fdset);
     int err = fdset.selectMilliSeconds(100);
     assert ( err != -1 );

     clientDum.process(fdset);
     if (!(n++ % 10)) cerr << "|/-\\"[(n/10)%4] << '\b';

   }   
   return 0;
}
