
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
	{x
	  workToDo=true;
	}
    }
}

 
bool 
processTransports()
{
   mStack.mTransportSelector.process();
}


bool 
processStateMachine()
{
  if ( mStack.mStateMacFifo.size() == 0 ) 
    {
      return false;
    }

  TransactionState::process();

  return true;
}


bool 
processTimer()
{
  mStack.mTimers.process();

  return false;
}
