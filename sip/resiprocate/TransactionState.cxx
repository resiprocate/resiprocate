#include <sipstack/TransactionState.hxx>
#include <sipstack/SipStack.hxx>
#include <sipstack/SipMessage.hxx>
#include <sipstack/TimerMessage.hxx>
#include <sipstack/Logger.hxx>
#include <sipstack/MethodTypes.hxx>

using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::SIP

TransactionState::TransactionState(SipStack& stack, Machine m, State s) : 
   mStack(stack),
   mMachine(m), 
   mState(s),
   mIsReliable(false), // !jf!
   mCancelStateMachine(0),
   mMsgToRetransmit(0)
{
}

TransactionState::~TransactionState()
{
   delete mCancelStateMachine;
   mCancelStateMachine = 0;
   
   delete mMsgToRetransmit;
   mMsgToRetransmit = 0;

   mState = Bogus;
}


void
TransactionState::process(SipStack& stack)
{
   Message* message = stack.mStateMacFifo.getNext();
   assert(message);
   DebugLog (<< "got message out of state machine fifo: " << *message);
   
   SipMessage* sip = dynamic_cast<SipMessage*>(message);
   TimerMessage* timer=0;
   
   if (sip == 0)
   {
      timer = dynamic_cast<TimerMessage*>(message);
   }
   
   const Data& tid = message->getTransactionId();
   TransactionState* state = stack.mTransactionMap.find(tid);
   if (state) // found transaction for sip msg
   {
      DebugLog (<< "Found transaction for msg " << *state);
      
      switch (state->mMachine)
      {
         case ClientNonInvite:
            state->processClientNonInvite(message);
            break;
         case ClientInvite:
            state->processClientInvite(message);
            break;
         case ServerNonInvite:
            state->processServerNonInvite(message);
            break;
         case ServerInvite:
            state->processClientInvite(message);
            break;
         case Stale:
            state->processStale(message);
            break;
         default:
            assert(0);
      }
   }
   else // new transaction
   {
      DebugLog (<< "Create new transaction for msg ");
      if (sip)
      {
         if (sip->isRequest())
         {
            // create a new state object and insert in the TransactionMap
               
            if (sip->isExternal()) // new sip msg from transport
            {
               if ((*sip)[RequestLine].getMethod() == INVITE)
               {
                  TransactionState* state = new TransactionState(stack, ServerInvite, Proceeding);
                  stack.mTimers.add(Timer::TimerTrying, tid, Timer::T100);
                  stack.mTransactionMap.add(tid,state);
               }
               else 
               {
                  TransactionState* state = new TransactionState(stack, ServerNonInvite,Trying);
                  stack.mTransactionMap.add(tid,state);
               }
               DebugLog(<< "Adding incoming message to TU fifo");
               stack.mTUFifo.add(sip);
            }
            else // new sip msg from the TU
            {
               if ((*sip)[RequestLine].getMethod() == INVITE)
               {
                  TransactionState* state = new TransactionState(stack, ClientInvite, Calling);
                  stack.mTimers.add(Timer::TimerB, tid, 64*Timer::T1 );
                  stack.mTransactionMap.add(tid,state);
               }
               else 
               {
                  TransactionState* state = new TransactionState(stack, ClientNonInvite, Trying);
                  stack.mTimers.add(Timer::TimerF, tid, 64*Timer::T1 );
                  stack.mTransactionMap.add(tid,state);
               }
               DebugLog(<< "Sending sip message to transport selector");
               stack.mTransportSelector.send(sip);
            }
         }
         else if (sip->isResponse()) // stray response
         {
            if (stack.mDiscardStrayResponses)
            {
               DebugLog (<< "discarding stray response: " << sip->brief());
               delete message;
            }
            else
            {
               // forward this statelessly
               assert(0);
            }
         }
         else // wasn't a request or a response
         {
            DebugLog (<< "discarding unknown message: " << sip->brief());
         }
      } 
      else // timer or other non-sip msg
      {
         DebugLog (<< "discarding non-sip message: " << message->brief());
         delete message;
      }
   }
}

void
TransactionState::processClientNonInvite(  Message* msg )
{ 

}


void
TransactionState::processClientInvite(  Message* msg )
{
}


