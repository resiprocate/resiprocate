#include "rutil/Logger.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "CommandLineParser.hxx"
#include "UserAgent.hxx"


using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST


class TestUserAgent : public UserAgent
{
   public:
      TestUserAgent(int argc, char** argv) : UserAgent(argc, argv)
      {
      }

      void startup()
      {
         InfoLog (<< "register for " << mAor);
         UInt32 e=0;
         NameAddr target(mAor);
         mDum.send(mDum.makeRegistration(target, e));
      }
         
      void onSuccess(ClientRegistrationHandle h, const SipMessage& response)
      {
         InfoLog (<< "Removing binding for " << *h);
         h->removeAll(true);
      }
};

int
main(int argc, char** argv)
{
   TestUserAgent tua(argc, argv);
   tua.startup();
   for (int i=0; i<100000; ++i)
   {
      tua.process();
      usleep(10);
   }
}
