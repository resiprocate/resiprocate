/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

/* Copyright 2007 Estacado Systems */
#ifndef CommandFifo_Include_Guard
#define CommandFifo_Include_Guard

#include <memory>

namespace resip
{
class Command;

/**
   This is a class that will call operator() on Commands passed to it. This 
   could allow for things like moving work across thread-boundaries. It is 
   expected that this will handle the deletion of the Command objects after they
   are invoked.
*/
class CommandFifo
{
   public:
      virtual ~CommandFifo() {};
      virtual void post(std::auto_ptr<Command> command)=0;
};
}

#endif
