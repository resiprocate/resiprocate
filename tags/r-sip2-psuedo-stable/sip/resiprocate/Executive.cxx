#include <sipstack/Executive.hxx>
#include <sipstack/SipStack.hxx>
#include <sipstack/TransactionState.hxx>
#include <util/Logger.hxx>

using namespace Vocal2;
#define VOCAL_SUBSYSTEM Subsystem::SIP

Executive::Executive( SipStack& stack)
  : mStack(stack)
{

}


void
Executive::process(fd_set* fdSet)
{
  bool workToDo = true;

  //DebugLog (<< "start Executive::process()");
  
  while( workToDo )
    {
      workToDo = false;

      if ( processTransports(fdSet) )
	{
	  workToDo=true;
	}
      
      if ( processTimer()) 
	{
	  workToDo=true;
	}

      if ( processStateMachine()) 
	{
	  workToDo=true;
	}
    }

  //DebugLog (<< "finish Executive::process()");
}

 
bool 
Executive::processTransports(fd_set* fdSet) 
{
   mStack.mTransportSelector.process(fdSet);

   return false;
}


bool 
Executive::processStateMachine()
{
   if ( mStack.mStateMacFifo.size() == 0 ) 
   {
      return false;
   }
   
   TransactionState::process(mStack);
   
   return true;
}

bool 
Executive::processTimer()
{
  mStack.mTimers.process();

  return false;
}



/// returns time in milliseconds when process next needs to be called 
int 
Executive::getTimeTillNextProcess()
{
   // FIX there needs to be some code here once the executive can tell
   // us this
   return 50;

} 


void 
Executive::buildFdSet( fd_set* fdSet, int* fdSetSize )
{
	assert( fdSet );
	assert( fdSetSize );
	
	mStack.mTransportSelector.buildFdSet( fdSet, fdSetSize );
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
