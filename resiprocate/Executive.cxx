
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
processTransports()
{
}


bool 
processStateMachine()
{
  if ( mStack.mStateMacFifo.size() == ) 
    {
      return false;
    }

  
  
}


bool 
processTimer()
{
}
