/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

/* Copyright 2007 Estacado Systems */
#ifndef ThreadCommandFifo_Include_Guard
#define ThreadCommandFifo_Include_Guard

#include "rutil/CommandFifo.hxx"
#include "rutil/ThreadIf.hxx"
#include "rutil/Fifo.hxx"

namespace resip
{
/**
   This is a subclass of CommandFifo that invokes all of its Commands
   in a specific thread (regardless of where post() is called from).
   The intent of the class is to provide a thread boundary between the
   Command Pattern FIFO and the thread which is requesting the work.
   
   @note After creating one of these, ThreadCommandFifo::run() must
   be called to start the new thread's execution. 
*/
class ThreadCommandFifo : public CommandFifo, public ThreadIf
{
   public:
      ThreadCommandFifo();
      virtual ~ThreadCommandFifo();
      
      virtual void post(std::auto_ptr<Command> command);
      virtual void thread();

      /**
         Deletes all messages in the fifo. Only use this after shutdown() and 
         join() are called.
      */
      virtual void clear();
   private:
      Fifo<Command> mFifo;
      
      // disabled
      ThreadCommandFifo(const ThreadCommandFifo& orig);
      ThreadCommandFifo& operator=(const ThreadCommandFifo& rhs);
};
}

#endif
