#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/AbandonServerTransaction.hxx"
#include "resip/stack/CancelClientInviteTransaction.hxx"
#include "resip/stack/AddTransport.hxx"
#include "resip/stack/RemoveTransport.hxx"
#include "resip/stack/TerminateFlow.hxx"
#include "resip/stack/EnableFlowTimer.hxx"
#include "resip/stack/ZeroOutStatistics.hxx"
#include "resip/stack/PollStatistics.hxx"
#include "resip/stack/ConnectionTerminated.hxx"
#include "resip/stack/KeepAlivePong.hxx"
#include "resip/stack/DnsInterface.hxx"
#include "resip/stack/DnsResultMessage.hxx"
#include "resip/stack/DnsResult.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/MethodTypes.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SendData.hxx"
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
#include "resip/stack/InteropHelper.hxx"
#include "resip/stack/KeepAliveMessage.hxx"
#include "rutil/ResipAssert.h"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/MD5Stream.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Random.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION

UInt32 TransactionState::StatelessIdCounter = 0;

TransactionState::TransactionState(TransactionController& controller, Machine m, 
                                   State s, const Data& id, MethodTypes method, const Data& methodText, TransactionUser* tu) : 
   mController(controller),
   mMachine(m), 
   mState(s),
   mIsAbandoned(false),
   mIsReliable(true), // !jf! 
   mNextTransmission(0),
   mDnsResult(0),
   mId(id),
   mMethod(method),
   mMethodText(method==UNKNOWN ? new Data(methodText) : 0),
   mCurrentMethodType(UNKNOWN),
   mCurrentResponseCode(0),
   mAckIsValid(false),
   mPendingOperation(None),
   mTransactionUser(tu),
   mFailureReason(TransportFailure::None),
   mFailureSubCode(0),
   mTcpConnectTimerStarted(false)
{
   StackLog (<< "Creating new TransactionState: " << *this);
}


TransactionState* 
TransactionState::makeCancelTransaction(TransactionState* tr, Machine machine, const Data& tid)
{
   TransactionState* cancel = new TransactionState(tr->mController, 
                                                   machine, 
                                                   Trying, 
                                                   tid, 
                                                   CANCEL, 
                                                   Data::Empty,
                                                   tr->mTransactionUser);
   // !jf! don't set this since it will be set by TransactionState::processReliability()
   //cancel->mIsReliable = tr->mIsReliable;  
   cancel->mResponseTarget = tr->mResponseTarget;
   cancel->mTarget = tr->mTarget;
   cancel->add(tid);

   // !jf! don't call processServerNonInvite since it will delete
   // the sip message which needs to get sent to the TU
   cancel->processReliability(tr->mTarget.getType());
   return cancel;
}

void 
TransactionState::handleInternalCancel(SipMessage* cancel,
                                       TransactionState& clientInvite)
{
   TransactionState* state = TransactionState::makeCancelTransaction(&clientInvite, ClientNonInvite, clientInvite.mId+"cancel");
   // Make sure the branch in the CANCEL matches the current 
   // branch of the INVITE, in case we have done a DNS failover (the transport 
   // sequences could be different by now)
   cancel->header(h_Vias).front().param(p_branch)=clientInvite.mNextTransmission->const_header(h_Vias).front().param(p_branch);
   state->processClientNonInvite(cancel);
   // for the INVITE in case we never get a 487
   clientInvite.mController.mTimers.add(Timer::TimerCleanUp, clientInvite.mId, 128*Timer::T1);
}

bool
TransactionState::handleBadRequest(const resip::SipMessage& badReq, TransactionController& controller)
{
   resip_assert(badReq.isRequest() && badReq.method() != ACK);
   try
   {
      SipMessage* error = Helper::makeResponse(badReq,400);
      if(badReq.getReason())
      {
         error->header(h_StatusLine).reason()+="(" + *(badReq.getReason())+ ")";
      }
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
   resip_assert(mState != Bogus);

   if (mDnsResult)
   {
      mDnsResult->destroy();
   }

   //StackLog (<< "Deleting TransactionState " << mId << " : " << this);
   erase(mId);
   
   delete mNextTransmission;
   delete mMethodText;
   mNextTransmission = 0;
   mMethodText = 0;

   mState = Bogus;
}

bool
TransactionState::processSipMessageAsNew(SipMessage* sip, TransactionController& controller, const Data& tid)
{
   MethodTypes method=sip->method();
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
            // ?bwc? Um, should we _really_ be doing this statelessly?
            InfoLog( << "No TU found for message: " << sip->brief());               
            SipMessage* noMatch = Helper::makeResponse(*sip, 500);
            Tuple target(sip->getSource());

            controller.mTransportSelector.transmit(noMatch, target);
            delete noMatch;
            return false;
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
         if (method == INVITE)
         {
            // !rk! This might be needlessly created.  Design issue.
            TransactionState* state = new TransactionState(controller, 
                                                            ServerInvite, 
                                                            Trying, 
                                                            tid, 
                                                            INVITE,
                                                            Data::Empty,
                                                            tu);

            state->mNextTransmission = state->make100(sip);
            state->mResponseTarget = sip->getSource(); // UACs source address
            // since we don't want to reply to the source port if rport present 
            state->mResponseTarget.setPort(Helper::getPortForReply(*sip));
            state->mIsReliable = isReliable(state->mResponseTarget.getType());
            state->add(tid);
               
            if (Timer::T100 == 0)
            {
               state->sendCurrentToWire(); // will get deleted when this is deleted
               state->mState = Proceeding;
            }
            else
            {
               //StackLog(<<" adding T100 timer (INV)");
               controller.mTimers.add(Timer::TimerTrying, tid, Timer::T100);
            }
            state->sendToTU(sip);
            return true;
         }
         else if (method == CANCEL)
         {
            // Note:  For cancel requests the tid member passed in will have the token "cancel" appended
            // to it, so that the cancel request can be treated as it's own transaction.  sip->getTransactionId()
            // will be the original tid from the wire and should match the tid of the INVITE request being 
            // cancelled.
            TransactionState* matchingInvite = 
               controller.mServerTransactionMap.find(sip->getTransactionId());
            if (matchingInvite == 0)
            {
               InfoLog (<< "No matching INVITE for incoming (from wire) CANCEL to uas");
               //was TransactionState::sendToTU(tu, controller, Helper::makeResponse(*sip, 481));
               SipMessage* response = Helper::makeResponse(*sip, 481);
               Tuple target(sip->getSource());
               controller.mTransportSelector.transmit(response, target);
               
               delete response;
               return false;
            }
            else
            {
               resip_assert(matchingInvite);
               TransactionState* state = TransactionState::makeCancelTransaction(matchingInvite, ServerNonInvite, tid);
               state->startServerNonInviteTimerTrying(*sip,tid);
               state->sendToTU(sip);
               return true;
            }
         }
         else if (method != ACK)
         {
            TransactionState* state = new TransactionState(controller, 
                                                            ServerNonInvite,
                                                            Trying, 
                                                            tid, 
                                                            method,
                                                            sip->methodStr(),
                                                            tu);
            state->mResponseTarget = sip->getSource();
            // since we don't want to reply to the source port if rport present 
            state->mResponseTarget.setPort(Helper::getPortForReply(*sip));
            state->add(tid);
            state->mIsReliable = isReliable(state->mResponseTarget.getType());
            state->startServerNonInviteTimerTrying(*sip,tid);
            state->sendToTU(sip);
            return true;
         }
            
         // Incoming ACK just gets passed to the TU
         //StackLog(<< "Adding incoming message to TU fifo " << tid);
         TransactionState::sendToTU(tu, controller, sip);
      }
      else // new sip msg from the TU
      {
         if (method == INVITE)
         {
            TransactionState* state = new TransactionState(controller, 
                                                            ClientInvite, 
                                                            Calling, 
                                                            tid, 
                                                            INVITE,
                                                            Data::Empty,
                                                            tu);
            state->add(state->mId);
            state->processClientInvite(sip);
         }
         else if (method == ACK)
         {
            TransactionState* state = new TransactionState(controller, 
                                                            Stateless, 
                                                            Calling, 
                                                            tid, 
                                                            ACK,
                                                            Data::Empty,
                                                            tu);
            state->add(state->mId);
            state->mController.mTimers.add(Timer::TimerStateless, state->mId, Timer::TS );
            state->processStateless(sip);
         }
         else if (method == CANCEL)
         {
            TransactionState* matchingInvite = controller.mClientTransactionMap.find(sip->getTransactionId());
               
            if (matchingInvite == 0)
            {
               InfoLog (<< "No matching INVITE for incoming (from TU) CANCEL to uac");
               TransactionState::sendToTU(tu, controller, Helper::makeResponse(*sip,481));
               return false;
            }
            else if (matchingInvite->mState == Calling) // CANCEL before 1xx received
            {
               WarningLog(<< "You can't CANCEL a request until a provisional has been received");
               StackLog (<< *matchingInvite);
               StackLog (<< *sip);

               matchingInvite->mIsAbandoned = true;
               return false;
            }
            else if (matchingInvite->mState == Completed)
            {
               // A final response was already seen for this INVITE transaction
               matchingInvite->sendToTU(Helper::makeResponse(*sip, 200));
               return false;
            }
            else
            {
               handleInternalCancel(sip, *matchingInvite);
            }
         }
         else 
         {
            TransactionState* state = new TransactionState(controller, 
                                                            ClientNonInvite, 
                                                            Trying, 
                                                            tid, 
                                                            method,
                                                            sip->methodStr(),
                                                            tu);
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
         return false;
      }
      else
      {
         StackLog (<< "forwarding stateless response: " << sip->brief());
         TransactionState* state = 
            new TransactionState(controller, 
                                 Stateless, 
                                 Calling, 
                                 Data(StatelessIdCounter++), 
                                 method,
                                 sip->methodStr(),
                                 tu);
         state->add(state->mId);
         state->mController.mTimers.add(Timer::TimerStateless, state->mId, Timer::TS );
         state->processStateless(sip);
      }
   }
   else // wasn't a request or a response
   {
      ErrLog (<< "Got a SipMessage that was neither a request nor response!" 
               << sip->brief());
      return false;
   }

   return true;
}