void
TransactionState::processServerNonInvite(  Message* msg )
{
   if (isRequest(msg) && !isFromTU(msg)) // from the wire
   {
      if (mState == Trying)
      {
         // ignore
         delete msg;
      }
      else if (mState == Proceeding || mState == Trying)
      {
         sendToWire(mMsgToRetransmit); 
         delete msg;
      }
      else
      {
         assert(0);
      }
   }
   else if (isResponse(msg) && isFromTU(msg))
   {
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      int code = (*sip)[StatusLine].getResponseCode();
      if (code >= 100 && code < 200) // 1XX
      {
         if (mState == Trying || mState == Proceeding)
         {
            delete mMsgToRetransmit;
            mMsgToRetransmit = sip;
            mState = Proceeding;
            sendToWire(sip); // don't delete msg
         }
         else
         {
            // ignore
            delete msg;
         }
      }
      else if (code >= 200 && code <= 699)
      {
         if (mIsReliable)
         {
            mMsgToRetransmit = sip;
            sendToWire(sip); // don't delete msg
            delete this;
         }
         else
         {
            if (mState == Trying || mState == Proceeding)
            {
               mState = Completed;
               mStack.mTimers.add(Timer::TimerJ, msg->getTransactionId(), 64*Timer::T1 );
               mMsgToRetransmit = sip;
               sendToWire(sip); // don't delete msg
            }
            else if (mState == Completed)
            {
               // ignore
               delete msg;               
            }
            else
            {
               assert(0);
            }
         }
      }
      else
      {
         // ignore
         delete msg;               
      }
   }
   else if (isTimer(msg))
   {
      assert (mState == Completed);
      assert(dynamic_cast<TimerMessage*>(msg)->getType() == Timer::TimerJ);
      delete msg;
      delete this;
   }
   else if (isTranportError(msg))
   {
      // inform TU
      delete msg;
      delete this;
      assert(0);
   }
}


void
TransactionState::processServerInvite(  Message* msg )
{
   if (isRequest(msg) && !isFromTU(msg))
   {
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      switch ((*sip)[RequestLine].getMethod())
      {
         case INVITE:
            if (mState == Proceeding || mState == Completed)
            {
               DebugLog (<< "Received invite from wire - forwarding to TU state=" << mState);
               mMsgToRetransmit = make100(sip); // for when TimerTrying fires
               sendToTU(msg); // don't delete
            }
            else
            {
               DebugLog (<< "Received invite from wire - ignoring state=" << mState);
               delete msg;
            }
            break;
            
         case ACK:
            if (mState == Completed)
            {
               if (mIsReliable)
               {
                  DebugLog (<< "Received ACK in Completed (reliable) - delete transaction");
                  delete this; 
                  delete msg;
               }
               else
               {
                  DebugLog (<< "Received ACK in Completed (unreliable) - confirmed, start Timer I");
                  mState = Confirmed;
                  mStack.mTimers.add(Timer::TimerI, msg->getTransactionId(), Timer::T4 );
                  delete msg;
               }
            }
            else
            {
               DebugLog (<< "Ignore ACK not in Completed state");
               delete msg;
            }
            break;

         case CANCEL:
            DebugLog (<< "Received Cancel, create Cancel transaction and process as server non-invite and send to TU");
            mCancelStateMachine = new TransactionState(mStack, ServerNonInvite, Trying);
            mStack.mTransactionMap.add(msg->getTransactionId(), mCancelStateMachine);
            mCancelStateMachine->processServerNonInvite(msg);
            sendToTU(msg); // don't delete msg
            break;

         default:
            DebugLog (<< "Received unexpected request. Ignoring message");
            delete msg;
            break;
      }
   }
   else if (isResponse(msg, 100, 699) && isFromTU(msg))
   {
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      int code = (*sip)[StatusLine].getResponseCode();
      
      switch ((*sip)[CSeq].getMethod())
      {
         case INVITE:
            if (code == 100)
            {
               if (mState == Trying)               
               {
                  DebugLog (<< "Received 100 in Trying State. Send over wire");
                  delete mMsgToRetransmit; // may be replacing the 100
                  mMsgToRetransmit = sip;
                  mState = Proceeding;
                  sendToWire(msg); // don't delete msg
               }
               else
               {
                  DebugLog (<< "Received 100 when not in Trying State. Ignoring");
                  delete msg;
               }
            }
            else if (code > 100 && code < 200)
            {
               if (mState == Trying || mState == Proceeding)
               {
                  DebugLog (<< "Received 100 in Trying or Proceeding. Send over wire");
                  delete mMsgToRetransmit; // may be replacing the 100
                  mMsgToRetransmit = sip;
                  mState = Proceeding;
                  sendToWire(msg); // don't delete msg
               }
               else
               {
                  DebugLog (<< "Received 100 when not in Trying State. Ignoring");
                  delete msg;
               }
            }
            else if (code >= 200 && code < 300)
            {
               if (mState == Trying || mState == Proceeding)
               {
                  DebugLog (<< "Received 2xx when in Trying or Proceeding State. Start Stale Timer, move to terminated.");
                  delete mMsgToRetransmit; 
                  mMsgToRetransmit = sip; // save it, even though it won't be transmitted
                  mMachine = Stale;
                  mState = Terminated;
                  mStack.mTimers.add(Timer::TimerStale, msg->getTransactionId(), Timer::TS );
                  sendToWire(msg); // don't delete
               }
               else
               {
                  DebugLog (<< "Received 2xx when not in Trying or Proceeding State. Ignoring");
                  delete msg;
               }
            }
            else if (code >= 300)
            {
               if (mState == Trying || mState == Proceeding)
               {
                  DebugLog (<< "Received failed response in Trying or Proceeding. Start Timer H, move to completed.");
                  delete mMsgToRetransmit; 
                  mMsgToRetransmit = sip; // save it, even though it won't be transmitted
                  mMachine = Stale;
                  mState = Completed;
                  mStack.mTimers.add(Timer::TimerH, msg->getTransactionId(), 64*Timer::T1 );
                  if (!mIsReliable)
                  {
                     mStack.mTimers.add(Timer::TimerG, msg->getTransactionId(), Timer::T1 );
                  }
                  sendToWire(msg); // don't delete msg
               }
               else
               {
                  DebugLog (<< "Received Final response when not in Trying or Proceeding State. Ignoring");
                  delete msg;
               }
            }
            else
            {
               DebugLog (<< "Received Invalid response line. Ignoring");
               delete msg;
            }
            break;
            
         case CANCEL:
            DebugLog (<< "Received Cancel, create Cancel transaction and process as server non-invite and send to TU");
            
            mCancelStateMachine = new TransactionState(mStack, ServerNonInvite, Trying);
            mStack.mTransactionMap.add(msg->getTransactionId(), mCancelStateMachine);
            mCancelStateMachine->processServerNonInvite(msg);
            sendToTU(msg); // don't delete
            break;
            
         default:
            DebugLog (<< "Received response to non invite or cancel. Ignoring");
            delete msg;
            break;
      }
   }
   else if (isTimer(msg))
   {
      TimerMessage* timer = dynamic_cast<TimerMessage*>(msg);
      switch (timer->getType())
      {
         case Timer::TimerG:
            if (mState == Completed)
            {
               DebugLog (<< "TimerG fired. retransmit, and readd TimerG");
               sendToWire(mMsgToRetransmit); // don't delete msg
               mStack.mTimers.add(Timer::TimerG, msg->getTransactionId(), timer->getDuration()*2 );
            }
            else
            {
               delete msg;
            }
            break;
            
         case Timer::TimerH:
         case Timer::TimerI:
            DebugLog (<< "TimerH or TimerI fired. Delete this");
            delete this;
            delete msg;
            break;
            
         case Timer::TimerJ:
            DebugLog (<< "TimerJ fired. Delete state of cancel");
            mCancelStateMachine = 0;
            delete mCancelStateMachine;
            delete msg;
            break;
            
         case Timer::TimerTrying:
            if (mState == Trying)
            {
               DebugLog (<< "TimerTrying fired. Send a 100");
               sendToWire(mMsgToRetransmit); // will get deleted when this is deleted
               delete msg;
            }
            else
            {
               DebugLog (<< "TimerTrying fired. Not in Trying state. Ignoring");
               delete msg;
            }
            break;
            
         default:
            assert(0); // programming error if any other timer fires
            break;
      }
   }
   else if (isTranportError(msg))
   {
      DebugLog (<< "Transport error received. Delete this");
      delete this;
      delete msg;
   }
   else
   {
      DebugLog (<< "TransactionState::processServerInvite received " << *msg << " out of context");
      assert(0); // !jf! programming error
   }
}


