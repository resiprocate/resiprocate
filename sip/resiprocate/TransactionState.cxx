#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/DnsInterface.hxx"
#include "resiprocate/DnsResult.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/MethodTypes.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/TimerMessage.hxx"
#include "resiprocate/TransactionController.hxx"
#include "resiprocate/TransactionState.hxx"
#include "resiprocate/TransactionTerminated.hxx"
#include "resiprocate/TransportMessage.hxx"
#include "resiprocate/TransportSelector.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/MD5Stream.hxx"
#include "resiprocate/os/Socket.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION


TransactionState::TransactionState(TransactionController& controller, Machine m, State s, const Data& id) : 
   mController(controller),
   mMachine(m), 
   mState(s),
   mIsCancel(false),
   mIsReliable(true), // !jf! 
   mMsgToRetransmit(0),
   mDnsResult(0),
   mId(id)
{
   DebugLog (<< "Creating new TransactionState: " << *this);
}


TransactionState* 
TransactionState::makeCancelTransaction(TransactionState* tr, Machine machine)
{
   TransactionState* cancel = new TransactionState(tr->mController, machine, Trying, tr->mId + "cancel");
   // !jf! don't set this since it will be set by TransactionState::processReliability()
   //cancel->mIsReliable = tr->mIsReliable;  
   cancel->mSource = tr->mSource;
   cancel->mIsCancel = true;
   cancel->mTarget = tr->mTarget;
   
   return cancel;
}

TransactionState::~TransactionState()
{
   assert(mState != Bogus);

   if (mDnsResult)
   {
      mDnsResult->destroy();
   }

   DebugLog (<< "Deleting TransactionState " << mId << " : " << this);
   erase(mId);
   
   delete mMsgToRetransmit;
   mMsgToRetransmit = 0;

   mState = Bogus;
}

