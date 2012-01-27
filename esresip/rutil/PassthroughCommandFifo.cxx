/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

/* Copyright 2007 Estacado Systems */
#include "rutil/PassthroughCommandFifo.hxx"

#include "rutil/Command.hxx"

namespace resip
{
PassthroughCommandFifo::PassthroughCommandFifo()
{

}

PassthroughCommandFifo::~PassthroughCommandFifo()
{

}


void 
PassthroughCommandFifo::post(std::auto_ptr<Command> command)
{
   (*command)();
}


}