void
TransactionState::processStale(  Message* msg )
{
}

bool
TransactionState::isRequest(Message* msg) const
{
   SipMessage* sip = dynamic_cast<SipMessage*>(msg);   
   return sip && sip->isRequest();
}

bool
TransactionState::isResponse(Message* msg, int lower, int upper) const
{
   SipMessage* sip = dynamic_cast<SipMessage*>(msg);
   if (sip)
   {
      int c = (*sip)[StatusLine].getResponseCode();
      return (c >= lower && c <= upper);
   }
   return false;
}

bool
TransactionState::isTimer(Message* msg) const
{
   return dynamic_cast<TimerMessage*>(msg) != 0;
}


bool
TransactionState::isFromTU(Message* msg) const
{
   SipMessage* sip = dynamic_cast<SipMessage*>(msg);
   return sip && !sip->isExternal();
}

bool
TransactionState::isTranportError(Message* msg) const
{
   return false; // !jf!
}

void
TransactionState::sendToWire(Message* msg) const
{
   SipMessage* sip=dynamic_cast<SipMessage*>(msg);
   assert(sip);
   mStack.mTransportSelector.send(sip);
}

void
TransactionState::sendToTU(Message* msg) const
{
   SipMessage* sip=dynamic_cast<SipMessage*>(msg);
   assert(sip);
   mStack.mTUFifo.add(sip);
}

SipMessage*
TransactionState::make100(SipMessage* request) const
{
   SipMessage* response = 0;
   // make sure request is a request and build it !jf!
   return response;
}


std::ostream& 
Vocal2::operator<<(std::ostream& strm, const Vocal2::TransactionState& state)
{
   strm << "Tstate[ mMach=" << state.mMachine <<  " mState=" 
        << state.mState << " mIsRel=" << state.mIsReliable;
   return strm;
}