void
TransactionState::process(TransactionController& controller)
{
   Message* message = controller.mStateMacFifo.getNext();
   assert(message);

   SipMessage* sip = dynamic_cast<SipMessage*>(message);
   if (sip && sip->isExternal() && sip->header(h_Vias).empty())
   {
      InfoLog(<< "TransactionState::process dropping message with no Via: " << sip->brief());
      delete sip;
      return;
   }
   Data tid = message->getTransactionId();

   // This ensures that CANCEL requests form unique transactions
   if (sip && sip->header(h_CSeq).method() == CANCEL) 
   {
      tid += "cancel";
   }
   
   TransactionState* state = 0;
   if (message->isClientTransaction()) state = controller.mClientTransactionMap.find(tid);
   else state = controller.mServerTransactionMap.find(tid);
   
   // this code makes sure that an ACK to a 200 is going to create a new
   // stateless transaction. In an ACK to a failure response, the mToTag will
   // have been set in the ServerTransaction as the 4xx passes through so it
   // will match. 
   if (state && sip && sip->isRequest() && sip->header(h_RequestLine).getMethod() == ACK)
   {
      if (sip->header(h_To).exists(p_tag) && sip->header(h_To).param(p_tag) != state->mToTag)
      {
         // Must have received an ACK to a 200;
         tid += "ack";
         if (message->isClientTransaction()) state = controller.mClientTransactionMap.find(tid);
         else state = controller.mServerTransactionMap.find(tid);
         // will be sent statelessly, if from TU
      }
   }

   if (state) // found transaction for sip msg
   {
      DebugLog (<< "Found matching transaction for " << *state);

      switch (state->mMachine)
      {
         case ClientNonInvite:
            state->processClientNonInvite(message);
            break;
         case ClientInvite:
            // ACK from TU will be Stateless
            assert (!(state->isFromTU(sip) &&  sip->isRequest() && sip->header(h_RequestLine).getMethod() == ACK));
            state->processClientInvite(message);
            break;
         case ServerNonInvite:
            state->processServerNonInvite(message);
            break;
         case ServerInvite:
            state->processServerInvite(message);
            break;
         case Stateless:
            state->processStateless(message);
            break;
         case ClientStale:
            state->processClientStale(message);
            break;
         case ServerStale:
            state->processServerStale(message);
            break;
         default:
            CritLog(<<"internal state error");
            assert(0);
            return;
      }
   }
   else if (sip)  // new transaction
   {
      if (sip->isRequest())
      {
         // create a new state object and insert in the TransactionMap
               
         if (sip->isExternal()) // new sip msg from transport
         {
            if (sip->header(h_RequestLine).getMethod() == INVITE)
            {
               // !rk! This might be needlessly created.  Design issue.
               TransactionState* state = new TransactionState(controller, ServerInvite, Trying, tid);
               state->mMsgToRetransmit = state->make100(sip);
               state->mSource = sip->getSource();
               // since we don't want to reply to the source port unless rport present 
               state->mSource.setPort(Helper::getSentPort(*sip));
               state->mState = Proceeding;
               state->mIsReliable = state->mSource.transport->isReliable();
               state->add(tid);
               
               if (Timer::T100 == 0)
               {
                  state->sendToWire(state->mMsgToRetransmit); // will get deleted when this is deleted
               }
               else
               {
                  //DebugLog(<<" adding T100 timer (INV)");
                  controller.mTimers.add(Timer::TimerTrying, tid, Timer::T100);
               }
            }
            else if (sip->header(h_RequestLine).getMethod() == CANCEL)
            {
               TransactionState* matchingInvite = controller.mServerTransactionMap.find(sip->getTransactionId());
               if (matchingInvite == 0)
               {
                  InfoLog (<< "No matching INVITE for incoming (from wire) CANCEL to uas");
                  controller.mTUFifo.add(Helper::makeResponse(*sip, 481));
                  delete sip;
                  return;
               }
               else
               {
                  assert(matchingInvite);
                  state = TransactionState::makeCancelTransaction(matchingInvite, ServerNonInvite);

                  state->mSource = sip->getSource();
                  // since we don't want to reply to the source port unless rport present 
                  state->mSource.setPort(Helper::getSentPort(*sip));
                  state->add(tid);
                  state->processReliability(matchingInvite->mTarget.getType());
                  // !jf! don't call processServerNonInvite since it will delete
                  // the sip message which needs to get sent to the TU
               }
            }
            else if (sip->header(h_RequestLine).getMethod() != ACK)
            {
               TransactionState* state = new TransactionState(controller, ServerNonInvite,Trying, tid);
               state->mSource = sip->getSource();
               // since we don't want to reply to the source port unless rport present 
               state->mSource.setPort(Helper::getSentPort(*sip));
               state->add(tid);
               state->mIsReliable = state->mSource.transport->isReliable();
            }
            

            // Incoming ACK just gets passed to the TU
            //DebugLog(<< "Adding incoming message to TU fifo " << tid);
            controller.mTUFifo.add(sip);
         }
         else // new sip msg from the TU
         {
            if (sip->header(h_RequestLine).getMethod() == INVITE)
            {
               TransactionState* state = new TransactionState(controller, ClientInvite, Calling, tid);
               state->add(state->mId);
               state->processClientInvite(sip);
            }
            else if (sip->header(h_RequestLine).getMethod() == ACK)
            {
               //TransactionState* state = new TransactionState(controller, Stateless, Calling, Data(controller.StatelessIdCounter++));
               TransactionState* state = new TransactionState(controller, Stateless, Calling, tid);
               state->add(state->mId);
               state->mController.mTimers.add(Timer::TimerStateless, state->mId, Timer::TS );
               state->processStateless(sip);
            }
            else if (sip->header(h_RequestLine).getMethod() == CANCEL)
            {
               TransactionState* matchingInvite = controller.mClientTransactionMap.find(sip->getTransactionId());
               if (matchingInvite == 0)
               {
                  InfoLog (<< "No matching INVITE for incoming (from TU) CANCEL to uac");
                  controller.mTUFifo.add(Helper::makeResponse(*sip, 481));
                  delete sip;
               }
               else if (matchingInvite->mState == Calling) // CANCEL before 1xx received
               {
                  WarningLog(<< "You can't CANCEL a request until a provisional has been received");
                  DebugLog (<< *matchingInvite);
                  DebugLog (<< *sip);

                  // !jf! the code here used to send a 200 and a 487 to the TU,
                  // if no INVITE had been sent out yet. -- i.e. dns result not
                  // processed yet 
                  delete sip;
                  assert(0);
               }
               else if (matchingInvite->mState == Completed)
               {
                  // A final response was already seen for this INVITE transaction
                  SipMessage* response200 = Helper::makeResponse(*sip, 200);
                  matchingInvite->sendToTU(response200);
                  delete sip;
               }
               else
               {
                  assert(matchingInvite);
                  state = TransactionState::makeCancelTransaction(matchingInvite, ClientNonInvite);
                  state->add(tid);
                  state->processReliability(matchingInvite->mTarget.getType());
                  state->processClientNonInvite(sip);                  
               }
            }
            else 
            {
               TransactionState* state = new TransactionState(controller, ClientNonInvite, Trying, tid);
               state->add(tid);
               state->processClientNonInvite(sip);
            }
         }
      }
      else if (sip->isResponse()) // stray response
      {
         if (controller.mDiscardStrayResponses)
         {
            DebugLog (<< "discarding stray response: " << sip->brief());
            delete message;
         }
         else
         {
            DebugLog (<< "forwarding stateless response: " << sip->brief());
            TransactionState* state = new TransactionState(controller, Stateless, Calling, Data(controller.StatelessIdCounter++));
            state->add(state->mId);
            state->mController.mTimers.add(Timer::TimerStateless, state->mId, Timer::TS );
            state->processStateless(sip);
         }
      }
      else // wasn't a request or a response
      {
         //DebugLog (<< "discarding unknown message: " << sip->brief());
      }
   } 
   else // timer or other non-sip msg
   {
      //DebugLog (<< "discarding non-sip message: " << message->brief());
      delete message;
   }
}


