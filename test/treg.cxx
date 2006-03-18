#include "rutil/Logger.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/DumFeature.hxx"
#include "resip/dum/OutgoingEvent.hxx"

#include "CommandLineParser.hxx"
#include "RegEventClient.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST
   
// This feature will add a missing Contact header to incoming SIP messages on
// 2xx responses to SUBSCRIBE or on NOTIFY requests. 
class FixMissingContact : public DumFeature 
{
   public:
      FixMissingContact(DialogUsageManager& dum, TargetCommand::Target& target) : 
         DumFeature(dum, target)
      {
      }
      
      virtual ProcessingResult process(Message* msg)
      {
         SipMessage* sip = dynamic_cast<SipMessage*>(msg);

         if (sip)
         {
            switch (sip->header(h_CSeq).method())
            {
               case NOTIFY:
                  if (sip->isRequest() && !sip->exists(h_Contacts) && sip->header(h_Contacts).empty())
                  {
                     sip->header(h_Contacts).push_back(sip->header(h_From));
                  }
                  break;

               case SUBSCRIBE:
                  if (sip->isResponse() && 
                      sip->header(h_StatusLine).statusCode() / 200 == 1 &&
                      !sip->exists(h_Contacts) && 
                      sip->header(h_Contacts).empty())
                  {
                     sip->header(h_Contacts).push_back(sip->header(h_To));                  
                  }
                  break;

               default:
                  break;
            }
         }
         return DumFeature::FeatureDone;
      }
};


// This feature is used to hardcode the host and port of the Contact header and
// will apply to any outgoing SIP requests. 
class OverrideContact : public DumFeature 
{
   public:
      OverrideContact(const Uri& contact, 
                      DialogUsageManager& dum, 
                      TargetCommand::Target& target) : 
         DumFeature(dum, target),
         mContact(contact)
      {
      }
      
      virtual ProcessingResult process(Message* msg)
      {
         OutgoingEvent* og = dynamic_cast<OutgoingEvent*>(msg);
         if (og)
         {
            SharedPtr<SipMessage> sip = og->message();
            if (sip->isRequest() && 
                sip->exists(h_Contacts) && 
                sip->header(h_Contacts).size() == 1)
            {
               sip->header(h_Contacts).front().uri().host() = mContact.host();
               sip->header(h_Contacts).front().uri().port() = mContact.port();
            }
         }
         
         return DumFeature::FeatureDone;
      }

   private:
      Uri mContact;
};


class MyApp : public RegEventClient
{
   public:
      MyApp(resip::SharedPtr<resip::MasterProfile> profile) : RegEventClient(profile)
      {
         SharedPtr<DumFeature> f1(new FixMissingContact(mDum, mDum.dumIncomingTarget()));
         mDum.addIncomingFeature(f1);
      }

      // optionally call this method once to override the host/port of contact
      // on outbound requests
      void overrideContact(const resip::Uri& contact)
      {
         // This is only necessary if the user agent is running behind a NAT. 
         SharedPtr<DumFeature> f2(new OverrideContact(contact, mDum, mDum.dumOutgoingTarget()));
         mDum.addOutgoingFeature(f2);
      }
      
      virtual void onRegEvent(const resip::Data& aor, const resip::Data& reg)
      {
         WarningLog (<< "Got result from " << aor << " --> " << reg);
      }
      
      virtual void onRegEventError(const resip::Data& aor, const resip::Data& reg)
      {
         WarningLog (<< "Got error for " << aor);
      }
};
   
int
main(int argc, char** argv)
{
   CommandLineParser clp(argc, argv);
   Log::initialize(clp.mLogType, clp.mLogLevel, argv[0]);
   
   SharedPtr<MasterProfile> profile(new MasterProfile);

   NameAddr from(clp.mAor);
   profile->setDefaultFrom(from);
   
   MyApp client(profile);

   // only necessary if behind a NAT
   client.overrideContact(clp.mContact);
   
   client.run();
   
   Uri a1(clp.mAor);
   client.watchAor(a1);
   
   sleep(3600);
   return 0;
}

