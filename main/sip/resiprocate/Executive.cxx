#include <sipstack/Executive.hxx>
#include <sipstack/SipStack.hxx>
#include <sipstack/TransactionState.hxx>

using namespace Vocal2;

Executive::Executive( SipStack& stack)
  : mStack(stack)
{

}


void
Executive::process()
{
  bool workToDo = true;

  while( workToDo )
    {
      workToDo = false;

      if ( processTransports() )
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
}

 
bool 
Executive::processTransports() 
{
   mStack.mTransportSelector.process();

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
