#include <sipstack/SipStack.hxx>
#include <sipstack/Fifo.hxx>
#include <sipstack/Data.hxx>
#include <sipstack/Executive.hxx>
#include <sipstack/SipMessage.hxx>
#include <sipstack/Message.hxx>

using namespace Vocal2;

SipStack::SipStack()
  : mExecutive(*this),
    mTransportSelector(5060),
    mTimers(mStateMacFifo)
{

}

void 
SipStack::send(SipMessage* msg)
{
  SipMessage* toSend = msg->clone();
  mStateMacFifo.add(toSend);
  
}

// this is only if you want to send to a destination not in the route. You
// probably don't want to use it. 
void 
SipStack::send(SipMessage* msg, const Data& dest)
{

  SipMessage* toSend = msg->clone();
  toSend->setFixedDest(dest);
  mStateMacFifo.add(toSend);
  
}


SipMessage* 
SipStack::receive()
{
   // Check to see if a message is available and if it is return the 
   // waiting message. Otherwise, return 0
   if (mTUFifo.messageAvailable())
   {
      // we should only ever have SIP messages on the TU Fifo
      Message *tmpMsg = mTUFifo.getNext();
      SipMessage *sipMsg = dynamic_cast<SipMessage*>(tmpMsg);
      assert (sipMsg);
      return sipMsg;
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
  return 0;

} 

void 
SipStack::runThread( enum ThreadFunction funcType )
{
  
  
  
}



