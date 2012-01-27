#include "rutil/MultithreadedCommandFifo.hxx"

#include "rutil/Command.hxx"

namespace resip
{
MultithreadedCommandFifo::MultithreadedCommandFifo(size_t threads)
{
   for(;threads>0;--threads)
   {
      mThreads.push_back(new CommandThread(mFifo));
      mThreads.back()->run();
   }
}

MultithreadedCommandFifo::~MultithreadedCommandFifo()
{
   shutdown();
}

void 
MultithreadedCommandFifo::post(std::auto_ptr<Command> command)
{
   mFifo.add(command.release());
}

void 
MultithreadedCommandFifo::shutdown()
{
   for(size_t i=0; i<mThreads.size(); ++i)
   {
      mThreads[i]->shutdown();
      mThreads[i]->join();
      delete mThreads[i];
   }
   mThreads.clear();
}

CommandThread::CommandThread(Fifo<Command>& fifo) :
   mFifo(fifo)
{}

CommandThread::~CommandThread()
{}

void 
CommandThread::thread()
{
   Command* command=0;
   while(!isShutdown())
   {
      if( (command=mFifo.getNext(100)) )
      {
         (*command)();
         delete command;
      }
   }
}
} // namespace resip
