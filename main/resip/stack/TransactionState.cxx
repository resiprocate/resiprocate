#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/ConnectionTerminated.hxx"
#include "resip/stack/DnsInterface.hxx"
#include "resip/stack/DnsResult.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/MethodTypes.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/StatisticsManager.hxx"
#include "resip/stack/TimerMessage.hxx"
#include "resip/stack/TransactionController.hxx"
#include "resip/stack/TransactionMessage.hxx"
#include "resip/stack/TransactionState.hxx"
#include "resip/stack/TransactionTerminated.hxx"
#include "resip/stack/TransportFailure.hxx"
#include "resip/stack/TransactionUserMessage.hxx"
#include "resip/stack/TransportFailure.hxx"
#include "resip/stack/TransportSelector.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "resip/stack/TuSelector.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/MD5Stream.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Random.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "resip/stack/KeepAliveMessage.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION

unsigned long TransactionState::StatelessIdCounter = 0;

TransactionState::TransactionState(TransactionController& controller, Machine m, 
                                   State s, const Data& id, TransactionUser* tu) : 
   mController(controller),
   mMachine(m), 
   mState(s),
   mIsCancel(false),
   mIsReliable(true), // !jf! 
   mMsgToRetransmit(0),
   mDnsResult(0),
   mId(id),
   mAckIsValid(false),
   mWaitingForDnsResult(false),
   mTransactionUser(tu),
   mFailureReason(TransportFailure::None)
{
   StackLog (<< "Creating new TransactionState: " << *this);
}


TransactionState* 
TransactionState::makeCancelTransaction(TransactionState* tr, Machine machine, const Data& tid)
{
   TransactionState* cancel = new TransactionState(tr->mController, machine, Trying, 
                                                   tid, tr->mTransactionUser);
   // !jf! don't set this since it will be set by TransactionState::processReliability()
   //cancel->mIsReliable = tr->mIsReliable;  
   cancel->mResponseTarget = tr->mResponseTarget;
   cancel->mIsCancel = true;
   cancel->mTarget = tr->mTarget;
   cancel->add(tid);

   // !jf! don't call processServerNonInvite since it will delete
   // the sip message which needs to get sent to the TU
   cancel->processReliability(tr->mTarget.getType());
   return cancel;
}

bool
TransactionState::handleBadRequest(const resip::SipMessage& badReq, TransactionController& controller)
{
   assert(badReq.isRequest() && badReq.method() != ACK);
   try
   {
      SipMessage* error = Helper::makeResponse(badReq,400);
      error->header(h_StatusLine).reason()+="(" + badReq.getReason() + ")";
      Tuple target(badReq.getSource());

      if(badReq.isExternal())
      {
         controller.mTransportSelector.transmit(error,target);
         delete error;
         return true;
      }
      else
      {
         // ?bwc? Should we put together a TransactionState here so we can
         // send a 400 to the TU?
         // TODO if we send the error to the TU, don't delete the error
         delete error;
         return false;
      }
   }
   catch(resip::BaseException& e)
   {
      ErrLog(<< "Exception thrown in TransactionState::handleBadRequest."
                  " This shouldn't happen. " << e);
      return false;
   }
}

TransactionState::~TransactionState()
{
   assert(mState != Bogus);

   if (mDnsResult)
   {
      mDnsResult->destroy();
   }

   //StackLog (<< "Deleting TransactionState " << mId << " : " << this);
   erase(mId);
   
   delete mMsgToRetransmit;
   mMsgToRetransmit = 0;

   mState = Bogus;
}

