#if !defined(TimerQueue_HXX)
#define TimerQueue_HXX

#include <vector>
#include <set>

#include <util/Timer.hxx>
#include <util/Fifo.hxx>

namespace Vocal2
{

class Message;

class TimerQueue
{
   public:
      TimerQueue(Fifo<Message>& fifo);

      Timer::Id add(Timer::Type type, const Data& transactionId, unsigned long msOffset);
      //void cancel(Timer::Id tid);
      
      void process();
      void run();
      
   private:
      Fifo<Message>& mFifo;
      std::multiset<Timer> mTimers;
};
 
}

#endif
