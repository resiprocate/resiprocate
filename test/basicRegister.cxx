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

    Log::initialize(Log::COUT, Log::DEBUG, argv[0]);

   SipStack clientStack;
   clientStack.addTransport(UDP, 5060);

   Client client;
   Profile* p = new Profile;
   DialogUsageManager clientDum(clientStack);
   clientDum.setProfile(p);

   clientDum.setClientRegistrationHandler(&client);
   
   clientDum.getProfile()->setDefaultRegistrationTime(70);
   SipMessage & regMessage = clientDum.makeRegistration(NameAddr("sip:502@jasomi.com"));
   cerr << regMessage << "Generated register: " << endl << regMessage << endl;

#if 0
   while ( (!client.done)
   {
     FdSet fdset;
     // Should these be buildFdSet on the DUM?
     stackA.buildFdSet(fdset);
     stackB.buildFdSet(fdset);
     int err = fdset.selectMilliSeconds(100);
     assert ( err != -1 );
     stackA.process(fdset);
     stackB.process(fdset);

     if (bHangupAt!=0)
     {
       if (time(NULL)>bHangUpAt)
       {
         handlerB.hangup();
       }
     }
   }   
#endif

}
