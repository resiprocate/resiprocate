/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

/* Copyright 2007 Estacado Systems */
#ifndef PassthroughCommandFifo_Include_Guard
#define PassthroughCommandFifo_Include_Guard

#include "rutil/CommandFifo.hxx"

namespace resip
{

/**
   @brief Simple CommandFifo implementation.

   The PassthroughCommandFifo executes commands as they are posted, on
   the thread that calls post().

   If a CommandFifo is required to provide a thread boundary, consider
   using ThreadCommandFifo.
*/
class PassthroughCommandFifo : public CommandFifo
{
   public:
      PassthroughCommandFifo();
      virtual ~PassthroughCommandFifo();
      
      /**
         @param command The command will be executed immediately in
         this method.
      */
      virtual void post(std::auto_ptr<Command> command);
   
   private:
      // disabled
      PassthroughCommandFifo(const PassthroughCommandFifo& orig);
      PassthroughCommandFifo& operator=(const PassthroughCommandFifo& rhs);
};
}

#endif
