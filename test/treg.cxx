#include "rutil/Logger.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "RegEventClient.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST


class MyApp : public RegEventClient
{
   public:
      MyApp(resip::SharedPtr<resip::MasterProfile> profile) : RegEventClient(profile)
      {
      }
      
      virtual void onRegEvent(const resip::Data& aor, const resip::Data& reg)
      {
         InfoLog (<< "Got result from " << aor << " --> " << reg);
      }
      
      virtual void onRegEventError(const resip::Data& aor, const resip::Data& reg)
      {
         InfoLog (<< "Got error for " << aor);
      }
};
   
int
main(int argc, char** argv)
{
   SharedPtr<MasterProfile> profile(new MasterProfile);
   NameAddr from(argv[1]);
   profile->setDefaultFrom(from);

   MyApp client(profile);
   client.run();
   
   Uri a1(argv[2]);
   client.watchAor(a1);
   
   sleep(3600);
   return 0;
}

