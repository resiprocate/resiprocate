#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx" 
#endif

#include <iostream>

#if defined (HAVE_POPT_H) 
#include <popt.h>
#else
#ifndef WIN32
#warning "will not work very well without libpopt"
#endif
#endif

#include "resiprocate/os/SelectInterruptor.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/os/ThreadIf.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

class FakeApp : public ThreadIf
{
   public: 
      FakeApp(SelectInterruptor& s);
      ~FakeApp() {};
      
      void thread();
   private:
      SelectInterruptor& mSi;
};

FakeApp::FakeApp(SelectInterruptor& s) : mSi(s)
{}

void FakeApp::thread()
{
    static unsigned wakeups[6] = { 3, 1, 0, 2, 5, 1 };

    for (unsigned long n = 0; n < sizeof(wakeups)/sizeof(long); n++)
    {
       InfoLog( << "Wakeup in: " << wakeups[n] << ", " << n+1 << " of " 
                << sizeof(wakeups)/sizeof(long));
       sleep(wakeups[n]);
       InfoLog(<< "Waking up select");
       mSi.interrupt();
    }
    shutdown();
}


int
main(int argc, char* argv[])
{
   Log::initialize(Log::Cout, Log::Debug, argv[0]);
   
   SelectInterruptor si;
   FakeApp app(si);

   InfoLog(<< "Starting FakeApp");
   app.run();

   int numWakeups = 0;
   while(!app.isShutdown())
   {
      FdSet fdset;
      si.buildFdSet(fdset);
      
      int ret = fdset.selectMilliSeconds(10000);
      
      if (ret > 0)
      {
         InfoLog(<< "Select detected: " << ret << " ready descriptors");
         si.process(fdset);
         numWakeups++;
      }
      else
      {
         InfoLog(<< "Select detected no ready descriptors, test failed");
         break;
      }
   }

   if (numWakeups == 6)
   {
      InfoLog(<< "Finished, test passed");
   }
   app.shutdown();
   app.join();
}
         
   
            