void
TransactionState::processStateless(Message* message)
{
   // for ACK messages from the TU, there is no transaction, send it directly
   // to the wire // rfc3261 17.1 Client Transaction
   SipMessage* sip = dynamic_cast<SipMessage*>(message);
   DebugLog (<< "TransactionState::processStateless: " << message->brief());
   
   // !jf! There is a leak for Stateless transactions associated with ACK to 200
   if (isFromTU(message))
   {
      delete mMsgToRetransmit;
      mMsgToRetransmit = sip;
      sendToWire(sip);
   }
   else if (isFromWire(message))
   {
      InfoLog (<< "Received message from wire on a stateless transaction");
      DebugLog (<< *message);
      //assert(0);
      sendToTU(sip);
   }
   else if (isTransportError(message))
   {
      processTransportFailure();
      
      delete message;
      delete this;
   }
   else if (isTimer(message))
   {
      TimerMessage* timer = dynamic_cast<TimerMessage*>(message);
      if (timer->getType() == Timer::TimerStateless)
      {
         delete message;
         delete this;
      }
   }
   else
   {
      assert(0);
   }
}

void
TransactionState::processClientNonInvite(  Message* msg )
{ 
   DebugLog (<< "TransactionState::processClientNonInvite: " << msg->brief());

   assert(!isInvite(msg));

   if (isRequest(msg) && isFromTU(msg))
   {
      //DebugLog (<< "received new non-invite request");
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      mMsgToRetransmit = sip;
      mController.mTimers.add(Timer::TimerF, mId, 64*Timer::T1 );
      sendToWire(sip);  // don't delete
   }
   else if (isResponse(msg) && isFromWire(msg)) // from the wire
   {
      //DebugLog (<< "received response from wire");

      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      int code = sip->header(h_StatusLine).responseCode();
      if (code >= 100 && code < 200) // 1XX
      {
         if (mState == Trying || mState == Proceeding)
         {
            mState = Proceeding;
            if (!mIsReliable)
            {
               mController.mTimers.add(Timer::TimerE2, mId, Timer::T2 );
            }
            sendToTU(msg); // don't delete            
         }
         else
         {
            // ignore
            delete msg;
         }
      }
      else if (code >= 200)
      {
         if (mIsReliable)
         {
            sendToTU(msg); // don't delete
            terminateClientTransaction(mId);
            delete this;
         }
         else
         {
            mState = Completed;
            mController.mTimers.add(Timer::TimerK, mId, Timer::T4 );            
            sendToTU(msg); // don't delete            
         }
      }
   }
   else if (isTimer(msg))
   {
      //DebugLog (<< "received timer in client non-invite transaction");

      TimerMessage* timer = dynamic_cast<TimerMessage*>(msg);
      switch (timer->getType())
      {
         case Timer::TimerE1:
            if (mState == Trying)
            {
               unsigned long d = timer->getDuration();
               if (d < Timer::T2) d *= 2;
               mController.mTimers.add(Timer::TimerE1, mId, d);
               DebugLog (<< "Retransmitting: " << mMsgToRetransmit->brief());
               sendToWire(mMsgToRetransmit, true);
               delete msg;
            }
            else
            {
               // ignore
               delete msg;
            }
            break;

         case Timer::TimerE2:
            if (mState == Proceeding)
            {
               mController.mTimers.add(Timer::TimerE2, mId, Timer::T2);
               DebugLog (<< "Retransmitting: " << mMsgToRetransmit->brief());
               sendToWire(mMsgToRetransmit, true);
               delete msg;
            }
            else 
            {
               // ignore
               delete msg;
            }
            break;

         case Timer::TimerF:
            // !jf! is this correct
            sendToTU(Helper::makeResponse(*mMsgToRetransmit, 408));
            terminateClientTransaction(mId);
            delete msg;
            delete this;
            break;

         case Timer::TimerK:
            terminateClientTransaction(mId);
            delete msg;
            delete this;
            break;

         default:
            //InfoLog (<< "Ignoring timer: " << *msg);
            delete msg;
            break;
      }
   }
   else if (isTransportError(msg))
   {
      processTransportFailure();
      delete msg;
   }
   else
   {
      //DebugLog (<< "TransactionState::processClientNonInvite: message unhandled");
      delete msg;
   }
}


