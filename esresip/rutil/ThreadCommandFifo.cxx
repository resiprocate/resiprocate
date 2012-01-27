/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

/* Copyright 2007 Estacado Systems */
#include "rutil/ThreadCommandFifo.hxx"

#include "rutil/Command.hxx"
#include "rutil/EsLogger.hxx"

namespace resip
{

ThreadCommandFifo::ThreadCommandFifo()
{
   ES_TRACE(estacado::SBF_OLDLOG, "constructor - " << this);
}

ThreadCommandFifo::~ThreadCommandFifo()
{
   ES_TRACE(estacado::SBF_OLDLOG, "destructor - " << this);

   shutdown();
   join();
   clear();
}


void 
ThreadCommandFifo::post(std::auto_ptr<Command> command)
{
   mFifo.add(command.release());
}

void
ThreadCommandFifo::thread()
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

void 
ThreadCommandFifo::clear()
{
   while(mFifo.messageAvailable())
   {
      delete mFifo.getNext(1);
   }
}


}
