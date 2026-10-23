// Regression test for GitHub issue #168: the Contact in a 200 to an
// out-of-dialog request (OPTIONS) must honour setOverrideHostAndPort() on
// the UserProfile of the UAS DialogSet, like the Contacts that Dialog and
// BaseCreator build do. Without an override the transport still fills in
// its own address. Covers DUM's automatic OPTIONS answer (answerOptions())
// and an application handler that answers with accept().
//
// Neither side sets an override on its MasterProfile, so this also checks
// that the per-DialogSet UserProfile is used: the one the UAS picks in
// AppDialogSet::selectUASUserProfile() for the response, and the one the
// UAC passes to makeOutOfDialogRequest() for the request.

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/AppDialogSetFactory.hxx"
#include "resip/dum/ClientOutOfDialogReq.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/OutOfDialogHandler.hxx"
#include "resip/dum/ServerOutOfDialogReq.hxx"
#include "resip/dum/UserProfile.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;
using namespace std;

// Gives every UAS DialogSet the same non-Master UserProfile.
class TestAppDialogSet : public AppDialogSet
{
   public:
      TestAppDialogSet(DialogUsageManager& dum, std::shared_ptr<UserProfile> profile)
         : AppDialogSet(dum), mProfile(std::move(profile))
      {
      }

   protected:
      virtual std::shared_ptr<UserProfile> selectUASUserProfile(const SipMessage&)
      {
         return mProfile;
      }

   private:
      std::shared_ptr<UserProfile> mProfile;
};

class TestAppDialogSetFactory : public AppDialogSetFactory
{
   public:
      TestAppDialogSetFactory(std::shared_ptr<UserProfile> profile) : mProfile(std::move(profile))
      {
      }

      virtual AppDialogSet* createAppDialogSet(DialogUsageManager& dum, const SipMessage&)
      {
         return new TestAppDialogSet(dum, mProfile);
      }

   private:
      std::shared_ptr<UserProfile> mProfile;
};

class TestOutOfDialogHandler : public OutOfDialogHandler
{
   public:
      Data name;
      bool sawSuccess;
      bool sawRequest;
      NameAddr contactSeen;
      NameAddr requestContactSeen;

      TestOutOfDialogHandler(const Data& n) : name(n), sawSuccess(false), sawRequest(false)
      {
      }

      // UAC side: capture the Contact from the 200.
      virtual void onSuccess(ClientOutOfDialogReqHandle, const SipMessage& msg)
      {
         cout << name << ": ClientOutOfDialogReq-onSuccess - " << msg.brief() << endl;
         assert(msg.exists(h_Contacts) && msg.header(h_Contacts).size() == 1);
         contactSeen = msg.header(h_Contacts).front();
         sawSuccess = true;
      }

      virtual void onFailure(ClientOutOfDialogReqHandle, const SipMessage& msg)
      {
         cout << name << ": ClientOutOfDialogReq-onFailure - " << msg.brief() << endl;
         assert(false);
      }

      // UAS side, once the handler is registered: capture the Contact from
      // the OPTIONS and answer with accept() rather than answerOptions().
      virtual void onReceivedRequest(ServerOutOfDialogReqHandle ood, const SipMessage& msg)
      {
         cout << name << ": ServerOutOfDialogReq-onReceivedRequest - " << msg.brief() << endl;
         assert(msg.exists(h_Contacts) && msg.header(h_Contacts).size() == 1);
         requestContactSeen = msg.header(h_Contacts).front();
         sawRequest = true;
         ood->send(ood->accept(200));
      }
};

static void
sendOptions(SipStack& stackUac, DialogUsageManager& dumUac,
            SipStack& stackUas, DialogUsageManager& dumUas,
            const std::shared_ptr<UserProfile>& uacProfile,
            TestOutOfDialogHandler& uac)
{
   uac.sawSuccess = false;
   dumUac.send(dumUac.makeOutOfDialogRequest(NameAddr("sip:uas@127.0.0.1:27301"), uacProfile, OPTIONS));

   for (int i = 0; i < 20 && !uac.sawSuccess; i++)
   {
      stackUac.process(50);
      while (dumUac.process());
      stackUas.process(50);
      while (dumUas.process());
   }

   assert(uac.sawSuccess);
   cout << "Contact in 200: " << uac.contactSeen << endl;
}