void
TransactionState::processClientInvite(  Message* msg )
{
   DebugLog(<< "TransactionState::processClientInvite: " << msg->brief() << " " << *this);
   
   if (isRequest(msg) && isFromTU(msg))
   {
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      switch (sip->header(h_RequestLine).getMethod())
      {
         // Received INVITE request from TU="Transaction User", Start Timer B which controls
         // transaction timeouts. 
         case INVITE:
            delete mMsgToRetransmit; 
            mMsgToRetransmit = sip;
            mController.mTimers.add(Timer::TimerB, mId, 64*Timer::T1 );
            sendToWire(msg); // don't delete msg
            break;
            
         case CANCEL:
            assert(0);
            break;
            
         default:
            delete msg;
            break;
      }
   }
   else if (isResponse(msg) && isFromWire(msg))
   {
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      int code = sip->header(h_StatusLine).responseCode();
      switch (sip->header(h_CSeq).method())
      {
         case INVITE:
            /* If the client transaction receives a provisional response while in
               the "Calling" state, it transitions to the "Proceeding" state. In the
               "Proceeding" state, the client transaction SHOULD NOT retransmit the
               request any longer (this will be Handled in  "else if (isTimer(msg))")
               The Retransmissions will be stopped, Not by Cancelling Timers but
               by Ignoring the fired Timers depending upon the State which stack is in.   
            */
            if (code >= 100 && code < 200) // 1XX
            {
               if (mState == Calling || mState == Proceeding)
               {
                  mState = Proceeding;
                  sendToTU(sip); // don't delete msg
               }
               else
               {
                  delete msg;
               }
            }

            /* When in either the "Calling" or "Proceeding" states, reception of a
               2xx response MUST cause the client transaction to enter the
               "Terminated" state, and the response MUST be passed up to the TU 
               State Machine is changed to Stale since, we wanted to ensure that 
               all 2xx gets to TU
            */
            else if (code >= 200 && code < 300)
            {
               sendToTU(sip); // don't delete msg
               terminateClientTransaction(mId);
               mMachine = ClientStale;
               DebugLog (<< "Received 2xx on client invite transaction");
               DebugLog (<< *this);
               mController.mTimers.add(Timer::TimerStaleClient, mId, Timer::TS );
            }
            else if (code >= 300)
            {
               // When in either the "Calling" or "Proceeding" states, reception of a
               // response with status code from 300-699 MUST cause the client
               // transaction to transition to "Completed".
               if (mIsReliable)
               {
                  // Stack MUST pass the received response up to the TU, and the client
                  // transaction MUST generate an ACK request, even if the transport is
                  // reliable
                  SipMessage* invite = mMsgToRetransmit;
                  mMsgToRetransmit = Helper::makeFailureAck(*invite, *sip);
                  delete invite;
                  
                  // want to use the same transport as was selected for Invite
                  assert(mTarget.getType() != UNKNOWN_TRANSPORT);

                  sendToWire(mMsgToRetransmit);
                  sendToTU(msg); // don't delete msg
                  terminateClientTransaction(mId);
                  delete this;
               }
               else
               {
                  if (mState == Calling || mState == Proceeding)
                  {
                     // MUST pass the received response up to the TU, and the client
                     // transaction MUST generate an ACK request, even if the transport is
                     // reliable, if transport is Unreliable then Fire the Timer D which 
                     // take care of re-Transmission of ACK 
                     mState = Completed;
                     mController.mTimers.add(Timer::TimerD, mId, Timer::TD );
                     SipMessage* ack;
                     ack = Helper::makeFailureAck(*mMsgToRetransmit, *sip);
                     delete mMsgToRetransmit;
                     mMsgToRetransmit = ack; 
                     sendToWire(ack);
                     sendToTU(msg); // don't delete msg
                  }
                  else if (mState == Completed)
                  {
                     // Any retransmissions of the final response that
                     // are received while in the "Completed" state MUST
                     // cause the ACK to be re-passed to the transport
                     // layer for retransmission.
                     assert (mMsgToRetransmit->header(h_RequestLine).getMethod() == ACK);
                     sendToWire(mMsgToRetransmit, true);
                  }
                  else
                  {
                     /* This should never Happen if it happens we should have a plan
                        what to do here?? for now assert will work
                     */
                     CritLog(  << "State invalid");
                     // !ah! syslog
                     assert(0);
                  }
               }
            }
            break;
            
         case CANCEL:
            assert(0);
            break;

         default:
            delete msg;
            break;
      }
   }
   else if (isTimer(msg))
   {
      /* Handle Transaction Timers , Retransmission Timers which were set and Handle
         Cancellation of Timers for Re-transmissions here */

      TimerMessage* timer = dynamic_cast<TimerMessage*>(msg);
      DebugLog (<< "timer fired: " << *timer);
      
      switch (timer->getType())
      {
         case Timer::TimerA:
            if (mState == Calling)
            {
               unsigned long d = timer->getDuration();
               if (d < Timer::T2) d *= 2;

               mController.mTimers.add(Timer::TimerA, mId, d);
               InfoLog (<< "Retransmitting INVITE: " << mMsgToRetransmit->brief());
               sendToWire(mMsgToRetransmit, true);
            }
            delete msg;
            break;

         case Timer::TimerB:
            if (mState == Calling)
            {
               sendToTU(Helper::makeResponse(*mMsgToRetransmit, 408));
               terminateClientTransaction(mId);
               delete this;
            }
            delete msg;
            break;

         case Timer::TimerD:
            terminateClientTransaction(mId);
            delete msg;
            delete this;
            break;

#if 0 // !jf!
         case Timer::TimerCleanUp:
            if (!mCancelStateMachine)
            {
               // !ah! Cancelled Invite Cleanup Timer fired.
               if (mMsgToRetransmit && 
                   mMsgToRetransmit->header(h_RequestLine).getMethod() == 
                   INVITE)
               {
                  // InfoLog(<<"Making 408 for canceled invite: "<< *mMsgToRetransmit);
                  sendToTU(Helper::makeResponse(*mMsgToRetransmit, 408));
               }
               terminateClientTransaction(msg->getTransactionId());
               delete this;
            }
            else
            {
               DebugLog(<<"ugh -- cancel state machine still lives.");
            }
            break;
#endif

         default:
            delete msg;
            break;
      }
   }
   else if (isTransportError(msg))
   {
      processTransportFailure();
      delete msg;
   }
   else
   {
      //DebugLog ( << "TransactionState::processClientInvite: message unhandled");
      delete msg;
   }
}


