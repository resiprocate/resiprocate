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
