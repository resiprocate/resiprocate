
#include "resiprocate/util/Logger.hxx"
#include "resiprocate/util/Data.hxx"
#include "resiprocate/util/ThreadIf.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP


class LogThread : public ThreadIf
{
    public:
      LogThread(const Data& description, const Log::ThreadSetting& s)
         : mDescription(description),
           mSetting(s)
      {}

      void thread()
      {
         Log::setThreadSetting(mSetting);
         while(!waitForShutdown(100))
         {
            DebugLog(<< mDescription << "  DEBUG");
            InfoLog(<< mDescription << "  INFO");
         }
      }
   private:
      Data mDescription;
      Log::ThreadSetting mSetting;
};

int logsInCall()
{
   InfoLog(<< "Got here?");
   return 17;
}

int
main(int argc, char* argv[])
{
   Log::initialize(Log::SYSLOG, Log::INFO, argv[0]);

   DebugLog(<<"This should not appear.");
   InfoLog(<<"This should appear.");
   
   LogThread service1a("service1----A", Log::ThreadSetting(1, Log::DEBUG));
   LogThread service1b("service1-------B", Log::ThreadSetting(1, Log::DEBUG));
   LogThread service1c("service1---------C", Log::ThreadSetting(1, Log::DEBUG));

   LogThread service2a("service2-----------A", Log::ThreadSetting(2, Log::DEBUG));
   LogThread service2b("service2------------------B", Log::ThreadSetting(2, Log::ERR));

   service1a.run();
   service1b.run();
   service1c.run();
//   service2a.run();
//   service2b.run();

   sleep(2);

   InfoLog(<<"Setting service 1 to INFO\n");
   Log::setServiceLevel(1, Log::INFO);
   sleep(2);

   InfoLog(<<"Setting service 1 to CRIT\n");
   Log::setServiceLevel(1, Log::CRIT);
   sleep(2);

   InfoLog(<<"Setting service 2 to DEBUG_STACK\n");
   Log::setServiceLevel(2, Log::DEBUG_STACK);
   sleep(2);

   InfoLog(<<"Setting service 1 to DEBUG\n");
   Log::setServiceLevel(1, Log::DEBUG);
   sleep(2);

   DebugLog(<<"This should still not appear.");
   InfoLog(<<"This should still appear.");

   service1a.shutdown();
   service1b.shutdown();
   service1c.shutdown();
   service2a.shutdown();
   service2b.shutdown();

   service1a.join();
   service1b.join();
   service1c.join();
   service2a.join();
   service2b.join();

   cerr << "All OK -- will assert or lock next" << endl;
   // this will assert except on WIN32 where it will deadlock
   InfoLog(<< "Assert or deadlock: " << logsInCall());
}