void
TransactionState::processServerNonInvite(  Message* msg )
{
   DebugLog (<< "TransactionState::processServerNonInvite: " << msg->brief());

   if (isRequest(msg) && !isInvite(msg) && isFromWire(msg)) // from the wire
   {
      if (mState == Trying)
      {
         // ignore
         delete msg;
      }
      else if (mState == Proceeding || mState == Completed)
      {
         sendToWire(mMsgToRetransmit, true);
         delete msg;
      }
      else
      {
         CritLog (<< "Fatal error in TransactionState::processServerNonInvite " 
                  << msg->brief()
                  << " state=" << *this);
         assert(0);
         return;
      }
   }
   else if (isResponse(msg) && isFromTU(msg))
   {
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      int code = sip->header(h_StatusLine).responseCode();
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
            terminateServerTransaction(mId);
            delete this;
         }
         else
         {
            if (mState == Trying || mState == Proceeding)
            {
               mState = Completed;
               mController.mTimers.add(Timer::TimerJ, mId, 64*Timer::T1 );
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
               CritLog (<< "Fatal error in TransactionState::processServerNonInvite " 
                        << msg->brief()
                        << " state=" << *this);
               assert(0);
               return;
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
      TimerMessage* timer = dynamic_cast<TimerMessage*>(msg);
      assert(timer);
      if (mState == Completed && timer->getType() == Timer::TimerJ)
      {
         terminateServerTransaction(mId);
         delete this;
      }
      delete msg;
   }
   else if (isTransportError(msg))
   {
      processTransportFailure();
      delete msg;
   }
   else
   {
      //DebugLog (<< "TransactionState::processServerNonInvite: message unhandled");
      delete msg;
   }
}


void
TransactionState::processServerInvite(  Message* msg )
{
   DebugLog (<< "TransactionState::processServerInvite: " << msg->brief());
   if (isRequest(msg) && isFromWire(msg))
   {
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      switch (sip->header(h_RequestLine).getMethod())
      {
         case INVITE:
            if (mState == Proceeding || mState == Completed)
            {
               /*
                 The server transaction has already been constructed so this
                 message is a retransmission.  The server transaction must
                 respond with a 100 Trying _or_ the last provisional response
                 passed from the TU for this transaction.
               */
               //DebugLog (<< "Received invite from wire - forwarding to TU state=" << mState);
               if (!mMsgToRetransmit)
               {
                  mMsgToRetransmit = make100(sip); // for when TimerTrying fires
               }
               delete msg;
               sendToWire(mMsgToRetransmit);
            }
            else
            {
               //DebugLog (<< "Received invite from wire - ignoring state=" << mState);
               delete msg;
            }
            break;
            
         case ACK:
            /*
              If an ACK is received while the server transaction is in the
              "Completed" state, the server transaction MUST transition to the
              "Confirmed" state.
            */
            if (mState == Completed)
            {
               if (mIsReliable)
               {
                  //DebugLog (<< "Received ACK in Completed (reliable) - delete transaction");
                  terminateServerTransaction(mId);
                  delete this; 
                  delete msg;
               }
               else
               {
                  //DebugLog (<< "Received ACK in Completed (unreliable) - confirmed, start Timer I");
                  mState = Confirmed;
                  mController.mTimers.add(Timer::TimerI, mId, Timer::T4 );
                  delete msg;
               }
            }
            else
            {
               //DebugLog (<< "Ignore ACK not in Completed state");
               delete msg;
            }
            break;

         case CANCEL:
            assert(0);
            break;

         default:
            //DebugLog (<< "Received unexpected request. Ignoring message");
            delete msg;
            break;
      }
   }
   else if (isResponse(msg, 100, 699) && isFromTU(msg))
   {
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      int code = sip->header(h_StatusLine).responseCode();
      switch (sip->header(h_CSeq).method())
      {
         case INVITE:
            if (code == 100)
            {
               if (mState == Trying || mState == Proceeding)
               {
                  //DebugLog (<< "Received 100 in Trying or Proceeding. Send over wire");
                  delete mMsgToRetransmit; // may be replacing the 100
                  mMsgToRetransmit = sip;
                  mState = Proceeding;
                  sendToWire(msg); // don't delete msg
               }
               else
               {
                  //DebugLog (<< "Ignoring 100 - not in Trying or Proceeding.");
                  delete msg;
               }
            }
            else if (code > 100 && code < 200)
            {
               if (mState == Trying || mState == Proceeding)
               {
                  //DebugLog (<< "Received 1xx in Trying or Proceeding. Send over wire");
                  delete mMsgToRetransmit; // may be replacing the 100
                  mMsgToRetransmit = sip;
                  mState = Proceeding;
                  sendToWire(msg); // don't delete msg
               }
               else
               {
                  //DebugLog (<< "Received 100 when not in Trying State. Ignoring");
                  delete msg;
               }
            }
            else if (code >= 200 && code < 300)
            {
               if (mState == Trying || mState == Proceeding)
               {
                  DebugLog (<< "Received 2xx when in Trying or Proceeding State of server invite transaction");
                  DebugLog (<< *this);
                  sendToWire(msg);
                  
                  terminateServerTransaction(mId);
                  mMachine = ServerStale;
                  mController.mTimers.add(Timer::TimerStaleServer, mId, Timer::TS );
               }
               else
               {
                  //DebugLog (<< "Received 2xx when not in Trying or Proceeding State. Ignoring");
                  delete msg;
               }
            }
            else if (code >= 300)
            {
               /*
                 While in the "Proceeding" state, if the TU passes a response with
                 status code from 300 to 699 to the server transaction, For unreliable 
                 transports,timer G is set to fire in T1 seconds, and is not set to 
                 fire for reliable transports.when the "Completed" state is entered, 
                 timer H MUST be set to fire in 64*T1 seconds for all transports.  
                 Timer H determines when the server transaction abandons retransmitting 
                 the response
               */

               if (mState == Trying || mState == Proceeding)
               {
                  DebugLog (<< "Received failed response in Trying or Proceeding. Start Timer H, move to completed." << *this);
                  delete mMsgToRetransmit; 
                  mMsgToRetransmit = sip; 
                  mState = Completed;
                  if (sip->header(h_To).exists(p_tag))
                  {
                     mToTag = sip->header(h_To).param(p_tag);
                  }
                  mController.mTimers.add(Timer::TimerH, mId, 64*Timer::T1 );
                  if (!mIsReliable)
                  {
                     mController.mTimers.add(Timer::TimerG, mId, Timer::T1 );
                  }
                  sendToWire(msg); // don't delete msg
               }
               else
               {
                  //DebugLog (<< "Received Final response when not in Trying or Proceeding State. Ignoring");
                  delete msg;
               }
            }
            else
            {
               //DebugLog (<< "Received Invalid response line. Ignoring");
               delete msg;
            }
            break;
            
         case CANCEL:
            assert(0);
            break;
            
         default:
            //DebugLog (<< "Received response to non invite or cancel. Ignoring");
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
               DebugLog (<< "TimerG fired. retransmit, and re-add TimerG");
               sendToWire(mMsgToRetransmit, true);
               mController.mTimers.add(Timer::TimerG, mId, timer->getDuration()*2 );
            }
            else
            {
               delete msg;
            }
            break;

            /*
              If timer H fires while in the "Completed" state, it implies that the
              ACK was never received.  In this case, the server transaction MUST
              transition to the "Terminated" state, and MUST indicate to the TU
              that a transaction failure has occurred. WHY we need to inform TU
              for Failure cases ACK ? do we really need to do this ???       

              !jf! this used to re-add TimerH if there was an associated CANCEL
              transaction. Don't know why. 
            */
         case Timer::TimerH:
         case Timer::TimerI:
            if (timer->getType() == Timer::TimerH)
            {
               InfoLog (<< "No ACK was received on a server transaction (Timer H)");
            }
            terminateServerTransaction(mId);
            delete this;
            delete msg;
            break;
            
     
         case Timer::TimerTrying:
            if (mState == Trying)
            {
               //DebugLog (<< "TimerTrying fired. Send a 100");
               sendToWire(mMsgToRetransmit); // will get deleted when this is deleted
               mState = Proceeding;
               delete msg;
            }
            else
            {
               //DebugLog (<< "TimerTrying fired. Not in Trying state. Ignoring");
               delete msg;
            }
            break;
            
         default:
            CritLog(<<"unexpected timer fired: " << timer->getType());
            assert(0); // programming error if any other timer fires
            delete msg;
            break;
      }
   }
   else if (isTransportError(msg))
   {
      processTransportFailure();
      delete msg;
   }
   else
   {
      //DebugLog (<< "TransactionState::processServerInvite: message unhandled");
      delete msg;
   }
}


