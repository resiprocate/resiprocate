#include "sip2/util/ThreadIf.hxx"
#include "sip2/util/Timer.hxx"

#include <unistd.h>
#include <iostream>

using namespace Vocal2;

using namespace std;

class Every4 : public ThreadIf
{
   public:
      void thread()
      {
         while(!waitForShutdown(4000))
         {
            cerr << Timer::getTimeMs() << endl;
         }
         cerr << "Stopped at: " << Timer::getTimeMs() << endl;
      }
};

class Old2 : public ThreadIf
{
    public:
        void thread()
        {
            while(!isShutdown())
            {
                sleep(2);
                cerr << Timer::getTimeMs() << endl;
            }
            cerr << "Stopped at: " << Timer::getTimeMs() << endl;
        }
};

class ShutdownSelf : public ThreadIf
{
    public:
        void thread()
        {
            while(!isShutdown())
            {
            }
        }
};

int main()
{
   {
      ShutdownSelf s;
      s.run();
      s.shutdown();
      s.join();
   }
   {
        Every4 e;
        e.run();
        sleep(5);
        UInt64 t = Timer::getTimeMs();
        cerr << "Stopping thread at: " << Timer::getTimeMs() << endl;
        e.shutdown();
        e.join();
        UInt64 j = Timer::getTimeMs();
        cerr << j - t << endl;
        
        assert(j - t < 10);
    }
    //this should not cause problems(signal is being called on a condition with
    //no waiting threads).
    {
        Old2 o;
        o.run();
        sleep(3);
        o.shutdown();
        o.join();
    }
    cerr << "OK" << endl;
    return 0;
}
