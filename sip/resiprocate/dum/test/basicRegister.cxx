#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/RegistrationHandler.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/os/Log.hxx"

using namespace resip;

class Client : public ClientRegistrationHandler
{
   public:
      Client() : done(false) {}

      virtual void onSuccess(ClientRegistration::Handle h, const SipMessage& response)
      {
         cerr << "Client::Success: " << response << endl;
         sleep(5);
         h->removeAll();
         done = true;
      }

      virtual void onFailure(ClientRegistration::Handle, const SipMessage& response)
      {
         cerr << "Client::Failure: " << response << endl;
      }
      bool done;
};

/*
class RegistrationServer : public ServerRegistrationHandler
{
   public:
      RegistrationServer() : done(false) {}
      virtual void onRefresh(ServerRegistration::Handle, const SipMessage& reg)=0;
      
      /// called when one of the contacts is removed
      virtual void onRemoveOne(ServerRegistration::Handle, const SipMessage& reg)=0;
      
      /// Called when all the contacts are removed 
      virtual void onRemoveAll(ServerRegistration::Handle, const SipMessage& reg)=0;
      
      /// Called when a new contact is added. This is after authentication has
      /// all sucseeded
      virtual void onAdd(ServerRegistration::Handle, const SipMessage& reg)=0;

      /// Called when an registration expires 
      virtual void onExpired(ServerRegistration::Handle, const NameAddr& contact)=0;

   private:
      bool done;
};
*/

int 
main (int argc, char** argv)
{
    int level=(int)Log::DEBUG;
    if (argc >1 ) level = atoi(argv[1]);

    Log::initialize(Log::COUT, (resip::Log::Level)level, argv[0]);

   SipStack clientStack;
   clientStack.addTransport(UDP, 15060);
   NameAddr aor("sip:502@jasomi.com");

   Client client;
   Profile* p = new Profile;
   DialogUsageManager clientDum(clientStack);
   clientDum.setProfile(p);
   clientDum.setClientRegistrationHandler(&client);
   clientDum.getProfile()->setDefaultRegistrationTime(70);
   clientDum.getProfile()->setDefaultAor(aor);

#if 0
   p->addDigestCredential( "sip.jasomi.com", "502", "resiprocate" );
#endif

   SipMessage & regMessage = clientDum.makeRegistration(aor);

   cerr << regMessage << "Generated register: " << endl << regMessage << endl;
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
