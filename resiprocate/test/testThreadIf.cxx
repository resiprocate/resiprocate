#include "resiprocate/util/ThreadIf.hxx"
#include "resiprocate/util/Timer.hxx"

#include <unistd.h>
#include <iostream>
#include <vector>
#include <cassert>

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
           while(!waitForShutdown(4000))
           {
           }
        }
};

class DerivedShutsDown : public ThreadIf
{
   public:
      DerivedShutsDown() {}
      ~DerivedShutsDown()
      {
         shutdown();
         join();
      }
         
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
      DerivedShutsDown d;
   }
      
   {
      vector<DerivedShutsDown*> threads;
      int numThreads = 20;
      
      for (int i=0; i < numThreads; i++)
      {
         threads.push_back(new DerivedShutsDown);
         threads.back()->run();
      }
      sleep(1);
      for (int i=0; i < numThreads; i++)
      {
         delete threads[i];
      }
      cerr << "finished many-thread test" << endl;
   }
   
   {
      DerivedShutsDown d;
      d.run();
      usleep(10000);
   }
   {
      ShutdownSelf* s= new ShutdownSelf();
      s->run();
      usleep(10000);
      s->shutdown();
      delete s;
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