void
TransactionState::process(TransactionController& controller)
{
   TransactionMessage* message = controller.mStateMacFifo.getNext();
   {
      KeepAliveMessage* keepAlive = dynamic_cast<KeepAliveMessage*>(message);
      if (keepAlive)
      {
         StackLog ( << "Sending keep alive to: " << keepAlive->getDestination());      
         controller.mTransportSelector.transmit(keepAlive, keepAlive->getDestination());
         delete keepAlive;
         return;      
      }

      ConnectionTerminated* term = dynamic_cast<ConnectionTerminated*>(message);
      if (term)
      {
         controller.mTuSelector.add(term);
         delete term;
         return;
      }
   }
   
   // .bwc. We can't do anything without a tid here. Check this first.
   Data tid;   
   try
   {
      tid = message->getTransactionId();
   }
   catch(SipMessage::Exception&)
   {
      // .bwc This is not our error. Do not ErrLog.
      DebugLog( << "TransactionState::process dropping message with invalid tid " << message->brief());
      delete message;
      return;
   }
   
   SipMessage* sip = dynamic_cast<SipMessage*>(message);
   
   if(sip)
   {
      // ?bwc? Should this come after checking for error conditions?
      if(controller.mStack.statisticsManagerEnabled() && sip->isExternal())
      {
         controller.mStatsManager.received(sip);
      }
      
      // .bwc. Check for error conditions we can respond to.
      if(sip->isRequest() && sip->method() != ACK)
      {
         if(sip->isExternal() && controller.isTUOverloaded())
         {
            SipMessage* tryLater = Helper::makeResponse(*sip, 503);
            tryLater->header(h_RetryAfter).value() = 32 + (Random::getRandom() % 32);
            tryLater->header(h_RetryAfter).comment() = "Server busy TRANS";
            Tuple target(sip->getSource());
            delete sip;
            controller.mTransportSelector.transmit(tryLater, target);
            delete tryLater;
            return;
         }
         
         if(sip->isInvalid())
         {
            handleBadRequest(*sip,controller);
            delete sip;
            return;
         }         
      }

#ifdef PEDANTIC_STACK
      try
      {
         sip->parseAllHeaders();
      }
      catch(resip::ParseException& e)
      {
         if(sip->isRequest() && sip->method()!=ACK)
         {
            handleBadRequest(*sip,controller);
         }
         
         InfoLog(<< "Exception caught by pedantic stack: " << e);
      }
#endif      
      
      // This ensures that CANCEL requests form unique transactions
      if (sip->method() == CANCEL) 
      {
         tid += "cancel";
      }
   }
      
   TransactionState* state = 0;
   if (message->isClientTransaction()) state = controller.mClientTransactionMap.find(tid);
   else state = controller.mServerTransactionMap.find(tid);
   
   // .bwc. This code ensures that the transaction state-machine can recover
   // from ACK/200 with the same tid as the original INVITE. This problem is
   // stupidly common. 
   if (state && sip && sip->isExternal() && sip->isRequest() && sip->method() == ACK)
   {
      if (!state->mAckIsValid)
      {
         // Must have received an ACK to a 200;
         // We will never respond to this, so nothing will need this tid for
         // driving transaction state. Additionally, 
         InfoLog(<<"Someone sent us an ACK/200 with the same tid as the "
                     "original INVITE. This is bad behavior, and should be "
                     "corrected in the client.");
         sip->mIsBadAck200=true;
         // .bwc. This is a new stateless transaction, despite its tid.
         state=0;
      }
   }

   if (state) // found transaction for sip msg
   {
      StackLog (<< "Found matching transaction for " << message->brief() << " -> " << *state);

      switch (state->mMachine)
      {
         case ClientNonInvite:
            state->processClientNonInvite(message);
            break;
         case ClientInvite:
            // ACK from TU will be Stateless
            assert (!(state->isFromTU(sip) &&  sip->isRequest() && sip->method() == ACK));
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
      StackLog (<< "No matching transaction for " << sip->brief());
      TransactionUser* tu = 0;      
      if (sip->isExternal())
      {
         if (controller.mTuSelector.haveTransactionUsers() && sip->isRequest())
         {
            tu = controller.mTuSelector.selectTransactionUser(*sip);
            if (!tu)
            {
               //InfoLog (<< "Didn't find a TU for " << sip->brief());
               // !bwc! We really should do something other than a 500 here.
               // If none of the TUs liked the request because of the Request-
               // Uri scheme, we should be returning a 416, for example.
               InfoLog( << "No TU found for message: " << sip->brief());               
               SipMessage* noMatch = Helper::makeResponse(*sip, 500);
               Tuple target(sip->getSource());
               delete sip;
               controller.mTransportSelector.transmit(noMatch, target);
               delete noMatch;
               return;
            }
            else
            {
               //InfoLog (<< "Found TU for " << sip->brief());
            }
         }
      }
      else
      {
         tu = sip->getTransactionUser();
         if (!tu)
         {
            //InfoLog (<< "No TU associated with " << sip->brief());
         }
      }
               
      if (sip->isRequest())
      {
         // create a new state object and insert in the TransactionMap
               
         if (sip->isExternal()) // new sip msg from transport
         {
            if (sip->method() == INVITE)
            {
               // !rk! This might be needlessly created.  Design issue.
               TransactionState* state = new TransactionState(controller, ServerInvite, Trying, tid, tu);
               state->mMsgToRetransmit = state->make100(sip);
               state->mResponseTarget = sip->getSource(); // UACs source address
               // since we don't want to reply to the source port if rport present 
               state->mResponseTarget.setPort(Helper::getPortForReply(*sip));
               state->mIsReliable = isReliable(state->mResponseTarget.getType());
               state->add(tid);
               
               if (Timer::T100 == 0)
               {
                  state->sendToWire(state->mMsgToRetransmit); // will get deleted when this is deleted
                  state->mState = Proceeding;
               }
               else
               {
                  //StackLog(<<" adding T100 timer (INV)");
                  controller.mTimers.add(Timer::TimerTrying, tid, Timer::T100);
               }
            }
            else if (sip->method() == CANCEL)
            {
               TransactionState* matchingInvite = 
                  controller.mServerTransactionMap.find(sip->getTransactionId());
               if (matchingInvite == 0)
               {
                  InfoLog (<< "No matching INVITE for incoming (from wire) CANCEL to uas");
                  //was TransactionState::sendToTU(tu, controller, Helper::makeResponse(*sip, 481));
                  SipMessage* response = Helper::makeResponse(*sip, 481);
                  Tuple target(sip->getSource());
                  controller.mTransportSelector.transmit(response, target);
                  delete sip;
                  delete response;
                  return;
               }
               else
               {
                  assert(matchingInvite);
                  state = TransactionState::makeCancelTransaction(matchingInvite, ServerNonInvite, tid);
                  state->startServerNonInviteTimerTrying(*sip,tid);
               }
            }
            else if (sip->method() != ACK)
            {
               TransactionState* state = new TransactionState(controller, ServerNonInvite,Trying, tid, tu);
               state->mResponseTarget = sip->getSource();
               // since we don't want to reply to the source port if rport present 
               state->mResponseTarget.setPort(Helper::getPortForReply(*sip));
               state->add(tid);
               state->mIsReliable = isReliable(state->mResponseTarget.getType());
               state->startServerNonInviteTimerTrying(*sip,tid);
            }
            
            // Incoming ACK just gets passed to the TU
            //StackLog(<< "Adding incoming message to TU fifo " << tid);
            TransactionState::sendToTU(tu, controller, sip);
         }
         else // new sip msg from the TU
         {
            if (sip->method() == INVITE)
            {
               TransactionState* state = new TransactionState(controller, ClientInvite, Calling, tid, tu);
               state->add(state->mId);
               state->processClientInvite(sip);
            }
            else if (sip->method() == ACK)
            {
               //TransactionState* state = new TransactionState(controller, Stateless, Calling, Data(StatelessIdCounter++));
               TransactionState* state = new TransactionState(controller, Stateless, Calling, tid, tu);
               state->add(state->mId);
               state->mController.mTimers.add(Timer::TimerStateless, state->mId, Timer::TS );
               state->processStateless(sip);
            }
            else if (sip->method() == CANCEL)
            {
               TransactionState* matchingInvite = controller.mClientTransactionMap.find(sip->getTransactionId());
               if (matchingInvite == 0)
               {
                  InfoLog (<< "No matching INVITE for incoming (from TU) CANCEL to uac");
                  TransactionState::sendToTU(tu, controller, Helper::makeResponse(*sip,481));
                  delete sip;
               }
               else if (matchingInvite->mState == Calling) // CANCEL before 1xx received
               {
                  WarningLog(<< "You can't CANCEL a request until a provisional has been received");
                  StackLog (<< *matchingInvite);
                  StackLog (<< *sip);

                  // if no INVITE had been sent out yet. -- i.e. dns result not
                  // processed yet 

                  // The CANCEL was received before the INVITE was sent
                  // This can happen in odd cases. Too common to assert.
                  // Be graceful.
                  TransactionState::sendToTU(tu, controller, Helper::makeResponse(*sip, 200));
                  matchingInvite->sendToTU(Helper::makeResponse(*matchingInvite->mMsgToRetransmit, 487));
                  matchingInvite->terminateClientTransaction(matchingInvite->mId);

                  delete matchingInvite;
                  delete sip;

               }
               else if (matchingInvite->mState == Completed)
               {
                  // A final response was already seen for this INVITE transaction
                  matchingInvite->sendToTU(Helper::makeResponse(*sip, 200));
                  delete sip;
               }
               else
               {
                  assert(matchingInvite);
                  state = TransactionState::makeCancelTransaction(matchingInvite, ClientNonInvite, tid);
                  state->processClientNonInvite(sip);
                  
                  // for the INVITE in case we never get a 487
                  matchingInvite->mController.mTimers.add(Timer::TimerCleanUp, sip->getTransactionId(), 128*Timer::T1);
               }
            }
            else 
            {
               TransactionState* state = new TransactionState(controller, ClientNonInvite, 
                                                              Trying, tid, tu);
               state->add(tid);
               state->processClientNonInvite(sip);
            }
         }
      }
      else if (sip->isResponse()) // stray response
      {
         if (controller.mDiscardStrayResponses)
         {
            InfoLog (<< "discarding stray response: " << sip->brief());
            delete message;
         }
         else
         {
            StackLog (<< "forwarding stateless response: " << sip->brief());
            TransactionState* state = 
               new TransactionState(controller, Stateless, Calling, 
                                    Data(StatelessIdCounter++), tu);
            state->add(state->mId);
            state->mController.mTimers.add(Timer::TimerStateless, state->mId, Timer::TS );
            state->processStateless(sip);
         }
      }
      else // wasn't a request or a response
      {
         //StackLog (<< "discarding unknown message: " << sip->brief());
      }
   } 
   else // timer or other non-sip msg
   {
      //StackLog (<< "discarding non-sip message: " << message->brief());
      delete message;
   }
}

void
TransactionState::startServerNonInviteTimerTrying(SipMessage& sip, Data& tid)
{
   unsigned int duration = 3500;
   if(Timer::T1 != 500) // optimzed for T1 == 500
   {
      // Iteratively calculate how much time before TimerE reaches T2 (RFC4320) - could be improved
      duration = Timer::T1;
      while(duration*2<Timer::T2) duration = duration * 2;
   }
   mMsgToRetransmit = make100(&sip);  // Store for use when timer expires
   mController.mTimers.add(Timer::TimerTrying, tid, duration );  // Start trying timer so that we can send 100 to NITs as recommened in RFC4320
}

void
TransactionState::processStateless(TransactionMessage* message)
{
   // for ACK messages from the TU, there is no transaction, send it directly
   // to the wire // rfc3261 17.1 Client Transaction
   SipMessage* sip = dynamic_cast<SipMessage*>(message);
   StackLog (<< "TransactionState::processStateless: " << message->brief());
   
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
      StackLog (<< *message);
      //assert(0);
      sendToTU(sip);
   }
   else if (isTransportError(message))
   {
      processTransportFailure(message);
      
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
TransactionState::saveOriginalContactAndVia(const SipMessage& sip)
{
   if(sip.exists(h_Contacts) && sip.header(h_Contacts).size() == 1)
   {
      mOriginalContact = std::auto_ptr<NameAddr>(new NameAddr(sip.header(h_Contacts).front()));
   }
   mOriginalVia = std::auto_ptr<Via>(new Via(sip.header(h_Vias).front()));
}

void TransactionState::restoreOriginalContactAndVia()
{
   if (mOriginalContact.get())
   {
      mMsgToRetransmit->header(h_Contacts).front() = *mOriginalContact;
   }                  
   if (mOriginalVia.get())
   {
      mOriginalVia->param(p_branch).incrementTransportSequence();
      mMsgToRetransmit->header(h_Vias).front() = *mOriginalVia;
   }
}

void
TransactionState::processClientNonInvite(TransactionMessage* msg)
{ 
   StackLog (<< "TransactionState::processClientNonInvite: " << msg->brief());

   assert(!isInvite(msg));

   if (isRequest(msg) && isFromTU(msg))
   {
      //StackLog (<< "received new non-invite request");
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      delete mMsgToRetransmit;
      saveOriginalContactAndVia(*sip);
      mMsgToRetransmit = sip;
      mController.mTimers.add(Timer::TimerF, mId, Timer::TF);
      sendToWire(sip);  // don't delete
   }
   else if (isResponse(msg) && isFromWire(msg)) // from the wire
   {
      //StackLog (<< "received response from wire");

      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      int code = sip->header(h_StatusLine).responseCode();
      if (code >= 100 && code < 200) // 1XX
      {
         if (mState == Trying || mState == Proceeding)
         {
            //?slg? if we set the timer in Proceeding, then every 1xx response will cause another TimerE2 to be set and many retransmissions will occur - which is not correct
            // Should we restart the E2 timer though?  If so, we need to use somekind of timer sequence number so that previous E2 timers get discarded.
            if (!mIsReliable && mState == Trying)
            {
               mController.mTimers.add(Timer::TimerE2, mId, Timer::T2 );
            }
            mState = Proceeding;
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
         // don't notify the TU of retransmissions
         if (mState == Trying || mState == Proceeding)
         {
            sendToTU(msg); // don't delete
         }
         else if (mState == Completed)
         {
            delete msg;
         }
         
         if (mIsReliable)
         {
            terminateClientTransaction(mId);
            delete this;
         }
         else if (mState != Completed) // prevent TimerK reproduced
         {
            mState = Completed;
            mController.mTimers.add(Timer::TimerK, mId, Timer::T4 );            
         }
      }
   }
   else if (isTimer(msg))
   {
      //StackLog (<< "received timer in client non-invite transaction");

      TimerMessage* timer = dynamic_cast<TimerMessage*>(msg);
      switch (timer->getType())
      {
         case Timer::TimerE1:
            if (mState == Trying)
            {
               unsigned long d = timer->getDuration();
               if (d < Timer::T2) d *= 2;
               mController.mTimers.add(Timer::TimerE1, mId, d);
               StackLog (<< "Retransmitting: " << mMsgToRetransmit->brief());
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
               StackLog (<< "Retransmitting: " << mMsgToRetransmit->brief());
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
            if (mState == Trying || mState == Proceeding)
            {
               sendToTU(Helper::makeResponse(*mMsgToRetransmit, 408));
               terminateClientTransaction(mId);
               delete this;
            }
            
            delete msg;
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
      processTransportFailure(msg);
      delete msg;
   }
   else
   {
      //StackLog (<< "TransactionState::processClientNonInvite: message unhandled");
      delete msg;
   }
}

void
TransactionState::processClientInvite(TransactionMessage* msg)
{
   StackLog(<< "TransactionState::processClientInvite: " << msg->brief() << " " << *this);
   if (isRequest(msg) && isFromTU(msg))
   {
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      switch (sip->method())
      {
         // Received INVITE request from TU="Transaction User", Start Timer B which controls
         // transaction timeouts. 
         case INVITE:
            delete mMsgToRetransmit; 
            saveOriginalContactAndVia(*sip);
            mMsgToRetransmit = sip;
            mController.mTimers.add(Timer::TimerB, mId, Timer::TB);
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
      switch (sip->method())
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
               //terminateClientTransaction(mId);
               mMachine = ClientStale;
               StackLog (<< "Received 2xx on client invite transaction");
               StackLog (<< *this);
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
                     assert (mMsgToRetransmit->method() == ACK);
                     sendToWire(mMsgToRetransmit, true);
                     delete msg;
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
      StackLog (<< "timer fired: " << *timer);
      
      switch (timer->getType())
      {
         case Timer::TimerA:
            if (mState == Calling)
            {
               unsigned long d = timer->getDuration()*2;
               // TimerA is supposed to double with each retransmit RFC3261 17.1.1          

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

         case Timer::TimerCleanUp:
            // !ah! Cancelled Invite Cleanup Timer fired.
            StackLog (<< "Timer::TimerCleanUp: " << *this << std::endl << *mMsgToRetransmit);
            if (mState == Proceeding)
            {
               assert(mMsgToRetransmit && mMsgToRetransmit->method() == INVITE);
               InfoLog(<<"Making 408 for canceled invite that received no response: "<< mMsgToRetransmit->brief());
               sendToTU(Helper::makeResponse(*mMsgToRetransmit, 408));
               terminateClientTransaction(msg->getTransactionId());
               delete this;
            }
            delete msg;
            break;

         default:
            delete msg;
            break;
      }
   }
   else if (isTransportError(msg))
   {
      processTransportFailure(msg);
      delete msg;
   }
   else
   {
      //StackLog ( << "TransactionState::processClientInvite: message unhandled");
      delete msg;
   }
}


void
TransactionState::processServerNonInvite(TransactionMessage* msg)
{
   StackLog (<< "TransactionState::processServerNonInvite: " << msg->brief());

   if (isRequest(msg) && !isInvite(msg) && isFromWire(msg)) // retransmission from the wire
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
            delete mMsgToRetransmit;
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
               delete mMsgToRetransmit;
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
      switch (timer->getType())
      {
         case Timer::TimerJ:
            if (mState == Completed)
            {
               terminateServerTransaction(mId);
               delete this;
            }
            delete msg;
            break;

         case Timer::TimerTrying:
            if (mState == Trying)
            {
               // Timer E has reached T2 - send a 100 as recommended by RFC4320 NIT-Problem-Actions
               sendToWire(mMsgToRetransmit);
               mState = Proceeding;
            }
            delete msg;
            break;

         default:
            delete msg;
            break;
      }
   }
   else if (isTransportError(msg))
   {
      processTransportFailure(msg);
      delete msg;
   }
   else
   {
      //StackLog (<< "TransactionState::processServerNonInvite: message unhandled");
      delete msg;
   }
}


void
TransactionState::processServerInvite(TransactionMessage* msg)
{
   StackLog (<< "TransactionState::processServerInvite: " << msg->brief());
   if (isRequest(msg) && isFromWire(msg))
   {
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      switch (sip->method())
      {
         case INVITE:
            // note: handling of initial INVITE message is done in TransactionState:process
            if (mState == Proceeding || mState == Completed)
            {
               /*
                 The server transaction has already been constructed so this
                 message is a retransmission.  The server transaction must
                 respond with a 100 Trying _or_ the last provisional response
                 passed from the TU for this transaction.
               */
               //StackLog (<< "Received invite from wire - forwarding to TU state=" << mState);
               if (!mMsgToRetransmit)
               {
                  mMsgToRetransmit = make100(sip);
               }
               delete msg;
               sendToWire(mMsgToRetransmit);
            }
            else
            {
               //StackLog (<< "Received invite from wire - ignoring state=" << mState);
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
                  //StackLog (<< "Received ACK in Completed (reliable) - delete transaction");
                  terminateServerTransaction(mId);
                  delete this; 
                  delete msg;
               }
               else
               {
                  //StackLog (<< "Received ACK in Completed (unreliable) - confirmed, start Timer I");
                  mState = Confirmed;
                  mController.mTimers.add(Timer::TimerI, mId, Timer::T4 );
                  delete msg;
               }
            }
            else
            {
               //StackLog (<< "Ignore ACK not in Completed state");
               delete msg;
            }
            break;

         case CANCEL:
            assert(0);
            break;

         default:
            //StackLog (<< "Received unexpected request. Ignoring message");
            delete msg;
            break;
      }
   }
   else if (isResponse(msg, 100, 699) && isFromTU(msg))
   {
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      int code = sip->header(h_StatusLine).responseCode();
      switch (sip->method())
      {
         case INVITE:
            if (code == 100)
            {
               if (mState == Trying || mState == Proceeding)
               {
                  //StackLog (<< "Received 100 in Trying or Proceeding. Send over wire");
                  delete mMsgToRetransmit; // may be replacing the 100
                  mMsgToRetransmit = sip;
                  mState = Proceeding;
                  sendToWire(msg); // don't delete msg
               }
               else
               {
                  //StackLog (<< "Ignoring 100 - not in Trying or Proceeding.");
                  delete msg;
               }
            }
            else if (code > 100 && code < 200)
            {
               if (mState == Trying || mState == Proceeding)
               {
                  //StackLog (<< "Received 1xx in Trying or Proceeding. Send over wire");
                  delete mMsgToRetransmit; // may be replacing the 100
                  mMsgToRetransmit = sip;
                  mState = Proceeding;
                  sendToWire(msg); // don't delete msg
               }
               else
               {
                  //StackLog (<< "Received 100 when not in Trying State. Ignoring");
                  delete msg;
               }
            }
            else if (code >= 200 && code < 300)
            {
               if (mState == Trying || mState == Proceeding)
               {
                  StackLog (<< "Received 2xx when in Trying or Proceeding State of server invite transaction");
                  StackLog (<< *this);
                  sendToWire(msg);
                  
                  // Keep the StaleServer transaction around, so we can keep the
                  // source Tuple that the request was received on. 
                  //terminateServerTransaction(mId);
                  mMachine = ServerStale;
                  mController.mTimers.add(Timer::TimerStaleServer, mId, Timer::TS );
                  delete msg;
               }
               else
               {
                  //StackLog (<< "Received 2xx when not in Trying or Proceeding State. Ignoring");
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
                  mAckIsValid=true;
                  StackLog (<< "Received failed response in Trying or Proceeding. Start Timer H, move to completed." << *this);
                  delete mMsgToRetransmit; 
                  mMsgToRetransmit = sip; 
                  mState = Completed;                     
                  mController.mTimers.add(Timer::TimerH, mId, Timer::TH );
                  if (!mIsReliable)
                  {
                     mController.mTimers.add(Timer::TimerG, mId, Timer::T1 );
                  }
                  sendToWire(msg); // don't delete msg
               }
               else
               {
                  //StackLog (<< "Received Final response when not in Trying or Proceeding State. Ignoring");
                  delete msg;
               }
            }
            else
            {
               //StackLog (<< "Received Invalid response line. Ignoring");
               delete msg;
            }
            break;
            
         case CANCEL:
            assert(0);
            break;
            
         default:
            //StackLog (<< "Received response to non invite or cancel. Ignoring");
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
               StackLog (<< "TimerG fired. retransmit, and re-add TimerG");
               sendToWire(mMsgToRetransmit, true);
               mController.mTimers.add(Timer::TimerG, mId, resipMin(Timer::T2, timer->getDuration()*2) );  //  TimerG is supposed to double - up until a max of T2 RFC3261 17.2.1
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
            break;

         case Timer::TimerTrying:
            if (mState == Trying)
            {
               //StackLog (<< "TimerTrying fired. Send a 100");
               sendToWire(mMsgToRetransmit); // will get deleted when this is deleted
               mState = Proceeding;
            }
            else
            {
               //StackLog (<< "TimerTrying fired. Not in Trying state. Ignoring");
            }
            break;
            
         default:
            CritLog(<<"unexpected timer fired: " << timer->getType());
            assert(0); // programming error if any other timer fires
            break;
      }
      delete timer;
   }
   else if (isTransportError(msg))
   {
      processTransportFailure(msg);
      delete msg;
   }
   else
   {
      //StackLog (<< "TransactionState::processServerInvite: message unhandled");
      delete msg;
   }
}


void
TransactionState::processClientStale(TransactionMessage* msg)
{
   StackLog (<< "TransactionState::processClientStale: " << msg->brief());

   if (isTimer(msg))
   {
      TimerMessage* timer = dynamic_cast<TimerMessage*>(msg);
      if (timer->getType() == Timer::TimerStaleClient)
      {
         terminateClientTransaction(mId);
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
      StackLog (<< *this);
      processTransportFailure(msg);
      delete msg;
   }
   else
   {
      if(isResponse(msg, 200, 299))
      {
         assert(isFromWire(msg));
         sendToTU(msg);
      }
      else
      {
         // might have received some other response because a downstream UAS is
         // misbehaving. For instance, sending a 487/INVITE after already
         // sending a 200/INVITE. In this case, discard the response
         StackLog (<< "Discarding extra response: " << *msg);
         delete msg;
      }
   }
}

void
TransactionState::processServerStale(TransactionMessage* msg)
{
   StackLog (<< "TransactionState::processServerStale: " << msg->brief());

   SipMessage* sip = dynamic_cast<SipMessage*>(msg);
   if (isTimer(msg))
   {
      TimerMessage* timer = dynamic_cast<TimerMessage*>(msg);
      if (timer->getType() == Timer::TimerStaleServer)
      {
         delete msg;
         terminateServerTransaction(mId);
         delete this;
      }
      else
      {
         delete msg;
      }
   }
   else if (isTransportError(msg))
   {
      WarningLog (<< "Got a transport error in Stale Server state");
      StackLog (<< *this);
      processTransportFailure(msg);
      delete msg;
   }
   else if (sip && isRequest(sip) && sip->method() == ACK)
   {
      // .bwc. We should never fall into this block. There is code in process
      // that should prevent it.
      assert(isFromWire(msg));
      InfoLog (<< "Passing ACK directly to TU: " << sip->brief());
      sendToTU(msg);
   }
   else if (sip && isRequest(sip) && sip->method() == INVITE)
   {
      // this can happen when an upstream UAC never received the 200 and
      // retransmits the INVITE when using unreliable transport
      // Drop the INVITE since the 200 will get retransmitted by the downstream UAS
      StackLog (<< "Dropping retransmitted INVITE in stale server transaction" << sip->brief());
      delete msg;
   }
   else if (isResponse(msg) && isFromTU(msg))
   {
      sendToWire(msg); 
      delete msg;
   }
   else
   {
      // .bwc. This can very easily be triggered by a stupid/malicious 
      // endpoint. This is not an error in our code. Do not ErrLog this.
      InfoLog(<<"ServerStale unexpected condition, dropping message.");
      if (sip)
      {
         InfoLog(<<sip->brief());
      }
      delete msg;
   }
}


void
TransactionState::processNoDnsResults()
{
   InfoLog (<< "Ran out of dns entries for " << mDnsResult->target() << ". Send 503");
   assert(mDnsResult->available() == DnsResult::Finished);
   SipMessage* response = Helper::makeResponse(*mMsgToRetransmit, 503);
   WarningCategory warning;
   warning.hostname() = DnsUtil::getLocalHostName();
   warning.code() = 499;
   switch(mFailureReason)
   {
      warning.text() = "No other DNS entries to try";
      case TransportFailure::None:
      case TransportFailure::Failure:
         break;
      case TransportFailure::NoTransport:
         response->header(h_StatusLine).reason() = "No matching transport found";
         break;
      case TransportFailure::NoRoute:
         response->header(h_StatusLine).reason() = "No route to host";
         break;
      case TransportFailure::CertNameMismatch:
         response->header(h_StatusLine).reason() = "Certificate Name Mismatch";
         break;
      case TransportFailure::CertValidationFailure:
         response->header(h_StatusLine).reason() = "Certificate Validation Failure";
   }
         
   response->header(h_Warnings).push_back(warning);

   sendToTU(response); // !jf! should be 480? 
   terminateClientTransaction(mId);
   if (mMachine != Stateless)
   {
	   delete this; 
   }
}

void
TransactionState::processTransportFailure(TransactionMessage* msg)
{
   TransportFailure* failure = dynamic_cast<TransportFailure*>(msg);
   assert(failure);
   assert(mState!=Bogus);

   // .bwc. We should only try multiple dns results if we are originating a
   // request. Additionally, there are (potential) cases where it would not
   // be appropriate to fail over even then.
   bool shouldFailover=false;
   if(mMachine==ClientNonInvite)
   {
      if(mState==Completed || mState==Terminated)
      {
         WarningLog(<<"Got a TransportFailure message in a " << mState <<
         " ClientNonInvite transaction. How did this happen? Since we have"
         " already completed the transaction, we shouldn't try"
         " additional DNS results.");
      }
      else
      {
         shouldFailover=true;
      }
   }
   else if(mMachine==ClientInvite)
   {
      if(mState==Completed || mState==Terminated)
      {
         // .bwc. Perhaps the attempted transmission of the ACK failed here.
         // (assuming this transaction got a failure response; not sure what
         // might have happened if this is not the case)
         // In any case, we should not try sending the INVITE anywhere else.
         InfoLog(<<"Got a TransportFailure message in a " << mState <<
         " ClientInvite transaction. Since we have"
         " already completed the transaction, we shouldn't try"
         " additional DNS results.");
      }
      else
      {
         if(mState==Proceeding)
         {
            // .bwc. We need to revert our state back to Calling, since we are
            // going to be sending the INVITE to a new endpoint entirely.

            // !bwc!
            // An interesting consequence occurs if our failover ultimately
            // sends to the same instance of a resip stack; we increment the 
            // transport sequence in our branch parameter, but any resip-based
            // stack will ignore this change, and process this "new" request as
            // a retransmission! Furthermore, our state will be out of phase
            // with the state at the remote endpoint, and if we have sent a
            // PRACK, it will know (and stuff will break)!
            // TODO What else needs to be done here to safely revert our state?
            mState=Calling;
         }
         shouldFailover=true;
      }
   }

   if(mDnsResult)
   {
      // .bwc. Greylist for 32s
      // !bwc! TODO make this duration configurable.
      mDnsResult->greylistLast(Timer::getTimeMs()+32000);
   }
   
   
   if(shouldFailover)
   {
      InfoLog (<< "Try sending request to a different dns result");
      assert(mMsgToRetransmit);
      if (failure->getFailureReason() > mFailureReason)
      {
         mFailureReason = failure->getFailureReason();
      }
      
      if (mMsgToRetransmit->isRequest() && mMsgToRetransmit->method() == CANCEL)
      {
         WarningLog (<< "Failed to deliver a CANCEL request");
         StackLog (<< *this);
         assert(mIsCancel);

         // In the case of a client-initiated CANCEL, we don't want to
         // try other transports in the case of transport error as the
         // CANCEL MUST be sent to the same IP/PORT as the orig. INVITE.
         //?dcm? insepct failure enum?
         SipMessage* response = Helper::makeResponse(*mMsgToRetransmit, 503);
         WarningCategory warning;
         warning.hostname() = DnsUtil::getLocalHostName();
         warning.code() = 499;
         warning.text() = "Failed to deliver CANCEL using the same transport as the INVITE was used";
         response->header(h_Warnings).push_back(warning);
         
         sendToTU(response);
         return;
      }

      assert(!mIsCancel);
      if (mDnsResult)
      {      
         switch (mDnsResult->available())
         {
            case DnsResult::Available:
               restoreOriginalContactAndVia();
               mTarget = mDnsResult->next();
               processReliability(mTarget.getType());
               sendToWire(mMsgToRetransmit);
               break;
            
            case DnsResult::Pending:
               mWaitingForDnsResult=true;
               restoreOriginalContactAndVia();
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
}

// called by DnsResult
void
TransactionState::rewriteRequest(const Uri& rewrite)
{
   // !bwc! TODO We need to address the race-conditions caused by callbacks
   // into a class whose thread-safety is accomplished through message-passing.
   // This function could very easily be called while other processing is
   // taking place due to a message from the state-machine fifo. In the end, I
   // imagine that we will need to have the callback place a message onto the
   // queue, and move all the code below into a function that handles that
   // message.

   assert(mMsgToRetransmit->isRequest());
   if (mMsgToRetransmit->header(h_RequestLine).uri() != rewrite)
   {
      InfoLog (<< "Rewriting request-uri to " << rewrite);
      mMsgToRetransmit->header(h_RequestLine).uri() = rewrite;
   }
}

void 
TransactionState::handle(DnsResult* result)
{
   // !bwc! TODO We need to address the race-conditions caused by callbacks
   // into a class whose thread-safety is accomplished through message-passing.
   // This function could very easily be called while other processing is
   // taking place due to a message from the state-machine fifo. In the end, I
   // imagine that we will need to have the callback place a message onto the
   // queue, and move all the code below into a function that handles that
   // message.

   // got a DNS response, so send the current message
   StackLog (<< *this << " got DNS result: " << *result);
   
   // .bwc. Were we expecting something from mDnsResult?
   if (mWaitingForDnsResult) 
   {
      assert(mDnsResult);
      switch (mDnsResult->available())
      {
         case DnsResult::Available:
            mWaitingForDnsResult=false;
            mTarget = mDnsResult->next();
            processReliability(mTarget.getType());
            mController.mTransportSelector.transmit(mMsgToRetransmit, mTarget);
            break;
            
         case DnsResult::Finished:
            mWaitingForDnsResult=false;
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
            StackLog (<< "Unreliable transport: " << *this);
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

// !ah! only used one place, so leaving it here instead of making a helper.
// !ah! broken out for clarity -- only used for forceTargets.
// Expects that host portion is IP address notation.

static const Tuple
simpleTupleForUri(const Uri& uri)
{
   const Data& host = uri.host();
   int port = uri.port();

   resip::TransportType transport = UNKNOWN_TRANSPORT;
 
  if (uri.exists(p_transport))
   {
      transport = Tuple::toTransport(uri.param(p_transport));
   }

   if (transport == UNKNOWN_TRANSPORT)
   {
      transport = UDP;
   }
   if (port == 0)
   {
      switch(transport)
      {
         case TLS:
            port = Symbols::DefaultSipsPort;
            break;
         case UDP:
         case TCP:
         default:
            port = Symbols::DefaultSipPort;
            break;
         // !ah! SCTP?

      }
   }

   return Tuple(host,port,transport);
}

void
TransactionState::sendToWire(TransactionMessage* msg, bool resend) 
{
   SipMessage* sip = dynamic_cast<SipMessage*>(msg);

   if (!sip)
   {
      CritLog(<<"sendToWire: message not a sip message at address " << (void*)msg);
      assert(sip);
      return;
   }

   if(mController.mStack.statisticsManagerEnabled())
   {
      mController.mStatsManager.sent(sip, resend);
   }

   // !jf! for responses, go back to source always (not RFC exactly)
   if (mMachine == ServerNonInvite || mMachine == ServerInvite || mMachine == ServerStale)
   {
      assert(mDnsResult == 0);
      assert(sip->exists(h_Vias));
      assert(!sip->header(h_Vias).empty());

      Tuple target(mResponseTarget);
      if (sip->hasForceTarget())
      {
         target = simpleTupleForUri(sip->getForceTarget());
         StackLog(<<"!ah! response with force target going to : "<<target);
      }
      else if (sip->header(h_Vias).front().exists(p_rport) && sip->header(h_Vias).front().param(p_rport).hasValue())
      {
         target.setPort(sip->header(h_Vias).front().param(p_rport).port());
         StackLog(<< "rport present in response, sending to " << target);
      }
      else
      {
         StackLog(<< "tid=" << sip->getTransactionId() << " sending to : " << target);
      }

      if (resend)
      {
         mController.mTransportSelector.retransmit(sip, target);
      }
      else
      {
         mController.mTransportSelector.transmit(sip, target);
      }
   }
   else if (sip->getDestination().mFlowKey)
   {
      // !bwc! We have the FlowKey. This completely specifies our Transport
      // (and Connection, if applicable)
      DebugLog(<< "Sending to tuple: " << sip->getDestination());
      mTarget = sip->getDestination();
      processReliability(mTarget.getType());
      mController.mTransportSelector.transmit(sip, mTarget); // dns not used
   }
   else if (mDnsResult == 0 && !mIsCancel) // no dns query yet
   {
      StackLog (<< "sendToWire with no dns result: " << *this);
      assert(sip->isRequest());
      assert(!mIsCancel);
      mDnsResult = mController.mTransportSelector.createDnsResult(this);
      mWaitingForDnsResult=true;
      mController.mTransportSelector.dnsResolve(mDnsResult, sip);
   }
   else // reuse the last dns tuple
   {
      assert(sip->isRequest());
      if(mTarget.getType() != UNKNOWN_TRANSPORT)
      {
         if (resend)
         {
            if (mTarget.transport)
            {
               mController.mTransportSelector.retransmit(sip, mTarget);
            }
            else
            {
               DebugLog (<< "No transport found(network could be down) for " << sip->brief());
            }
         }
         else
         {
            mController.mTransportSelector.transmit(sip, mTarget);
         }
      }
      else
      {
         // .bwc. While the resolver was attempting to find a target, another
         // request came down from the TU. This could be a bug in the TU, or 
         // could be a retransmission of an ACK/200. Either way, we cannot
         // expect to ever be able to send this request (nowhere to store it
         // temporarily).
         DebugLog(<< "Received a second request from the TU for a transaction"
                     " that already existed, before the DNS subsystem was done "
                     "resolving the target for the first request. Either the TU"
                     " has messed up, or it is retransmitting ACK/200 (the only"
                     " valid case for this to happen)");
      }
   }
}

void
TransactionState::sendToTU(TransactionMessage* msg) const
{
   SipMessage* sipMsg = dynamic_cast<SipMessage*>(msg);
   if (sipMsg && sipMsg->isResponse() && mDnsResult)
   {
      // whitelisting rules.
      switch (sipMsg->header(h_StatusLine).statusCode())
      {
         case 503:
            // blacklist last target.
            // .bwc. If there is no Retry-After, we do not blacklist
            // (see RFC 3261 sec 21.5.4 para 1)
            if(sipMsg->exists(resip::h_RetryAfter) && 
               sipMsg->header(resip::h_RetryAfter).isWellFormed())
            {
               unsigned int relativeExpiry= sipMsg->header(resip::h_RetryAfter).value();
               
               mDnsResult->blacklistLast(resip::Timer::getTimeMs()+relativeExpiry*1000);
            }
         
            break;
         case 408:
            if(sipMsg->getReceivedTransport() == 0 && mState == Trying)  // only greylist if internally generated and we haven't received any responses yet
            {
               // greylist last target.
               // ?bwc? How long do we greylist this for? Probably should make
               // this configurable. TODO
               mDnsResult->greylistLast(resip::Timer::getTimeMs() + 32000);
            }

            break;
         default:
            // !bwc! Debatable.
            mDnsResult->whitelistLast();
            break;
      }
   }
   TransactionState::sendToTU(mTransactionUser, mController, msg);
}

void
TransactionState::sendToTU(TransactionUser* tu, TransactionController& controller, TransactionMessage* msg) 
{
   if (!tu)
   {
      DebugLog(<< "Send to default TU: " << std::endl << std::endl << *msg);
   }
   else
   {
	  DebugLog (<< "Send to TU: " << *tu << " " << std::endl << std::endl << *msg);
   }
   
   msg->setTransactionUser(tu);   
   controller.mTuSelector.add(msg, TimeLimitFifo<Message>::InternalElement);
}

SipMessage*
TransactionState::make100(SipMessage* request) const
{
   return (Helper::makeResponse(*request, 100));
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
TransactionState::isRequest(TransactionMessage* msg) const
{
   SipMessage* sip = dynamic_cast<SipMessage*>(msg);   
   return sip && sip->isRequest();
}

bool
TransactionState::isInvite(TransactionMessage* msg) const
{
   if (isRequest(msg))
   {
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      return (sip->method()) == INVITE;
   }
   return false;
}

bool
TransactionState::isResponse(TransactionMessage* msg, int lower, int upper) const
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
TransactionState::isTimer(TransactionMessage* msg) const
{
   return dynamic_cast<TimerMessage*>(msg) != 0;
}

bool
TransactionState::isFromTU(TransactionMessage* msg) const
{
   SipMessage* sip = dynamic_cast<SipMessage*>(msg);
   return sip && !sip->isExternal();
}

bool
TransactionState::isFromWire(TransactionMessage* msg) const
{
   SipMessage* sip = dynamic_cast<SipMessage*>(msg);
   return sip && sip->isExternal();
}

bool
TransactionState::isTransportError(TransactionMessage* msg) const
{
   return dynamic_cast<TransportFailure*>(msg) != 0;
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
   if (mController.mTuSelector.isTransactionUserStillRegistered(mTransactionUser) && 
       mTransactionUser->isRegisteredForTransactionTermination())
   {
      //StackLog (<< "Terminate client transaction " << tid);
      sendToTU(new TransactionTerminated(tid, true, mTransactionUser));
   }
}

void
TransactionState::terminateServerTransaction(const Data& tid)
{
   mState = Terminated;
   if (mController.mTuSelector.isTransactionUserStillRegistered(mTransactionUser) && 
       mTransactionUser->isRegisteredForTransactionTermination())
   {
      //StackLog (<< "Terminate server transaction " << tid);
      sendToTU(new TransactionTerminated(tid, false, mTransactionUser));
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
   strm << " target=" << state.mResponseTarget;
   //if (state.mTransactionUser) strm << " tu=" << *state.mTransactionUser;
   //else strm << "default TU";
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
