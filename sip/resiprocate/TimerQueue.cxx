#include <sipstack/TimerQueue.hxx>
#include <sipstack/TimerMessage.hxx>
#include <sipstack/Logger.hxx>

using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::SIP


TimerQueue::TimerQueue(Fifo<Message>& fifo) : mFifo(fifo)
{
}

Timer::Id
TimerQueue::add(Timer::Type type, const Data& transactionId, unsigned long msOffset)
{
   Timer t(msOffset, type, transactionId);
   mTimers.insert(t);
   DebugLog (<< "Adding timer: " << type << " tid=" << transactionId << " ms=" << msOffset);
   
   return t.getId();
}

void
TimerQueue::process()
{
   // get the set of timers that have fired and insert TimerMsg into the fifo

   Timer now(Timer::getTimeMs());
   for (std::multiset<Timer>::iterator i=mTimers.lower_bound(now); 
        i != mTimers.upper_bound(now); i++)
   {
      TimerMessage* t = new TimerMessage(i->mTransactionId, i->mType);
      DebugLog (<< "Adding timer to fifo: " << " tid=" << i->mTransactionId << " type=" << i->mType);
      mFifo.add(t);
   }
}

void
TimerQueue::run()
{
   assert(0);
   // for the thread
}