void
TransactionState::processClientStale(  Message* msg )
{
   DebugLog (<< "TransactionState::processClientStale: " << msg->brief());

   //SipMessage* sip = dynamic_cast<SipMessage*>(msg);
   if (isTimer(msg))
   {
      TimerMessage* timer = dynamic_cast<TimerMessage*>(msg);
      if (timer->getType() == Timer::TimerStaleClient)
      {
         delete this;
         delete msg;
      }
      else
      {
         delete msg;
      }
   }
   else if (isTransportError(msg))
   {
      WarningLog (<< "Got a transport error in Stale Client state");
      DebugLog (<< *this);
      processTransportFailure();
      delete msg;
   }
   else
   {
      assert(isResponse(msg, 200, 299));
      assert(isFromWire(msg));
      sendToTU(msg);
   }
}

void
TransactionState::processServerStale(  Message* msg )
{
   DebugLog (<< "TransactionState::processServerStale: " << msg->brief());

   //SipMessage* sip = dynamic_cast<SipMessage*>(msg);
   if (isTimer(msg))
   {
      TimerMessage* timer = dynamic_cast<TimerMessage*>(msg);
      if (timer->getType() == Timer::TimerStaleServer)
      {
         delete this;
         delete msg;
      }
      else
      {
         delete msg;
      }
   }
   else if (isTransportError(msg))
   {
      WarningLog (<< "Got a transport error in Stale Server state");
      DebugLog (<< *this);
      processTransportFailure();
      delete msg;
   }
   else
   {
      assert(isResponse(msg));
      assert(isFromTU(msg));
      sendToWire(msg); 
   }
}


