
using Vocal2;

void 
SipStack::send(SipMessage* msg)
{
  SipMessage* toSend = msg.clone();
  mStateMacFifo.add(msg)
  
}

// this is only if you want to send to a destination not in the route. You
// probably don't want to use it. 
void 
SipStack::send(SipMessage* msg, const Data& dest="default" )
{

  SipMessage* toSend = msg.clone();
  toSend->setFixedDest(dest);
  mStateMacFifo.add(msg);
  
}


SipMessage* 
SipStack::receive()
{

  // Check to see if a message is available and if it is return the 
  // waiting message. Otherwise, return 0
  if (mTUFifo.messageAvailable())
    {
      return mTUFifo.getNext();
    }
  return 0;
  
}


void 
SipStack::process()
{
  
  mExecutive.process();
  
}


/// returns time in milliseconds when process next needs to be called 
int 
SipStack::getTimeTillNextProcess()
{

  // FIX there needs to be some code here once the executive can tell
  // us this
  
} 

void 
runThread( ThreadFunction )
{
  
  
  
}