void
TransactionState::process(TransactionController& controller, 
                           TransactionMessage* message)
{ 
   // Note:  KeepAliveMessage is a special SipMessage - check for it first
   KeepAliveMessage* keepAlive = dynamic_cast<KeepAliveMessage*>(message);
   if (keepAlive)
   {
      StackLog ( << "Sending keep alive to: " << keepAlive->getDestination());      
      controller.mTransportSelector.transmit(keepAlive, keepAlive->getDestination());
      delete keepAlive;
      return;
   }

   SipMessage* sip = dynamic_cast<SipMessage*>(message);
   if(!sip)
   {
      KeepAlivePong* pong = dynamic_cast<KeepAlivePong*>(message);
      if (pong)
      {
         controller.mTuSelector.add(pong);
         delete pong;
         return;
      }

      ConnectionTerminated* term = dynamic_cast<ConnectionTerminated*>(message);
      if (term)
      {
         controller.mTuSelector.add(term);
         delete term;
         return;
      }

      TerminateFlow* termFlow = dynamic_cast<TerminateFlow*>(message);
      if(termFlow)
      {
         controller.mTransportSelector.terminateFlow(termFlow->getFlow());
         delete termFlow;
         return;
      }

      EnableFlowTimer* enableFlowTimer = dynamic_cast<EnableFlowTimer*>(message);
      if(enableFlowTimer)
      {
         controller.mTransportSelector.enableFlowTimer(enableFlowTimer->getFlow());
         delete enableFlowTimer;
         return;
      }

      ZeroOutStatistics* zeroOutStatistics = dynamic_cast<ZeroOutStatistics*>(message);
      if(zeroOutStatistics)
      {
         controller.mStatsManager.zeroOut();
         delete zeroOutStatistics;
         return;
      }

      PollStatistics* pollStatistics = dynamic_cast<PollStatistics*>(message);
      if(pollStatistics)
      {
         controller.mStatsManager.poll();
         delete pollStatistics;
         return;
      }

      AddTransport* addTransport = dynamic_cast<AddTransport*>(message);
      if(addTransport)
      {
         controller.mTransportSelector.addTransport(addTransport->getTransport(), true /* isStackRunning */);
         delete addTransport;
         return;
      }

      RemoveTransport* removeTransport = dynamic_cast<RemoveTransport*>(message);
      if(removeTransport)
      {
         controller.mTransportSelector.removeTransport(removeTransport->getTransportKey());
         delete removeTransport;
         return;
      }
   }
   
   // .bwc. We can't do anything without a tid here. Check this first.
   Data tid;   
   try
   {
      tid = message->getTransactionId();
   }
   catch(resip::BaseException&)
   {
      // .bwc This is not our error. Do not ErrLog.
      DebugLog( << "TransactionState::process dropping message with invalid tid " << message->brief());
      delete message;
      return;
   }
   
   MethodTypes method = UNKNOWN;

   if(sip)
   {
      method=sip->method();
      // ?bwc? Should this come after checking for error conditions?
      if(controller.mStack.statisticsManagerEnabled() && sip->isExternal())
      {
         controller.mStatsManager.received(sip);
      }
      
      // .bwc. Check for error conditions we can respond to.
      if(sip->isRequest() && method != ACK)
      {
         // .bwc. If we are going to statelessly send a 503, we should do it
         // in the transport, before we do expensive stuff like basicCheck and
         // stampReceived. If the TU fifo is backed up, we should send a 
         // _stateful_ 503 below. (And only if the specific TU that can handle
         // this message is backed up; if other TUs are congested, we should let 
         // it pass.)
         /*
         if(sip->isExternal() && (controller.isTUOverloaded() || controller.mStateMacFifo.isRejecting()))
         {
            SipMessage* tryLater = Helper::makeResponse(*sip, 503);
            if( controller.mStateMacFifo.isRejecting() )
              tryLater->header(h_RetryAfter).value() = controller.mStateMacFifo.getRetryAfter();
            else
              tryLater->header(h_RetryAfter).value() = 32 + (Random::getRandom() % 32);
              
            //tryLater->header(h_RetryAfter).comment() = "Server busy TRANS";
            Tuple target(sip->getSource());
            delete sip;
            controller.mTransportSelector.transmit(tryLater, target);
            delete tryLater;
            return;
         }
         */
         
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
      catch(resip::BaseException& e)
      {
         if(sip->isRequest() && method!=ACK)
         {
            handleBadRequest(*sip,controller);
         }
         
         InfoLog(<< "Exception caught by pedantic stack: " << e);
      }
#endif      
      
      // This ensures that CANCEL requests form unique transactions
      if (method == CANCEL) 
      {
         tid += "cancel";
      }
   }
      
   TransactionState* state = 0;
   if (message->isClientTransaction()) 
   {
      state = controller.mClientTransactionMap.find(tid);
   }
   else 
   {
      state = controller.mServerTransactionMap.find(tid);
   }
   
   if (state && sip && sip->isExternal())
   {
      // Various kinds of response fixup.
      if(sip->isResponse() &&
         state->mNextTransmission)
      {
         // .bwc. This code (if enabled) ensures that responses have the same
         // CallId and tags as the request did (excepting the introduction of a 
         // remote tag). This is to protect dialog-stateful TUs that don't react 
         // gracefully when a stupid/malicious endpoint fiddles with the tags 
         // and/or CallId when it isn't supposed to. (DUM is one such TU)
         if(state->mController.getFixBadDialogIdentifiers())
         {
            if(sip->const_header(h_CallId).isWellFormed())
            {
               if(!(sip->const_header(h_CallId) == 
                           state->mNextTransmission->const_header(h_CallId)))
               {
                  InfoLog(<< "Other end modified our Call-Id... correcting.");
                  sip->header(h_CallId) = state->mNextTransmission->const_header(h_CallId);
               }
            }
            else
            {
               InfoLog(<< "Other end corrupted our CallId... correcting.");
               sip->header(h_CallId) = state->mNextTransmission->const_header(h_CallId);
            }
   
            const NameAddr& from = state->mNextTransmission->const_header(h_From);
            if(sip->const_header(h_From).isWellFormed())
            {
               // Overwrite tag.
               if(from.exists(p_tag))
               {
                  if(!sip->const_header(h_From).exists(p_tag) ||
                     sip->const_header(h_From).param(p_tag) != from.param(p_tag))
                  {
                     InfoLog(<<"Other end modified our local tag... correcting.");
                     sip->header(h_From).param(p_tag) = from.param(p_tag);
                  }
               }
               else if(sip->const_header(h_From).exists(p_tag))
               {
                  InfoLog(<<"Other end added a local tag for us... removing.");
                  sip->header(h_From).remove(p_tag);
               }
            }
            else
            {
               InfoLog(<<"Other end corrupted our From header... replacing.");
               // Whole header is hosed, overwrite.
               sip->header(h_From) = from;
            }
   
            const NameAddr& to = state->mNextTransmission->const_header(h_To);
            if(sip->const_header(h_To).isWellFormed())
            {
               // Overwrite tag.
               if(to.exists(p_tag))
               {
                  if(!sip->const_header(h_To).exists(p_tag) ||
                     sip->const_header(h_To).param(p_tag) != to.param(p_tag))
                  {
                     InfoLog(<<"Other end modified the (existing) remote tag... "
                                 "correcting.");
                     sip->header(h_To).param(p_tag) = to.param(p_tag);
                  }
               }
            }
            else
            {
               InfoLog(<<"Other end corrupted our To header... replacing.");
               // Whole header is hosed, overwrite.
               sip->header(h_To) = to;
            }
         }

         // .bwc. This code (if enabled) ensures that responses have the same
         // CSeq number as the request did. This is to protect TUs that don't 
         // react gracefully when a stupid/malicious endpoint fiddles with the 
         // CSeq number. (This is a very cheap check; we already parse the CSeq
         // for incoming messages)
         if(state->mController.getFixBadCSeqNumbers())
         {
            unsigned int old=state->mNextTransmission->const_header(h_CSeq).sequence();
            if(sip->const_header(h_CSeq).sequence() != old)
            {
               InfoLog(<<"Other end changed our CSeq number... replacing.");
               sip->header(h_CSeq).sequence()=old;
            }
         }
      }
      // .bwc. This code ensures that the transaction state-machine can recover
      // from ACK/200 with the same tid as the original INVITE. This problem is
      // stupidly common. 
      if(sip->isRequest() && method == ACK && !state->mAckIsValid)
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

   if(state && sip)
   {
      switch(state->mMethod)
      {
         case INVITE:
            if(method != INVITE && method != ACK)
            {
               // Maybe respond if a request?
               delete sip;
               return;
            }
            break;
         case UNKNOWN:
            if(!state->mMethodText || *(state->mMethodText) != sip->methodStr())
            {
               // Maybe respond if a request?
               delete sip;
               return;
            }
            break;
         default:
            if(state->mMethod != method)
            {
               // Maybe respond if a request?
               delete sip;
               return;
            }
            break;
      }

      // .bwc. in private email 1 Feb 2013:
      // According to the spec, there is no such thing as a reliable NIT
      // retransmission; what we have just observed is a transaction id collision
      // technically. Maybe a reliable NIT transaction collision needs special
      // handling? It is probably a lot more common that this is a confused client,
      // than a client that has innocently used the same tid as some other client,
      // though. Maybe we should just ignore such requests?
      if(sip->isExternal() && sip->isRequest() &&
         method != ACK &&
         state->mIsReliable)
      {
         InfoLog(<<"Someone sent us a request with a repeated transaction ID "
                     "over a reliable transport.  Discarding the request.");
         delete sip;
         return;
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
            resip_assert (!sip || !(state->isFromTU(sip) &&  sip->isRequest() && method == ACK));
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
            resip_assert(0);
            return;
      }
   }
   else if (sip)  // new transaction
   {
      try
      {
         bool processed = processSipMessageAsNew(sip, controller, tid);
         if (!processed)
         {
            delete sip;      
         }
      }
      catch(resip::BaseException& e)   
      {
         StackLog ( << "Got badly formatted sip message, error: " << e.what());      
         if(sip->isRequest() && sip->method()!=ACK)
         {
            handleBadRequest(*sip, controller);
         }         
         delete sip;      
      }
      catch(std::exception& err)      
      {
         StackLog ( << "Got error: " << err.what());
         delete sip;
      }
   } 
   else // timer or other non-sip msg
   {
      //StackLog (<< "discarding non-sip message: " << message->brief());
      delete message;
   }
}

void
TransactionState::processTimer(TransactionController& controller,
                                 TimerMessage* message)
{
   Data tid = message->getTransactionId();

   if(controller.getRejectionBehavior()==CongestionManager::REJECTING_NON_ESSENTIAL)
   {
      // .bwc. State machine fifo is backed up; we probably should not be 
      // retransmitting anything right now. If we have a retransmit timer, 
      // reschedule for later, but don't retransmit.
      switch(message->getType())
      {
         case Timer::TimerA: // doubling
            controller.mTimers.add(Timer::TimerA, 
                                    tid, 
                                    message->getDuration()*2);
            delete message;
            return;
         case Timer::TimerE1:// doubling, until T2
         case Timer::TimerG: // doubling, until T2
            controller.mTimers.add(message->getType(), 
                                    tid, 
                                    resipMin(message->getDuration()*2,
                                             Timer::T2));
            delete message;
            return;
         case Timer::TimerE2:// just reset
            controller.mTimers.add(Timer::TimerE2, 
                                    tid, 
                                    Timer::T2);
            delete message;
            return;
         default:
            ; // let it through
      }
   }

   TransactionState* state = 0;
   if (message->isClientTransaction()) state = controller.mClientTransactionMap.find(tid);
   else state = controller.mServerTransactionMap.find(tid);
   
   if (state) // found transaction for timer
   {
      StackLog (<< "Found matching transaction for " << message->brief() << " -> " << *state);

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
            resip_assert(0);
            return;
      }
   }
   else
   {
      delete message;
   }

}

void
TransactionState::startServerNonInviteTimerTrying(SipMessage& sip, const Data& tid)
{
   unsigned int duration = 3500;
   if(Timer::T1 != 500) // optimzed for T1 == 500
   {
      // Iteratively calculate how much time before TimerE reaches T2 (RFC4320) - could be improved
      duration = Timer::T1;
      while(duration*2<Timer::T2) duration = duration * 2;
   }
   resetNextTransmission(make100(&sip));  // Store for use when timer expires
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
      resetNextTransmission(sip);
      sendCurrentToWire();
   }
   else if(sip && isFromWire(sip))
   {
      InfoLog (<< "Received message from wire on a stateless transaction");
      StackLog (<< *sip);
      //assert(0);
      sendToTU(sip);
   }
   else if (isTransportError(message))
   {
      processTransportFailure(message);
      
      delete message;
      delete this;
   }
   else if (isTcpConnectState(message))
   {
       // stateless mode is not supported
       //processTcpConnectState(message);
       delete message;
   }
   else if (isTimer(message))
   {
      TimerMessage* timer = dynamic_cast<TimerMessage*>(message);
      if (timer->getType() == Timer::TimerStateless)
      {
         delete message;
         delete this;
      }
      else
      {
         delete timer;
         resip_assert(0);
      }
   }
   else if(dynamic_cast<DnsResultMessage*>(message))
   {
      handleSync(mDnsResult);
      delete message;
   }
   else if (isAbandonServerTransaction(message))
   {
      // ?
      delete message;
   }
   else
   {
      delete message;
      resip_assert(0);
   }
}