void
TransactionState::processNoDnsResults()
{
   InfoLog (<< "Ran out of dns entries for " << mMsgToRetransmit->brief());
   assert(mDnsResult->available() == DnsResult::Finished);
   sendToTU(Helper::makeResponse(*mMsgToRetransmit, 503, "No DNS entries left")); // !jf! should be 480? 
   terminateClientTransaction(mId);
   delete this; 
}

void
TransactionState::processTransportFailure()
{
   InfoLog (<< "Try sending request to a different dns result");
   assert(mMsgToRetransmit);

   if (mMsgToRetransmit->isRequest() && mMsgToRetransmit->header(h_RequestLine).getMethod() == CANCEL)
   {
      WarningLog (<< "Failed to deliver a CANCEL request");
      DebugLog (<< *this);
      assert(mIsCancel);

      // In the case of a client-initiated CANCEL, we don't want to
      // try other transports in the case of transport error as the
      // CANCEL MUST be sent to the same IP/PORT as the orig. INVITE.
      sendToTU(Helper::makeResponse(*mMsgToRetransmit, 503));
      return;
   }

   assert(!mIsCancel);
   if (mDnsResult)
   {
      switch (mDnsResult->available())
      {
         case DnsResult::Available:
            mMsgToRetransmit->header(h_Vias).front().param(p_branch).incrementTransportSequence();
            mTarget = mDnsResult->next();
            processReliability(mTarget.getType());
            sendToWire(mMsgToRetransmit);
            break;
         
         case DnsResult::Pending:
            mMsgToRetransmit->header(h_Vias).front().param(p_branch).incrementTransportSequence();
            break;

         case DnsResult::Finished:
            processNoDnsResults();
            break;

         case DnsResult::Destroyed:
         default:
            InfoLog (<< "Bad state: " << *this);
            assert(0);
      }
   }
}

// called by DnsResult
void 
TransactionState::handle(DnsResult* result)
{
   // got a DNS response, so send the current message
   DebugLog (<< *this << " got DNS result: " << *result);

   if (mTarget.getType() == UNKNOWN_TRANSPORT) 
   {
      assert(mDnsResult);
      switch (mDnsResult->available())
      {
         case DnsResult::Available:
            mTarget = mDnsResult->next();
            processReliability(mTarget.getType());
            mController.mTransportSelector.transmit(mMsgToRetransmit, mTarget);
            break;
            
         case DnsResult::Finished:
            processNoDnsResults();
            break;

         case DnsResult::Pending:
            break;
            
         case DnsResult::Destroyed:
         default:
            assert(0);
            break;
      }
   }
   else
   {
      // can't be retransmission  
      sendToWire(mMsgToRetransmit, false); 
   }
}

void
TransactionState::processReliability(TransportType type)
{
   switch (type)
   {
      case UDP:
      case DCCP:
         if (mIsReliable)
         {
            mIsReliable = false;
            DebugLog (<< "Unreliable transport: " << *this);
            switch (mMachine)
            {
               case ClientNonInvite:
                  mController.mTimers.add(Timer::TimerE1, mId, Timer::T1 );
                  break;
                  
               case ClientInvite:
                  mController.mTimers.add(Timer::TimerA, mId, Timer::T1 );
                  break;

               default:
                  break;
            }
         }
         break;
         
      default:
         if (!mIsReliable)
         {
            mIsReliable = true;
         }
         break;
   }
}



void
TransactionState::sendToWire(Message* msg, bool resend) 
{
   SipMessage* sip=dynamic_cast<SipMessage*>(msg);

   if (!sip)
   {
      CritLog(<<"sendToWire: message not a sip message at address " << (void*)msg);
      assert(sip);
      return;
   }

   // !jf! for responses, go back to source always (not RFC exactly)
   if (mMachine == ServerNonInvite || mMachine == ServerInvite || mMachine == ServerStale)
   {
      assert(mDnsResult == 0);

      if (resend)
      {
         mController.mTransportSelector.retransmit(sip, mSource);
      }
      else
      {
         mController.mTransportSelector.transmit(sip, mSource);
      }
   }
   else if (mDnsResult == 0) // no dns query yet
   {
      DebugLog (<< "sendToWire with no dns result: " << *this);

      // !jf! This can only happen for a "stateless" transaction, so only send it
      // once and don't worry about failures
      if (sip->isResponse())
      {
         assert (!sip->header(h_Vias).empty());

         Via& via = sip->header(h_Vias).front();

         if (via.exists(p_received))
         {
            Tuple tuple(via.param(p_received), 
                        (via.exists(p_rport) && via.param(p_rport).hasValue()) ? via.param(p_rport).port() : via.sentPort(),
                        Tuple::toTransport(via.transport()));
            mController.mTransportSelector.transmit(sip, tuple);
         }
         else
         {
            Tuple tuple(via.sentHost(),
                        (via.exists(p_rport) && via.param(p_rport).hasValue()) ? via.param(p_rport).port() : via.sentPort(),
                        Tuple::toTransport(via.transport()));
            mController.mTransportSelector.transmit(sip, tuple);
         }
      }
      else
      {
         if (mIsCancel)
         {
            assert(mTarget.getType() != UNKNOWN_TRANSPORT);
            if (resend)
            {
               mController.mTransportSelector.retransmit(sip, mTarget);
            }
            else
            {
               mController.mTransportSelector.transmit(sip, mTarget);
            }
         }
         else
         {
            mDnsResult = mController.mTransportSelector.dnsResolve(sip, this);
            assert(mDnsResult); // !ah! is this really an assertion or an error?

            // do it now, if there is an immediate result
            if (mDnsResult->available() == DnsResult::Available)
            {
               handle(mDnsResult);
            }
         }
      }
   }
   else // reuse the last dns tuple
   {
      assert(mTarget.getType() != UNKNOWN_TRANSPORT);

      if (resend)
      {
         mController.mTransportSelector.retransmit(sip, mTarget);
      }
      else
      {
         mController.mTransportSelector.transmit(sip, mTarget);
      }
   }
}

