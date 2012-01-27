#ifndef MultithreadedCommandFifo_Include_Guard
#define MultithreadedCommandFifo_Include_Guard

#include "rutil/CommandFifo.hxx"

#include "rutil/Fifo.hxx"
#include "rutil/ThreadIf.hxx"

#include <vector>

namespace resip
{
class CommandThread;

/**
   Multi-threaded CommandFifo subclass. You should probably not use this as the
   CommandFifo used by ReparteeManager, since ReparteeManager assumes that all
   Commands it passes will be invoked sequentially. This is intended to be used 
   as a generic thread-bank for invoking Commands, where order of execution does 
   not really matter.
*/
class MultithreadedCommandFifo : public CommandFifo
{
   public:
      explicit MultithreadedCommandFifo(size_t threads);
      virtual ~MultithreadedCommandFifo();

/////////////////// Must implement unless abstract ///

      virtual void post(std::auto_ptr<Command> command);

   protected:
      void shutdown();
      Fifo<Command> mFifo;
      std::vector<CommandThread*> mThreads;
}; // class MultithreadedCommandFifo

class CommandThread : public ThreadIf
{
   public:
      explicit CommandThread(Fifo<Command>& fifo);
      virtual ~CommandThread();
      virtual void thread();

   protected:
      Fifo<Command>& mFifo;
};

} // namespace resip

#endif // include guard