void 
TransactionState::saveOriginalContactAndVia(const SipMessage& sip)
{
   if(sip.exists(h_Contacts) && sip.const_header(h_Contacts).size() == 1 &&
      sip.const_header(h_Contacts).front().isWellFormed())
   {
      mOriginalContact = std::auto_ptr<NameAddr>(new NameAddr(sip.header(h_Contacts).front()));
   }
   mOriginalVia = std::auto_ptr<Via>(new Via(sip.header(h_Vias).front()));
}

void TransactionState::restoreOriginalContactAndVia()
{
   if (mOriginalContact.get())
   {
      mNextTransmission->header(h_Contacts).front() = *mOriginalContact;
   }                  
   if (mOriginalVia.get())
   {
      mOriginalVia->param(p_branch).incrementTransportSequence();
      mNextTransmission->header(h_Vias).front() = *mOriginalVia;
   }
}

void
TransactionState::processClientNonInvite(TransactionMessage* msg)
{ 
   StackLog (<< "TransactionState::processClientNonInvite: " << msg->brief());

   if (isRequest(msg) && isFromTU(msg))
   {
      //StackLog (<< "received new non-invite request");
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      resetNextTransmission(sip);
      saveOriginalContactAndVia(*sip);
      mController.mTimers.add(Timer::TimerF, mId, Timer::TF);
      sendCurrentToWire();
   }
   else if (isResponse(msg) && isFromWire(msg)) // from the wire
   {
      //StackLog (<< "received response from wire");

      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      int code = sip->const_header(h_StatusLine).responseCode();
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
         else
         {
            resip_assert(0);
            delete sip;
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
            // !bwc! Got final response in NIT. We don't need to do anything
            // except quietly absorb retransmissions. Dump all state.
            if(mDnsResult)
            {
               mDnsResult->destroy();
               mDnsResult=0;
               mPendingOperation=None;
            }
            resetNextTransmission(0);
         }
      }
      else
      {
         resip_assert(0);
         delete sip;
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
               StackLog (<< "Transmitting current message");
               sendCurrentToWire();
               delete timer;
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
               StackLog (<< "Transmitting current message");
               sendCurrentToWire();
               delete timer;
            }
            else 
            {
               // ignore
               delete msg;
            }
            break;

         case Timer::TcpConnectTimer:
             if (!mTcpConnectTimerStarted) // Ignore timer if we we connected (note: when we connect we set mTcpConnectTimerStarted to false)
             {
                 delete msg;
                 break;
             }
             // else fallthrough 
         case Timer::TimerF:
            if (mState == Trying || mState == Proceeding)
            {
               // !bwc! We hold onto this until we get a response from the wire
               // in client transactions, for this contingency.
               resip_assert(mNextTransmission);
               if(mPendingOperation == Dns)
               {
                  WarningLog(<< "Transaction timed out while waiting for DNS "
                              "result uri=" << 
                              mNextTransmission->const_header(h_RequestLine).uri());
                  sendToTU(Helper::makeResponse(*mNextTransmission, 503, "DNS Timeout"));
               }
               else
               {
                  sendToTU(Helper::makeResponse(*mNextTransmission, 408));
               }
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
   else if (isTcpConnectState(msg))
   {
      processTcpConnectState(msg);
      delete msg;
   }
   else if(dynamic_cast<DnsResultMessage*>(msg))
   {
      handleSync(mDnsResult);
      delete msg;
   }
   else if (isAbandonServerTransaction(msg))
   {
      // ?
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
            if(mState==Calling && !mNextTransmission && mMsgToRetransmit.empty())
            {
               resetNextTransmission(sip);
               saveOriginalContactAndVia(*sip);
               mController.mTimers.add(Timer::TimerB, mId, Timer::TB );
               sendCurrentToWire();
            }
            else
            {
               WarningLog(<< "TU sent us a duplicate INVITE: fix this!");
               delete sip;
            }
            break;
            
         case CANCEL:
            resip_assert(0);
            delete msg;
            break;

         default:
            WarningLog(<< "TU sent us an erroneous request inside a Client"
                        " INVITE transaction: fix this!");
            delete msg;
            break;
      }
   }
   else if (isResponse(msg) && isFromWire(msg))
   {
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      int code = sip->const_header(h_StatusLine).responseCode();
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
                  if(mIsAbandoned)
                  {
                     SipMessage* cancel = Helper::makeCancel(*mNextTransmission);
                     // Iterate through message decorators on the INVITE and see if any need to be copied to the CANCEL
                     mNextTransmission->copyOutboundDecoratorsToStackCancel(*cancel);
                     handleInternalCancel(cancel, *this);
                     mIsAbandoned=false;
                  }
                  // !bwc! We have gotten a response. We don't need to
                  // retransmit the original INVITE anymore (so we clear mMsgToRetransmit), 
                  // but we do need to retain the full original INVITE until we get a final 
                  // response, in case we need to forge an ACK.
                  mMsgToRetransmit.clear();
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
               mIsAbandoned=false;
               sendToTU(sip); // don't delete msg
               //terminateClientTransaction(mId);
               mMachine = ClientStale;
               mState = Completed;
               // !bwc! We have a final response. We don't need either of
               // mMsgToRetransmit or mNextTransmission. We ignore further
               // traffic.
               resetNextTransmission(0);
               if(mDnsResult)
               {
                  mDnsResult->destroy();
                  mDnsResult=0;
                  mPendingOperation=None;
               }
               StackLog (<< "Received 2xx on client invite transaction");
               StackLog (<< *this);
               mController.mTimers.add(Timer::TimerStaleClient, mId, Timer::TS );
            }
            else if (code >= 300)
            {
               mIsAbandoned=false;
               // When in either the "Calling" or "Proceeding" states, reception of a
               // response with status code from 300-699 MUST cause the client
               // transaction to transition to "Completed".
               if (mIsReliable)
               {
                  // Stack MUST pass the received response up to the TU, and the client
                  // transaction MUST generate an ACK request, even if the transport is
                  // reliable
                  SipMessage* ack = Helper::makeFailureAck(*mNextTransmission, *sip);
                  mNextTransmission->copyOutboundDecoratorsToStackFailureAck(*ack);
                  resetNextTransmission(ack);
                  
                  // want to use the same transport as was selected for Invite
                  resip_assert(mTarget.getType() != UNKNOWN_TRANSPORT);
                  sendCurrentToWire();
                  sendToTU(sip); // don't delete msg
                  terminateClientTransaction(mId);
                  
                  // !bwc! We only do this because we are assured the ACK
                  // will make it to the other end; if we are using an 
                  // unreliable transport, we need to stick around to absorb
                  // retransmissions of the response.
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
                     SipMessage* ack = Helper::makeFailureAck(*mNextTransmission, *sip);
                     mNextTransmission->copyOutboundDecoratorsToStackFailureAck(*ack);
                     resetNextTransmission(ack);
                     sendCurrentToWire();
                     if(mDnsResult)
                     {
                        mDnsResult->destroy();
                        mDnsResult=0;
                        mPendingOperation=None;
                     }
                     sendToTU(sip); // don't delete msg
                  }
                  else if (mState == Completed)
                  {
                     // Any retransmissions of the final response that
                     // are received while in the "Completed" state MUST
                     // cause the ACK to be re-passed to the transport
                     // layer for retransmission.
                     sendCurrentToWire();
                     delete sip;
                  }
                  else
                  {
                     /* This should never Happen if it happens we should have a plan
                        what to do here?? for now assert will work
                     */
                     CritLog(  << "State invalid");
                     // !ah! syslog
                     resip_assert(0);
                     delete sip;
                  }
               }
            }
            else
            {
               delete sip;
               resip_assert(0);
            }
            break;
            
         case CANCEL:
            resip_assert(0);
            delete sip;
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
            if (mState == Calling && !mIsAbandoned)
            {
               unsigned long d = timer->getDuration()*2;
               // TimerA is supposed to double with each retransmit RFC3261 17.1.1          

               mController.mTimers.add(Timer::TimerA, mId, d);
               DebugLog (<< "Retransmitting INVITE ");
               sendCurrentToWire();
            }
            delete msg;
            break;

         case Timer::TcpConnectTimer:
             if (!mTcpConnectTimerStarted) // Ignore timer we we connected (note: when we connect we set mTcpConnectTimerStarted to false)
             {
                 delete msg;
                 break;
             }
             // else fallthrough
         case Timer::TimerB:
            if (mState == Calling)
            {
               resip_assert(mNextTransmission && mNextTransmission->isRequest() &&
                        mNextTransmission->method()==INVITE);
               if(mPendingOperation == Dns)
               {
                  WarningLog(<< "Transaction timed out while waiting for DNS "
                              "result uri=" << 
                              mNextTransmission->const_header(h_RequestLine).uri());
                  sendToTU(Helper::makeResponse(*mNextTransmission, 503, "DNS Timeout"));
               }
               else
               {
                  sendToTU(Helper::makeResponse(*mNextTransmission, 408));
               }
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
            if (mState == Proceeding)
            {
               resip_assert(mNextTransmission && mNextTransmission->isRequest() && 
                        mNextTransmission->method() == INVITE);
               StackLog (<< "Timer::TimerCleanUp: " << *this << std::endl << *mNextTransmission);
               InfoLog(<<"Making 408 for canceled invite that received no response: "<< mNextTransmission->brief());
               if(mPendingOperation == Dns)
               {
                  WarningLog(<< "Transaction timed out while waiting for DNS "
                              "result uri=" << 
                              mNextTransmission->const_header(h_RequestLine).uri());
                  sendToTU(Helper::makeResponse(*mNextTransmission, 503, "DNS Timeout"));
               }
               else
               {
                  sendToTU(Helper::makeResponse(*mNextTransmission, 408));
               }
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
   else if (isTcpConnectState(msg))
   {
       processTcpConnectState(msg);
       delete msg;
   }
   else if (isCancelClientTransaction(msg))
   {
      // TU wants to CANCEL this transaction. See if we can...
      if(mState==Proceeding)
      {
         // We can send the CANCEL now.
         SipMessage* cancel=Helper::makeCancel(*mNextTransmission);
         mNextTransmission->copyOutboundDecoratorsToStackCancel(*cancel);
         TransactionState::handleInternalCancel(cancel, *this);
      }
      else if(mState==Calling)
      {
         // We can't send the CANCEL yet, remember to.
         mIsAbandoned = true;
      }
      delete msg;
   }
   else if(dynamic_cast<DnsResultMessage*>(msg))
   {
      handleSync(mDnsResult);
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
         if(mIsAbandoned)
         {
            resip_assert(mState == Completed);
            mIsAbandoned=false;
            // put a 500 in mNextTransmission
            SipMessage* req = dynamic_cast<SipMessage*>(msg);
            resetNextTransmission(Helper::makeResponse(*req, 500));
            sendCurrentToWire();
         }
         else
         {
            // We have already sent a 100, but we have just received a retransmission.  Requests 
            // likely crossed on the wire.  We need to respond with another 100, but the last one was
            // cleared so re-create the 100 now. 
            SipMessage* sip = dynamic_cast<SipMessage*>(msg);
            if (sip && mMsgToRetransmit.empty() && !mNextTransmission)
            {
               resetNextTransmission(make100(sip));
            }
            sendCurrentToWire();
         }
         delete msg;
      }
      else
      {
         CritLog (<< "Fatal error in TransactionState::processServerNonInvite " 
                  << msg->brief()
                  << " state=" << *this);
         resip_assert(0);
         delete msg;
         return;
      }
   }
   else if (isResponse(msg) && isFromTU(msg))
   {
      SipMessage* sip = dynamic_cast<SipMessage*>(msg);
      int code = sip->const_header(h_StatusLine).responseCode();
      if (code >= 100 && code < 200) // 1XX
      {
         if (mState == Trying || mState == Proceeding)
         {
            resetNextTransmission(sip);
            mState = Proceeding;
            sendCurrentToWire(); // don't delete msg
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
            resetNextTransmission(sip);
            sendCurrentToWire();
            terminateServerTransaction(mId);
            
            // !bwc! We can only do this because we are in a reliable
            // transport, and do not need to hang around to soak up 
            // retransmissions.
            delete this;
         }
         else
         {
            if (mState == Trying || mState == Proceeding)
            {
               mState = Completed;
               mController.mTimers.add(Timer::TimerJ, mId, 64*Timer::T1 );
               resetNextTransmission(sip);
               sendCurrentToWire();
            }
            else if (mState == Completed)
            {
               // ignore
               delete sip;
            }
            else
            {
               CritLog (<< "Fatal error in TransactionState::processServerNonInvite " 
                        << msg->brief()
                        << " state=" << *this);
               resip_assert(0);
               delete sip;
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
      resip_assert(timer);
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
               sendCurrentToWire();
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
      // Failed to send response - transport has likely been removed
      WarningLog (<< "Failed to send response to server transaction (transport was likely removed)." << *this);
      delete msg;
      terminateServerTransaction(mId);
      delete this;
   }
   else if (isAbandonServerTransaction(msg))
   {
      if(mState==Trying || mState==Proceeding)
      {
         mIsAbandoned = true;

         // !bwc! We could check to see if we have a 100 lying around, and 
         // convert it into a 500 for immediate transmission, but it is not 
         // clear that this is a good idea, especially if the TU has abandoned 
         // this transaction after the remote endpoint has stopped 
         // retransmitting. Maybe we could use a time-stamp to help here? Would
         // it be worth the extra memory footprint?

         if (mIsReliable)
         {
            // If we haven't sent a 500 yet, we never will (no retransmissions 
            // to make the response with).
            terminateServerTransaction(mId);
            delete this;
         }
         else
         {
            // If we haven't sent a 500 yet, we'll do so when the next
            // retransmission comes in. In the meantime, set up timers for
            // transaction termination.
            mState = Completed;
            mController.mTimers.add(Timer::TimerJ, mId, 64*Timer::T1 );
         }
      }
      delete msg;
   }
   else if(dynamic_cast<DnsResultMessage*>(msg))
   {
      handleSync(mDnsResult);
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
            if(mIsAbandoned)
            {
               mIsAbandoned=false;
               mAckIsValid=true;
               resetNextTransmission(Helper::makeResponse(*sip, 500));
               mState = Completed;
               mController.mTimers.add(Timer::TimerH, mId, Timer::TH );
               if (!mIsReliable)
               {
                  mController.mTimers.add(Timer::TimerG, mId, Timer::T1 );
               }
               sendCurrentToWire();
               delete msg;
               return;
            }

            if (mState == Proceeding || mState == Completed)
            {
               /*
                 The server transaction has already been constructed so this
                 message is a retransmission.  The server transaction must
                 respond with a 100 Trying _or_ the last provisional response
                 passed from the TU for this transaction.
               */
               //StackLog (<< "Received invite from wire - forwarding to TU state=" << mState);
               
               // !bwc! If we have nothing to respond with, make something.
               if (mMsgToRetransmit.empty() && !mNextTransmission)
               {
                  resetNextTransmission(make100(sip));
               }
               delete sip;
               sendCurrentToWire();
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
                  // !bwc! Got an ACK/failure; we can stop retransmitting
                  // our failure response now.
                  resetNextTransmission(0);
                  delete sip;
               }
            }
            else
            {
               //StackLog (<< "Ignore ACK not in Completed state");
               delete msg;
            }
            break;

         case CANCEL:
            resip_assert(0);
            delete sip;
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
      int code = sip->const_header(h_StatusLine).responseCode();
      switch (sip->method())
      {
         case INVITE:
            if (code == 100)
            {
               if (mState == Trying || mState == Proceeding)
               {
                  //StackLog (<< "Received 100 in Trying or Proceeding. Send over wire");
                  resetNextTransmission(sip); // may be replacing the 100
                  mState = Proceeding;
                  sendCurrentToWire(); // don't delete msg
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
                  resetNextTransmission(sip); // may be replacing the 100
                  mState = Proceeding;
                  sendCurrentToWire(); // don't delete msg
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
                  resetNextTransmission(sip); // may be replacing the 100
                  sendCurrentToWire();
                  
                  // Keep the StaleServer transaction around, so we can keep the
                  // source Tuple that the request was received on. 
                  //terminateServerTransaction(mId);
                  mMachine = ServerStale;
                  mController.mTimers.add(Timer::TimerStaleServer, mId, Timer::TS );
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
                  resetNextTransmission(sip);
                  mState = Completed;
                  mController.mTimers.add(Timer::TimerH, mId, Timer::TH );
                  if (!mIsReliable)
                  {
                     mController.mTimers.add(Timer::TimerG, mId, Timer::T1 );
                  }
                  sendCurrentToWire(); // don't delete msg
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
            resip_assert(0);
            delete sip;
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
               sendCurrentToWire();
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
               sendCurrentToWire(); // will get deleted when this is deleted
               mState = Proceeding;
            }
            else
            {
               //StackLog (<< "TimerTrying fired. Not in Trying state. Ignoring");
            }
            break;
            
         default:
            CritLog(<<"unexpected timer fired: " << timer->getType());
            resip_assert(0); // programming error if any other timer fires
            break;
      }
      delete timer;
   }
   else if (isTransportError(msg))
   {
      // Failed to send response - transport has likely been removed
      WarningLog (<< "Failed to send response to server transaction (transport was likely removed)." << *this);
      delete msg;
      terminateServerTransaction(mId);
      delete this;
   }
   else if (isAbandonServerTransaction(msg))
   {
      if((mState == Trying || mState == Proceeding) && !mIsAbandoned)
      {
         // We need to schedule teardown, and 500 the next retransmission.
         if(mNextTransmission)
         {
            mMsgToRetransmit.clear();
            // hey, we had a 1xx laying around! Turn it into a 500 and send.
            resip_assert(mNextTransmission->isResponse());
            resip_assert(mNextTransmission->const_header(h_StatusLine).statusCode()/100==1);
            mNextTransmission->header(h_StatusLine).statusCode()=500;
            mNextTransmission->header(h_StatusLine).reason()="Server Error";
            sendCurrentToWire();
            mAckIsValid=true;
            StackLog (<< "Received failed response in Trying or Proceeding. Start Timer H, move to completed." << *this);
            mState = Completed;
            mController.mTimers.add(Timer::TimerH, mId, Timer::TH );
            if (!mIsReliable)
            {
               mController.mTimers.add(Timer::TimerG, mId, Timer::T1 );
            }
         }
         else
         {
            // !bwc! TODO try to convert mMsgToRetransmit if present.
            if(mIsReliable)
            {
               // We will never see another retransmission of the INVITE. We
               // need to bail.
               terminateServerTransaction(mId);
               delete this;
            }
            else
            {
               // We should see a retransmission of the INVITE shortly, or we
               // will time out eventually. Be patient...
               mIsAbandoned = true;
            }
         }
      }
      delete msg;
   }
   else if(dynamic_cast<DnsResultMessage*>(msg))
   {
      handleSync(mDnsResult);
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
   else if(isResponse(msg, 200, 299))
   {
      resip_assert(isFromWire(msg));
      sendToTU(msg);
   }
   else if(dynamic_cast<DnsResultMessage*>(msg))
   {
      handleSync(mDnsResult);
      delete msg;
   }
   else if (isAbandonServerTransaction(msg))
   {
      // ?
      delete msg;
   }
   else if (isCancelClientTransaction(msg))
   {
      // ?
      delete msg;
   }
   else
   {
      // might have received some other response because a downstream UAS is
      // misbehaving. For instance, sending a 487/INVITE after already
      // sending a 200/INVITE. It could also be some other message type.
      StackLog (<< "Discarding extra message: " << *msg);
      delete msg;
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
      resip_assert(isFromWire(msg));
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
      resetNextTransmission(sip);
      sendCurrentToWire(); 
   }
   else if(dynamic_cast<DnsResultMessage*>(msg))
   {
      handleSync(mDnsResult);
      delete msg;
   }
   else if (isAbandonServerTransaction(msg))
   {
      // ?
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
   if(!mNextTransmission || mNextTransmission->method()==ACK)
   {
      // This is probably an ACK; since we know we will never need to send a 
      // response to an ACK, we delete mNextTransmission as soon as we 
      // serialize it.
      return;
   }

   WarningCategory warning;
   SipMessage* response = Helper::makeResponse(*mNextTransmission, 503);
   warning.hostname() = mController.mHostname;
   warning.code() = 399;
   warning.text().reserve(100);

   if(mDnsResult)
   {
      InfoLog (<< "Ran out of dns entries for " << mDnsResult->target() << ". Send 503");
      resip_assert(mDnsResult->available() == DnsResult::Finished);
      oDataStream warnText(warning.text());
      warnText << "No other DNS entries to try ("
               << mFailureReason << "," << mFailureSubCode << ")";
   }
   else
   {
      oDataStream warnText(warning.text());
      warnText << "Transport failure ("
               << mFailureReason << "," << mFailureSubCode << ")";
   }

   switch(mFailureReason)
   {
      case TransportFailure::None:
         response->header(h_StatusLine).reason() = "No DNS results";
         break;

      case TransportFailure::Failure:
      case TransportFailure::TransportNoSocket:
      case TransportFailure::TransportBadConnect:
      case TransportFailure::ConnectionUnknown:
      case TransportFailure::ConnectionException:
         response->header(h_StatusLine).reason() = "Transport failure: no transports left to try";
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
         break;
      case TransportFailure::TransportNoExistConn:
         if(InteropHelper::getOutboundVersion() >= 5)
         {
            response->header(h_StatusLine).statusCode() = 430;
         }
         else
         {
            response->header(h_StatusLine).statusCode() = 410;
         }
         response->header(h_StatusLine).reason() = "Flow failed";
         warning.text() = "Flow no longer exists";
         break;
      case TransportFailure::TransportShutdown:
         response->header(h_StatusLine).reason() = "Transport shutdown: no transports left to try";
         break;
   }

   response->header(h_Warnings).push_back(warning);

   sendToTU(response); 
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
   resip_assert(failure);
   resip_assert(mState!=Bogus);

   // We come here if the tcp connect timer expires, so we reset the flag incase we are
   // going to try another DNS entry that is also TCP.
   mTcpConnectTimerStarted = false;

   // Store failure reasons
   if (failure->getFailureReason() > mFailureReason)
   {
      mFailureReason = failure->getFailureReason();
      mFailureSubCode = failure->getFailureSubCode();
   }

   if (mNextTransmission &&  // Note:  If we just transmitted an ACK then mNextTransmission is cleared, so this check is necessary
       mNextTransmission->isRequest() && 
       mNextTransmission->method() == CANCEL &&
       mState != Completed && 
       mState != Terminated)
   {
      WarningLog (<< "Failed to deliver a CANCEL request");
      StackLog (<< *this);
      resip_assert(mMethod==CANCEL);

      // In the case of a client-initiated CANCEL, we don't want to
      // try other transports in the case of transport error as the
      // CANCEL MUST be sent to the same IP/PORT as the orig. INVITE.
      //?dcm? insepct failure enum?
      SipMessage* response = Helper::makeResponse(*mNextTransmission, 503);
      WarningCategory warning;
      warning.hostname() = mController.mHostname;
      warning.code() = 399;
      warning.text() = "Failed to deliver CANCEL using the same transport as the INVITE was used";
      response->header(h_Warnings).push_back(warning);
         
      sendToTU(response);
      return;
   }

   if(!mDnsResult)
   {
      InfoLog(<< "Transport failure on send that did not use DNS.");
      processNoDnsResults();
   }
   // else If we did DNS resolution, then check if we should try to failover to another DNS entry
   else if(mDnsResult)
   {
      // .bwc. Greylist for 32s
      // !bwc! TODO make this duration configurable.
      mDnsResult->greylistLast(Timer::getTimeMs()+32000);

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
   
      if(shouldFailover)
      {
         InfoLog (<< "Try sending request to a different dns result");
         resip_assert(mMethod!=CANCEL);

         switch (mDnsResult->available())
         {
            case DnsResult::Available:
               InfoLog(<< "We have another DNS result to try.");
               restoreOriginalContactAndVia();
               mTarget = mDnsResult->next();
               mMsgToRetransmit.clear();
               processReliability(mTarget.getType());
               sendCurrentToWire();
               break;
            
            case DnsResult::Pending:
               InfoLog(<< "We have a DNS query pending.");
               mPendingOperation=Dns;
               restoreOriginalContactAndVia();
               mMsgToRetransmit.clear();
               break;

            case DnsResult::Finished:
               InfoLog(<< "No DNS results remain.");
               processNoDnsResults();
               break;

            case DnsResult::Destroyed:
            default:
               InfoLog (<< "Bad state: " << *this);
               resip_assert(0);
         }
      }
      else
      {
         InfoLog(<< "Transport failure on send, and failover is disabled.");
         processNoDnsResults();
      }
   }
}

void 
TransactionState::processTcpConnectState(TransactionMessage* msg)
{
   TcpConnectState* tcpConnectState = dynamic_cast<TcpConnectState*>(msg);
   resip_assert(tcpConnectState);

   if (tcpConnectState->getState() == TcpConnectState::ConnectStarted &&
       !mTcpConnectTimerStarted && Timer::TcpConnectTimeout != 0 &&
       (mState == Trying || mState == Calling))
   {
      // Start Timer
      mController.mTimers.add(Timer::TcpConnectTimer, mId, Timer::TcpConnectTimeout);
      mTcpConnectTimerStarted = true;
   }
   else if (tcpConnectState->getState() == TcpConnectState::Connected &&
       (mState == Trying || mState == Calling))
   {
      mTcpConnectTimerStarted = false;
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

   resip_assert(mNextTransmission->isRequest());
   if (mNextTransmission->const_header(h_RequestLine).uri() != rewrite)
   {
      InfoLog (<< "Rewriting request-uri to " << rewrite);
      mNextTransmission->header(h_RequestLine).uri() = rewrite;
      // !bwc! Changing mNextTransmission invalidates mMsgToRetransmit.
      mMsgToRetransmit.clear();
   }
}

void
TransactionState::handle(DnsResult* result)
{
   // ?bwc? Maybe optmize this to use handleSync() directly when running in
   // single-threaded mode?
   DnsResultMessage* dns = new DnsResultMessage(mId,isClient());
   mController.mStateMacFifo.add(static_cast<TransactionMessage*>(dns));
}

void
TransactionState::handleSync(DnsResult* result)  // !slg! it is strange that we pass the result in here, then more or less ignore it in the method
{
   StackLog (<< *this << " got DNS result: " << *result);
   
   // .bwc. Were we expecting something from mDnsResult?
   if (mPendingOperation == Dns)
   {
      resip_assert(mDnsResult);
      switch (mDnsResult->available())
      {
         case DnsResult::Available:
            mPendingOperation=None;
            mTarget = mDnsResult->next();
            // below allows TU to know which transport we send on
            // (The Via mechanism for setting transport doesn't work for TLS)
            mTarget.mTransportKey = mNextTransmission->getDestination().mTransportKey;
            processReliability(mTarget.getType());
            sendCurrentToWire();
            break;
            
         case DnsResult::Finished:
            mPendingOperation=None;
            processNoDnsResults();
            break;

         case DnsResult::Pending:
            break;
            
         case DnsResult::Destroyed:
         default:
            resip_assert(0);
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
TransactionState::sendCurrentToWire() 
{
   if(!mMsgToRetransmit.empty())
   {
      if(mController.mStack.statisticsManagerEnabled())
      {
         mController.mStatsManager.retransmitted(mCurrentMethodType, 
                                                   isClient(), 
                                                   mCurrentResponseCode);
      }

      mController.mTransportSelector.retransmit(mMsgToRetransmit);
   }
   else if(mNextTransmission) // initial transmission; need to determine target
   {
      SipMessage* sip=mNextTransmission;
      TransportSelector::TransmitState transmitState = TransportSelector::Unsent;

      if(isClient())
      {
         if(mTarget.getType() != UNKNOWN_TRANSPORT) // mTarget is set, so just send.
         {
            transmitState=mController.mTransportSelector.transmit(
                        sip, 
                        mTarget,
                        mIsReliable ? 0 : &mMsgToRetransmit);
         }
         else // mTarget isn't set...
         {
            if (sip->getDestination().mFlowKey) //...but sip->getDestination() will work
            {
               // ?bwc? Maybe we should be nice to the TU and do DNS in this case?
               resip_assert(sip->getDestination().getType() != UNKNOWN_TRANSPORT);

               // .bwc. We have the FlowKey. This completely specifies our 
               // Transport (and Connection, if applicable). No DNS required.
               DebugLog(<< "Sending to tuple: " << sip->getDestination());
               mTarget = sip->getDestination();
               processReliability(mTarget.getType());
               transmitState=mController.mTransportSelector.transmit(
                           sip, 
                           mTarget,
                           mIsReliable ? 0 : &mMsgToRetransmit);
            }
            else // ...so DNS is required...
            {
               if(mDnsResult == 0) // ... and we haven't started a DNS query yet.
               {
                  StackLog (<< "sendToWire with no dns result: " << *this);
                  resip_assert(sip->isRequest());
                  resip_assert(mMethod!=CANCEL); // .bwc. mTarget should be set in this case.
                  mDnsResult = mController.mTransportSelector.createDnsResult(this);
                  mPendingOperation=Dns;
                  mController.mTransportSelector.dnsResolve(mDnsResult, sip);
               }
               else // ... but our DNS query isn't done yet.
               {
                  // .bwc. While the resolver was attempting to find a target, another
                  // request came down from the TU. This could be a bug in the TU, or 
                  // could be a retransmission of an ACK/200. Either way, we cannot
                  // expect to ever be able to send this request (nowhere to store it
                  // temporarily).
                  // ?bwc? Higher log-level?
                  DebugLog(<< "Received a second request from the TU for a transaction"
                           " that already existed, before the DNS subsystem was done "
                           "resolving the target for the first request. Either the TU"
                           " has messed up, or it is retransmitting ACK/200 (the only"
                           " valid case for this to happen)");
               }
            }
         }
      }
      else // server transaction
      {
         resip_assert(mDnsResult == 0);
         resip_assert(sip->exists(h_Vias));
         resip_assert(!sip->const_header(h_Vias).empty());

         // .bwc. Code that tweaks mResponseTarget based on stuff in the SipMessage.
         // ?bwc? Why?
         if (sip->hasForceTarget())
         {
            // ?bwc? Override the target for a single response? Should we even
            // allow this? What about client transactions? Should we overwrite 
            // mResponseTarget here? I don't think this has been thought out properly.
            Tuple target = simpleTupleForUri(sip->getForceTarget());
            StackLog(<<"!ah! response with force target going to : "<<target);
            transmitState=mController.mTransportSelector.transmit(
                        sip, 
                        target,
                        mIsReliable ? 0 : &mMsgToRetransmit);
         }
         else
         {
            if (sip->const_header(h_Vias).front().exists(p_rport) && sip->const_header(h_Vias).front().param(p_rport).hasValue())
            {
               // ?bwc? This was not setting the port in mResponseTarget before. Why would
               // the rport be different than the port in mResponseTarget? Didn't we 
               // already set this? Maybe the TU messed with it? If so, why should we pay 
               // attention to it? Again, this hasn't been thought out.
               mResponseTarget.setPort(sip->const_header(h_Vias).front().param(p_rport).port());
               StackLog(<< "rport present in response: " << mResponseTarget.getPort());
            }
   
            StackLog(<< "tid=" << sip->getTransactionId() << " sending to : " << mResponseTarget);
            transmitState=mController.mTransportSelector.transmit(
                        sip, 
                        mResponseTarget,
                        mIsReliable ? 0 : &mMsgToRetransmit);
         }
      }

      // !bwc! If we don't have DNS results yet, or TransportSelector::transmit
      // fails, we hang on to the full original SipMessage, in the hope that 
      // next time it works.
      if (transmitState == TransportSelector::Sent)
      {
         onSendSuccess();
      }
   }
   else
   {
      resip_assert(0);
   }
}

void
TransactionState::onSendSuccess()
{
   SipMessage* sip=mNextTransmission;

   if(mController.mStack.statisticsManagerEnabled())
   {
      mController.mStatsManager.sent(sip);
   }

   mCurrentMethodType = sip->method();
   if(sip->isResponse())
   {
      mCurrentResponseCode = sip->const_header(h_StatusLine).statusCode();
   }

   // !bwc! If mNextTransmission is a non-ACK request, we need to save the
   // initial request in case we need to send a simulated 408 or a 503 to
   // the TU (at least, until we get a response back)
   if(!mNextTransmission->isRequest() || mNextTransmission->method()==ACK)
   {
      delete mNextTransmission;
      mNextTransmission=0;
   }
}

void
TransactionState::sendToTU(TransactionMessage* msg)
{
   SipMessage* sipMsg = dynamic_cast<SipMessage*>(msg);
   if (sipMsg && sipMsg->isResponse() && mDnsResult)
   {
      // whitelisting rules.
      switch (sipMsg->const_header(h_StatusLine).statusCode())
      {
         case 503:
            // blacklist last target.
            // .bwc. If there is no Retry-After, we do not blacklist
            // (see RFC 3261 sec 21.5.4 para 1)
            if(sipMsg->exists(resip::h_RetryAfter) && 
               sipMsg->const_header(resip::h_RetryAfter).isWellFormed())
            {
               unsigned int relativeExpiry= sipMsg->const_header(resip::h_RetryAfter).value();
               
               if(relativeExpiry!=0)
               {
                  mDnsResult->blacklistLast(resip::Timer::getTimeMs()+relativeExpiry*1000);
               }
            }
            
            break;
         case 408:
            if(!sipMsg->isFromWire() && (mState == Trying || mState==Calling))  // only greylist if internally generated and we haven't received any responses yet
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

   CongestionManager::RejectionBehavior behavior=CongestionManager::NORMAL;
   behavior=mController.mTuSelector.getRejectionBehavior(mTransactionUser);

   if(behavior!=CongestionManager::NORMAL)
   {
      if(sipMsg)
      {
         resip_assert(sipMsg->isExternal());
         if(sipMsg->isRequest())
         {
            // .bwc. This could be an initial request, or an ACK/200.
            if(sipMsg->method()==ACK)
            {
               // ACK/200 is a continuation of old work. We only reject if
               // we're really hosed.
               if(behavior==CongestionManager::REJECTING_NON_ESSENTIAL)
               {
                  delete msg;
                  return;
               }
            }
            else
            {
               // .bwc. This is new work. Reject.
               SipMessage* response(Helper::makeResponse(*sipMsg, 503));
               delete sipMsg;
               
               UInt16 retryAfter=mController.mTuSelector.getExpectedWait(mTransactionUser);
               response->header(h_RetryAfter).value()=retryAfter;
               response->setFromTU();
               if(mMethod==INVITE)
               {
                  processServerInvite(response);
               }
               else
               {
                  processServerNonInvite(response);
               }
               return;
            }
         }
         else
         {
            // .bwc. This could be a response from the wire, or an internally
            // generated pseudo-response. This is always a continuation of
            // old work.
            if(behavior==CongestionManager::REJECTING_NON_ESSENTIAL &&
               mTransactionUser &&
               !mTransactionUser->responsesMandatory())
            {
               delete sipMsg;
               return;
            }
         }
      }
      else
      {
         // .bwc. This is some sort of timer, or other message. If we don't know 
         // any better, we need to assume this is essential for the safe
         // operation of the TU.
      }
   }
   
   TransactionState::sendToTU(mTransactionUser, mController, msg);
}

void
TransactionState::sendToTU(TransactionUser* tu, TransactionController& controller, TransactionMessage* msg) 
{   
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
   if (isClient())
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
   if (isClient())
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
      int c = sip->const_header(h_StatusLine).responseCode();
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

bool
TransactionState::isTcpConnectState(TransactionMessage* msg) const
{
    return dynamic_cast<TcpConnectState*>(msg) != 0;
}

bool 
TransactionState::isAbandonServerTransaction(TransactionMessage* msg) const
{
   return dynamic_cast<AbandonServerTransaction*>(msg) != 0;
}

bool 
TransactionState::isCancelClientTransaction(TransactionMessage* msg) const
{
   return dynamic_cast<CancelClientInviteTransaction*>(msg) != 0;
}


const Data&
TransactionState::tid(SipMessage* sip) const
{
   resip_assert(0);
   resip_assert (mMachine != Stateless || (mMachine == Stateless && !mId.empty()));
   resip_assert (mMachine == Stateless || (mMachine != Stateless && sip));
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

bool 
TransactionState::isClient() const
{
   switch(mMachine)
   {
      case ClientNonInvite:
      case ClientInvite:
      case ClientStale:
      case Stateless:
         return true;
      case ServerNonInvite:
      case ServerInvite:
      case ServerStale:
         return false;
      default:
         resip_assert(0);
   }
   return false;
}

EncodeStream& 
resip::operator<<(EncodeStream& strm, const resip::TransactionState& state)
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
 * vi: set shiftwidth=3 expandtab:
 */
