#include <sipstack/Executive.hxx>
#include <sipstack/SipStack.hxx>
#include <sipstack/TransactionState.hxx>
#include <sipstack/Logger.hxx>


using namespace Vocal2;
#define VOCAL_SUBSYSTEM Subsystem::SIP

Executive::Executive( SipStack& stack)
  : mStack(stack)
{

}


void
Executive::process()
{
  bool workToDo = true;

  DebugLog (<< std::endl << "start Executive::process()");
  
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

  DebugLog (<< "finish Executive::process()" << std::endl);
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