int
main(int argc, char* argv[])
{
   Log::initialize(Log::Cout, resip::Log::Info, argv[0]);

   // e.g. the externally reachable addresses of UAs running in containers
   const Uri uacOverrideHostAndPort("sip:10.10.10.9:12011");
   const Uri uasOverrideHostAndPort("sip:10.10.10.8:12010");

   // Set up UAC. Requests are sent with a UserProfile of their own.
   SipStack stackUac;
   DialogUsageManager dumUac(stackUac);
   stackUac.addTransport(UDP, 27300);

   auto uacMasterProfile = std::make_shared<MasterProfile>();
   dumUac.setMasterProfile(uacMasterProfile);
   auto uacUserProfile = std::make_shared<UserProfile>(uacMasterProfile);
   uacUserProfile->setDefaultFrom(NameAddr("sip:uac@127.0.0.1:27300"));
   uacUserProfile->setOverrideHostAndPort(uacOverrideHostAndPort);

   TestOutOfDialogHandler uac("UAC");
   dumUac.addOutOfDialogHandler(OPTIONS, &uac);

   // Set up UAS, with no OPTIONS handler so that DUM answers by itself.
   // Incoming requests get a UserProfile of their own.
   SipStack stackUas;
   DialogUsageManager dumUas(stackUas);
   stackUas.addTransport(UDP, 27301);

   auto uasMasterProfile = std::make_shared<MasterProfile>();
   dumUas.setMasterProfile(uasMasterProfile);
   auto uasUserProfile = std::make_shared<UserProfile>(uasMasterProfile);
   dumUas.setAppDialogSetFactory(std::make_unique<TestAppDialogSetFactory>(uasUserProfile));

   // 1. Automatic answer, with an override.
   uasUserProfile->setOverrideHostAndPort(uasOverrideHostAndPort);
   sendOptions(stackUac, dumUac, stackUas, dumUas, uacUserProfile, uac);
   assert(uac.contactSeen.uri().host() == uasOverrideHostAndPort.host());
   assert(uac.contactSeen.uri().port() == uasOverrideHostAndPort.port());
   assert(uac.contactSeen.uri().user() == "uas");

   // 2. Automatic answer, without an override: the transport's address.
   uasUserProfile->unsetOverrideHostAndPort();
   sendOptions(stackUac, dumUac, stackUas, dumUas, uacUserProfile, uac);
   assert(!uac.contactSeen.uri().host().empty());
   assert(uac.contactSeen.uri().host() != uasOverrideHostAndPort.host());
   assert(uac.contactSeen.uri().port() == 27301);
   assert(uac.contactSeen.uri().user() == "uas");

   // 3. Application answers with accept(), with an override.
   TestOutOfDialogHandler uas("UAS");
   dumUas.addOutOfDialogHandler(OPTIONS, &uas);
   uasUserProfile->setOverrideHostAndPort(uasOverrideHostAndPort);
   sendOptions(stackUac, dumUac, stackUas, dumUas, uacUserProfile, uac);
   assert(uac.contactSeen.uri().host() == uasOverrideHostAndPort.host());
   assert(uac.contactSeen.uri().port() == uasOverrideHostAndPort.port());
   assert(uac.contactSeen.uri().user() == "uas");

   // ...and the OPTIONS itself carried the UAC UserProfile's override.
   assert(uas.sawRequest);
   cout << "Contact in OPTIONS: " << uas.requestContactSeen << endl;
   assert(uas.requestContactSeen.uri().host() == uacOverrideHostAndPort.host());
   assert(uas.requestContactSeen.uri().port() == uacOverrideHostAndPort.port());
   assert(uas.requestContactSeen.uri().user() == "uac");

   // Neither MasterProfile was given an override.
   assert(!uacMasterProfile->hasOverrideHostAndPort());
   assert(!uasMasterProfile->hasOverrideHostAndPort());

   cout << "!!!!!!!!!!!!!!!!!! Successful !!!!!!!!!! " << endl;
}