void
TransactionState::sendToTU(Message* msg) const
{
   SipMessage* sip=dynamic_cast<SipMessage*>(msg);
   //DebugLog(<< "Send to TU: " << *msg);
   assert(sip);
   mController.mTUFifo.add(sip);
}

SipMessage*
TransactionState::make100(SipMessage* request) const
{
   SipMessage* sip=Helper::makeResponse(*request, 100);
   return sip;
}


void
TransactionState::add(const Data& tid)
{
   if (mMachine == ClientNonInvite || mMachine == ClientInvite || mMachine == ClientStale || mMachine == Stateless )
   {
      mController.mClientTransactionMap.add(tid, this);
   }
   else
   {
      mController.mServerTransactionMap.add(tid, this);
   }
}

void
TransactionState::erase(const Data& tid)
{
   if (mMachine == ClientNonInvite || mMachine == ClientInvite || mMachine == ClientStale || mMachine == Stateless)
   {
      mController.mClientTransactionMap.erase(tid);
   }
   else
   {
      mController.mServerTransactionMap.erase(tid);
   }
}


bool
TransactionState::isRequest(Message* msg) const
{
   SipMessage* sip = dynamic_cast<SipMessage*>(msg);   
   return sip && sip->isRequest();
}

bool
TransactionState::isInvite(Message* msg) const
{
   if (isRequest(msg))
   {
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      return (sip->header(h_RequestLine).getMethod()) == INVITE;
   }
   return false;
}

bool
TransactionState::isResponse(Message* msg, int lower, int upper) const
{
   SipMessage* sip = dynamic_cast<SipMessage*>(msg);
   if (sip && sip->isResponse())
   {
      int c = sip->header(h_StatusLine).responseCode();
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
TransactionState::isFromWire(Message* msg) const
{
   SipMessage* sip = dynamic_cast<SipMessage*>(msg);
   return sip && sip->isExternal();
}

bool
TransactionState::isTransportError(Message* msg) const
{
   TransportMessage* t = dynamic_cast<TransportMessage*>(msg);
   return (t && t->isFailed());
}

const Data&
TransactionState::tid(SipMessage* sip) const
{
   assert(0);
   assert (mMachine != Stateless || (mMachine == Stateless && !mId.empty()));
   assert (mMachine == Stateless || (mMachine != Stateless && sip));
   return (mId.empty() && sip) ? sip->getTransactionId() : mId;
}

void
TransactionState::terminateClientTransaction(const Data& tid)
{
   mState = Terminated;
   if (mController.mRegisteredForTransactionTermination)
   {
      //DebugLog (<< "Terminate client transaction " << tid);
      mController.mTUFifo.add(new TransactionTerminated(tid, true));
   }
}

void
TransactionState::terminateServerTransaction(const Data& tid)
{
   mState = Terminated;
   if (mController.mRegisteredForTransactionTermination)
   {
      //DebugLog (<< "Terminate server transaction " << tid);
      mController.mTUFifo.add(new TransactionTerminated(tid, false));
   }
}


std::ostream& 
resip::operator<<(std::ostream& strm, const resip::TransactionState& state)
{
   strm << "tid=" << state.mId << " [ ";
   switch (state.mMachine)
   {
      case TransactionState::ClientNonInvite:
         strm << "ClientNonInvite";
         break;
      case TransactionState::ClientInvite:
         strm << "ClientInvite";
         break;
      case TransactionState::ServerNonInvite:
         strm << "ServerNonInvite";
         break;
      case TransactionState::ServerInvite:
         strm << "ServerInvite";
         break;
      case TransactionState::Stateless:
         strm << "Stateless";
         break;
      case TransactionState::ClientStale:
         strm << "ClientStale";
         break;
      case TransactionState::ServerStale:
         strm << "ServerStale";
         break;
   }
   
   strm << "/";
   switch (state.mState)
   {
      case TransactionState::Calling:
         strm << "Calling";
         break;
      case TransactionState::Trying:
         strm << "Trying";
         break;
      case TransactionState::Proceeding:
         strm << "Proceeding";
         break;
      case TransactionState::Completed:
         strm << "Completed";
         break;
      case TransactionState::Confirmed:
         strm << "Confirmed";
         break;
      case TransactionState::Terminated:
         strm << "Terminated";
         break;
      case TransactionState::Bogus:
         strm << "Bogus";
         break;
   }
   
   strm << (state.mIsReliable ? " reliable" : " unreliable");
   strm << " target=" << state.mSource;
   strm << "]";
   return strm;
}

/* Local Variables: */
/* c-file-style: "ellemtel" */
/* End: */

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
