#include <sipstack/SipStack.hxx>
#include <sipstack/Executive.hxx>
#include <sipstack/SipMessage.hxx>
#include <sipstack/Message.hxx>
#include <util/Fifo.hxx>
#include <util/Data.hxx>
#include <util/Logger.hxx>


using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::SIP
SipStack::SipStack()
  : mExecutive(*this),
    mTransportSelector(*this),
    mTimers(mStateMacFifo)
{

}

void 
SipStack::send(const SipMessage& msg)
{
   SipMessage* toSend = new SipMessage(msg);
   mStateMacFifo.add(toSend);
}

// this is only if you want to send to a destination not in the route. You
// probably don't want to use it. 
void 
SipStack::sendTo(const SipMessage& msg, const Data& dest)
{
   SipMessage* toSend = new SipMessage(msg);
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
      DebugLog (<< "message available");
      
      // we should only ever have SIP messages on the TU Fifo
      Message *tmpMsg = mTUFifo.getNext();
      SipMessage *sipMsg = dynamic_cast<SipMessage*>(tmpMsg);
      assert (sipMsg);
      return sipMsg;
   }
   else
   {
      DebugLog (<< "no message available");
      return 0;
   }
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

