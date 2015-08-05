
#include "resip/stack/Contents.hxx"
#include "resip/dum/BaseCreator.hxx"
#include "resip/dum/ClientInviteSession.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/DialogEventStateManager.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/DumTimeout.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/ServerInviteSession.hxx"
#include "resip/dum/ServerSubscription.hxx"
#include "resip/dum/UsageUseException.hxx"
#include "resip/dum/DumHelper.hxx"
#include "resip/stack/SipFrag.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "rutil/compat.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

ClientInviteSession::ClientInviteSession(DialogUsageManager& dum,
                                         Dialog& dialog,
                                         SharedPtr<SipMessage> request,
                                         const Contents* initialOffer,
                                         DialogUsageManager::EncryptionLevel level,
                                         ServerSubscriptionHandle serverSub) :
   InviteSession(dum, dialog),
   mStaleCallTimerSeq(1),
   mCancelledTimerSeq(1),
   mServerSub(serverSub),
   mAllowOfferInPrack(false)
{
   resip_assert(request->isRequest());
   if(initialOffer)  
   {
      mProposedLocalOfferAnswer = auto_ptr<Contents>(initialOffer->clone());
      mProposedEncryptionLevel = level;
   }
   *mLastLocalSessionModification = *request;  // Copy message, so that modifications to mLastLocalSessionModification don't effect creator->getLastRequest

   mState=UAC_Start;
}

ClientInviteSessionHandle
ClientInviteSession::getHandle()
{
   return ClientInviteSessionHandle(mDum, getBaseHandle().getId());
}

const Contents&
ClientInviteSession::getEarlyMedia() const
{
   return *mEarlyMedia;
}

void
ClientInviteSession::provideOffer(const Contents& offer, DialogUsageManager::EncryptionLevel level, const Contents* alternative)
{
   InfoLog (<< toData(mState) << ": provideOffer");

   switch(mState)
   {
      case UAC_EarlyWithAnswer:
         if(mAllowOfferInPrack)
         {
            // This flag is enabled when we are about to send our first PRACK.  We are
            // allowed to send an offer in our first PRACK request, so don't use UPDATE in 
            // this case.
            //  Remember proposed local offferAnswer.
            mProposedLocalOfferAnswer = InviteSession::makeOfferAnswer(offer, alternative);
            mProposedEncryptionLevel = level;
         }
         else
         {
            transition(UAC_SentUpdateEarly);

            //  Creates an UPDATE request with application supplied offer.
            mDialog.makeRequest(*mLastLocalSessionModification, UPDATE);
            InviteSession::setOfferAnswer(*mLastLocalSessionModification, offer);

            //  Remember proposed local offferAnswer.
            mProposedLocalOfferAnswer = InviteSession::makeOfferAnswer(offer, alternative);
            mProposedEncryptionLevel = level;

            //  Send the req and do state transition.
            DumHelper::setOutgoingEncryptionLevel(*mLastLocalSessionModification, mProposedEncryptionLevel);
            send(mLastLocalSessionModification);
         }
         break;

      case UAC_SentAnswer:
         // just queue it for later
         transition(UAC_QueuedUpdate);
         mProposedLocalOfferAnswer = InviteSession::makeOfferAnswer(offer, alternative);
         mProposedEncryptionLevel = level;
         break;

      case UAC_Start:
      case UAC_Early:
      case UAC_EarlyWithOffer:
      case UAC_Answered:
      case UAC_SentUpdateEarly:
      case UAC_ReceivedUpdateEarly:
      case UAC_Cancelled:
      case UAC_QueuedUpdate:
      case Terminated:
         WarningLog (<< "Incorrect state to provideOffer: " << toData(mState));
         throw DialogUsage::Exception("Can't provide an offer", __FILE__,__LINE__);
         break;

      default:
         InviteSession::provideOffer(offer, level, alternative);
         break;
   }
}

void
ClientInviteSession::provideOffer (const Contents& offer)
{
   this->provideOffer(offer, mCurrentEncryptionLevel, 0);
}

void
ClientInviteSession::provideAnswer (const Contents& answer)
{
   InfoLog (<< toData(mState) << ": provideAnswer");

   switch(mState)
   {
      case UAC_EarlyWithOffer:
      {
         transition(UAC_SentAnswer);

         //  Remember proposed local offerAnswer.
         mCurrentRemoteOfferAnswer = mProposedRemoteOfferAnswer;
         mCurrentLocalOfferAnswer = InviteSession::makeOfferAnswer(answer);

         //  Creates a PRACK request with application supplied answer
         sendPrack(answer, mCurrentEncryptionLevel);
         break;
      }

      case UAC_Answered:
      {
         transition(Connected);
         sendAck(&answer);

         mCurrentRemoteOfferAnswer = mProposedRemoteOfferAnswer;
         mCurrentLocalOfferAnswer = InviteSession::makeOfferAnswer(answer);
         break;
      }
      case UAC_ReceivedUpdateEarly:
      {
         transition(UAC_EarlyWithAnswer); //.dcm. earlyWithAnwer is a strange
                                          //name...maybe earlyEstablished?
         //this sequence is repeated in many places...due for refactoring.
         //see ReceivedUpdate handling in InviteSession.
         //?dcm? are session timers allowed in the early dialog?

         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, *mLastRemoteSessionModification, 200);
         InviteSession::setOfferAnswer(*response, answer, 0);
         mCurrentLocalOfferAnswer = InviteSession::makeOfferAnswer(answer);
         mCurrentRemoteOfferAnswer = mProposedRemoteOfferAnswer;
         InfoLog (<< "Sending " << response->brief());
         DumHelper::setOutgoingEncryptionLevel(*response, mCurrentEncryptionLevel);
         send(response);
         break;
      }
        
      case UAC_Start:
      case UAC_Early:
      case UAC_EarlyWithAnswer:
      case UAC_SentUpdateEarly:
      case UAC_SentAnswer:
      case UAC_Cancelled:
      case UAC_QueuedUpdate:
      case Terminated:
         WarningLog (<< "Incorrect state to provideAnswer: " << toData(mState));
         throw DialogUsage::Exception("Can't provide an answer", __FILE__,__LINE__);
         break;

      default:
         InviteSession::provideAnswer(answer);
         break;
   }
}

void
ClientInviteSession::end()
{
   end(NotSpecified);
}

void
ClientInviteSession::end(const Data& userReason)
{
   mUserEndReason = userReason;
   end(InviteSession::UserSpecified);
}

void
ClientInviteSession::end(EndReason reason)
{
   InfoLog (<< toData(mState) << ": end");
   if (mEndReason == NotSpecified)
   {
      mEndReason = reason;   
   }

   switch(mState)
   {
      case UAC_Early:
      case UAC_EarlyWithOffer:
      case UAC_EarlyWithAnswer:
      case UAC_Answered:
      case UAC_SentUpdateEarly:
      case UAC_ReceivedUpdateEarly:
      case UAC_SentAnswer:
      case UAC_QueuedUpdate:
      case UAC_Cancelled: // !jf! possibly incorrect to always BYE in UAC_Cancelled
      {
         SharedPtr<SipMessage> msg = sendBye();
         transition(Terminated);
         mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::LocalBye, msg.get()); 
         break;
      }

      case UAC_Start:
         WarningLog (<< "Try to end when in state=" << toData(mState));
         resip_assert(0);
         break;

      case Terminated:
         // no-op
         break;

      default:
         InviteSession::end(reason);
         break;
   }
}

void
ClientInviteSession::reject (int statusCode, WarningCategory *warning)
{
   InfoLog (<< toData(mState) << ": reject(" << statusCode << ")");

   switch(mState)
   {
      case UAC_ReceivedUpdateEarly:
      {
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, *mLastRemoteSessionModification, statusCode);
         if(warning)
         {
            response->header(h_Warnings).push_back(*warning);
         }

         //  Send the req and do state transition.
         send(response);
         transition(UAC_EarlyWithAnswer);
         break;
      }

      case UAC_Answered:
      {
         // We received an offer in a 2xx response, and we want to reject it
         // ACK with no body, then send bye
         sendAck();
         SharedPtr<SipMessage> msg = sendBye();
         transition(Terminated);
         mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::LocalBye, msg.get()); 
         break;
      }

      case UAC_Start:
      case UAC_Early:
      case UAC_EarlyWithOffer:
      case UAC_EarlyWithAnswer:
      case UAC_SentUpdateEarly:
      case UAC_SentAnswer:
      case UAC_Cancelled:
         WarningLog (<< "Try to reject when in state=" << toData(mState));
         resip_assert(0);
         break;

      default:
         InviteSession::reject(statusCode, warning);
         break;
   }
}

void
ClientInviteSession::cancel()
{
   switch(mState)
   {
      case UAC_Early:
      case UAC_EarlyWithOffer:
      case UAC_EarlyWithAnswer:
      case UAC_SentUpdateEarly:
      case UAC_ReceivedUpdateEarly:
      case UAC_SentAnswer:
         InfoLog (<< toData(mState) << ": cancel");
         startCancelTimer();
         transition(UAC_Cancelled);
         break;

      case UAC_Cancelled:
      case Terminated:
         // no-op already cancelled or ended
         break;

      default:
         resip_assert(0);
         break;
   }
}

void
ClientInviteSession::onForkAccepted()
{
   switch(mState)
   {
      case UAC_Early:
      case UAC_EarlyWithOffer:
      case UAC_EarlyWithAnswer:
      case UAC_SentUpdateEarly:
      case UAC_ReceivedUpdateEarly:
         InfoLog (<< toData(mState) << ": onForkAccepted");
         // !jf! should we avoid creating another timer here? I don't think it
         // matters. Default timer is for 32secs. This is here to handle the
         // cleanup on forked INVITEs that have sent a provisional response but
         // don't ever receive a final response. 
         mDum.addTimerMs(DumTimeout::WaitingForForked2xx, Timer::TH, getBaseHandle(), 1); 
         break;
      default:
         // If the dialog is already set up (or cancelled) we disregard. 
         break;
   }
}

void
ClientInviteSession::startCancelTimer()
{
   InfoLog (<< toData(mState) << ": startCancelTimer");
   mDum.addTimerMs(DumTimeout::Cancelled, Timer::TH, getBaseHandle(), ++mCancelledTimerSeq);
}

void
ClientInviteSession::startStaleCallTimer()
{
   InfoLog (<< toData(mState) << ": startStaleCallTimer");
   unsigned long when = mDialog.mDialogSet.getUserProfile()->getDefaultStaleCallTime();
   
   mDum.addTimer(DumTimeout::StaleCall, 
                 when, 
                 getBaseHandle(), 
                 ++mStaleCallTimerSeq);
}

void
ClientInviteSession::sendSipFrag(const SipMessage& msg)
{
   if (mServerSub.isValid())
   {
      if (msg.isResponse() && mState >= UAC_Start && mState <= UAC_Cancelled)
      {
         int code = msg.header(h_StatusLine).statusCode();
         if (code > 100)
         {
            SipFrag contents;
            contents.message().header(h_StatusLine) = msg.header(h_StatusLine);
            if(mDialog.mDialogSet.getUserProfile()->getExtraHeadersInReferNotifySipFragEnabled())
            {
               contents.message().header(h_Vias) = msg.header(h_Vias);
               contents.message().header(h_From) = msg.header(h_From);
               contents.message().header(h_To) = msg.header(h_To);
               contents.message().header(h_CallId) = msg.header(h_CallId);
               contents.message().header(h_CSeq) = msg.header(h_CSeq);
               contents.message().header(h_Contacts) = msg.header(h_Contacts);
            }
            if (code < 200)
            {
               mServerSub->send(mServerSub->update(&contents));
            }
            else
            {
               mServerSub->end(NoResource, &contents);
            }
         }
      }
   }
}

void
ClientInviteSession::dispatch(const SipMessage& msg)
{
  try
  {
     if (msg.isRequest())
     {
        if (msg.header(h_RequestLine).method() == INFO)
        {
           InviteSession::dispatchInfo(msg);
           return;
        }
        if (msg.header(h_RequestLine).method() == MESSAGE)
        {
           InviteSession::dispatchMessage(msg);
           return;
        }
     }

     if (isBadRseq(msg))
     {
        return;
     }

     sendSipFrag(msg);
     switch(mState)
     {
        case UAC_Start:
           dispatchStart(msg);
           break;
        case UAC_Early:
           dispatchEarly(msg);
           break;
        case UAC_EarlyWithOffer:
           dispatchEarlyWithOffer(msg);
           break;
        case UAC_EarlyWithAnswer:
           dispatchEarlyWithAnswer(msg);
           break;
        case UAC_Answered:
           dispatchAnswered(msg);
           break;
        case UAC_SentUpdateEarly:
           dispatchSentUpdateEarly(msg);
           break;
        case UAC_SentUpdateEarlyGlare:
           dispatchSentUpdateEarlyGlare(msg);
           break;
        case UAC_ReceivedUpdateEarly:
           dispatchReceivedUpdateEarly(msg);
           break;
        case UAC_SentAnswer:
           dispatchSentAnswer(msg);
           break;
        case UAC_QueuedUpdate:
           dispatchQueuedUpdate(msg);
           break;
        case UAC_Cancelled:
           dispatchCancelled(msg);
           break;
        default:
           InviteSession::dispatch(msg);
           break;
     }
  }
  catch (BaseException& e)
  {
     WarningLog (<< "Caught: " << e);
     onFailureAspect(getHandle(), msg);
     end(NotSpecified); 
  }
}

void
ClientInviteSession::dispatch(const DumTimeout& timer)
{
   if (timer.type() == DumTimeout::Cancelled)
   {
      if(timer.seq() == mCancelledTimerSeq)
      {
         if (mServerSub.isValid())
         {
            SipMessage response;
            mDialog.makeResponse(response, *mLastLocalSessionModification, 487);
            sendSipFrag(response);
         }
         transition(Terminated);
         mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::LocalCancel);
         mDum.destroy(this);
      }
   }
   else if (timer.type() == DumTimeout::StaleCall)
   {
      if(timer.seq() == mStaleCallTimerSeq)
      {
         mDum.mInviteSessionHandler->onStaleCallTimeout(getHandle());
         mDum.mInviteSessionHandler->terminate(getHandle());
      }
   }
   else if (timer.type() == DumTimeout::WaitingForForked2xx)
   {
      transition(Terminated);
      mDum.mInviteSessionHandler->onForkDestroyed(getHandle());
      mDum.destroy(this);
   }
   else if (timer.type() == DumTimeout::Glare)
   {
      if (mState == UAC_SentUpdateEarlyGlare)
      {
         transition(UAC_SentUpdateEarly);
         InfoLog (<< "Retransmitting the UPDATE (glare condition timer)");
         mDialog.makeRequest(*mLastLocalSessionModification, UPDATE);  // increments CSeq
         send(mLastLocalSessionModification);
      }
      else
      {
         InviteSession::dispatch(timer);
      }
   }
   else
   {
      InviteSession::dispatch(timer);
   }
}

void
ClientInviteSession::handleRedirect (const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   transition(Terminated);

   if (mDum.mDialogEventStateManager)
   {
      mDum.mDialogEventStateManager->onTerminated(mDialog, msg, InviteSessionHandler::Rejected);
   }

   handler->onRedirected(getHandle(), msg);
   mDum.destroy(this);
}

void
ClientInviteSession::handleProvisional(const SipMessage& msg)
{
   resip_assert(msg.isResponse());
   resip_assert(msg.header(h_StatusLine).statusCode() < 200);
   resip_assert(msg.header(h_StatusLine).statusCode() > 100);

   //.dcm. Kept the following checks here rather than discardMessage as the
   // state machine can be affected(termination).

   // !dcm! should we really end the InviteSession or should be discard the 1xx instead?
   // Check CSeq in 1xx against original INVITE request
   if (msg.header(h_CSeq).sequence() != mDialog.mDialogSet.getCreator()->getLastRequest()->header(h_CSeq).sequence())
   {
      InfoLog (<< "Failure:  CSeq doesn't match invite: " << msg.brief());
      onFailureAspect(getHandle(), msg);
      end(NotSpecified);
      return;
   }
   else if (isReliable(msg))
   {
      if (!msg.exists(h_RSeq))
      {
         InfoLog (<< "Failure:  No RSeq in 1xx: " << msg.brief());
         onFailureAspect(getHandle(), msg);
         end(NotSpecified);
         return;
      }
   }

   startStaleCallTimer();
   onProvisionalAspect(getHandle(), msg);
}

void
ClientInviteSession::handleFinalResponse(const SipMessage& msg)
{
   resip_assert(msg.isResponse());
   resip_assert(msg.header(h_StatusLine).statusCode() >= 200);
   resip_assert(msg.header(h_StatusLine).statusCode() < 300);

   handleSessionTimerResponse(msg);
   storePeerCapabilities(msg);
   ++mStaleCallTimerSeq;  // disable stale call timer
}

void
ClientInviteSession::handle1xxOffer(const SipMessage& msg, const Contents& offer)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;

   handleProvisional(msg);
   mProposedRemoteOfferAnswer = InviteSession::makeOfferAnswer(offer);
   mCurrentEncryptionLevel = getEncryptionLevel(msg);
   handler->onOffer(getSessionHandle(), msg, offer);
}

void
ClientInviteSession::handle1xxAnswer(const SipMessage& msg, const Contents& answer)
{
   setCurrentLocalOfferAnswer(msg);
   mCurrentEncryptionLevel = getEncryptionLevel(msg);
   mCurrentRemoteOfferAnswer = InviteSession::makeOfferAnswer(answer);

   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   handleProvisional(msg);

   // flag to let handle1xxAnswer know that it is OK to send an offer in the 
   // first PRACK (for a provisional with SDP answer) and to let providerOffer know 
   // that the offer will be going in an PRACK and not in an update.  Flag is not 
   // needed after handle1xxAnswer is called so it is reset.
   mAllowOfferInPrack = true;  

   handler->onAnswer(getSessionHandle(), msg, answer);

   // Reset flag - no longer needed
   mAllowOfferInPrack = false;

   // If offer is provided in onAnswer callback then send offer in PRACK
   if(mProposedLocalOfferAnswer.get())
   {
      sendPrack(*mProposedLocalOfferAnswer.get(), mProposedEncryptionLevel);
   }
   else
   {
      sendPrackIfNeeded(msg);
   }
}

// will not include SDP (this is a subsequent 1xx)
void
ClientInviteSession::sendPrackIfNeeded(const SipMessage& msg)
{
   resip_assert(msg.isResponse());
   resip_assert(msg.header(h_StatusLine).statusCode() < 200);
   resip_assert(msg.header(h_StatusLine).statusCode() > 100);
   
   if (isReliable(msg))
   {
      SharedPtr<SipMessage> prack(new SipMessage);
      mDialog.makeRequest(*prack, PRACK);
      prack->header(h_RAck) = mRelRespInfo;
      send(prack);
   }
}

// This version is used to send an answer to the UAS in PRACK from EarlyWithOffer 
// state. Assumes that it is the first PRACK. Subsequent PRACK will not have SDP
// Also used to send an offer in the first PRACK if the 18x included an
// answer.
void
ClientInviteSession::sendPrack(const Contents& offerAnswer, DialogUsageManager::EncryptionLevel encryptionLevel)
{
   SharedPtr<SipMessage> prack(new SipMessage);
   mDialog.makeRequest(*prack, PRACK);
   prack->header(h_RAck)= mRelRespInfo;
   
   InviteSession::setOfferAnswer(*prack, offerAnswer);

   DumHelper::setOutgoingEncryptionLevel(*prack, encryptionLevel);
   send(prack);
}

void
ClientInviteSession::dispatchStart (const SipMessage& msg)
{
   resip_assert(msg.isResponse());
   resip_assert(msg.header(h_StatusLine).statusCode() > 100);
   resip_assert(msg.header(h_CSeq).method() == INVITE);

   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   InviteSession::Event event = toEvent(msg, offerAnswer.get());

   switch (event)
   {
      case On1xx:
         transition(UAC_Early);
         handler->onNewSession(getHandle(), InviteSession::None, msg);
         if(!isTerminated())  
         {
            handleProvisional(msg);
            sendPrackIfNeeded(msg);  // may wish to move emprty PRACK handling
                                     // outside the state machine            
         }
         break;

      case On1xxEarly:  // only unreliable
         //!dcm! according to draft-ietf-sipping-offeranswer there can be a non
         // reliable 1xx followed by a reliable 1xx.  Also, the intial 1xx
         // doesn't have to have an offer. However, DUM will only generate
         // reliabled 1xx responses when 100rel is available.

         transition(UAC_Early);
         mEarlyMedia = InviteSession::makeOfferAnswer(*offerAnswer);
         handler->onNewSession(getHandle(), InviteSession::None, msg);
         if(!isTerminated())  
         {
            handleProvisional(msg);
            if(!isTerminated())  
            {
               handler->onEarlyMedia(getHandle(), msg, *offerAnswer);
            }
         }
         break;

      case On1xxOffer:
         transition(UAC_EarlyWithOffer);
         handler->onNewSession(getHandle(), InviteSession::Offer, msg);
         if(!isTerminated())  
         {
            handle1xxOffer(msg, *offerAnswer);
         }
         break;

      case On1xxAnswer:
         transition(UAC_EarlyWithAnswer);
         handler->onNewSession(getHandle(), InviteSession::Answer, msg);
         if(!isTerminated())  
         {
            handle1xxAnswer(msg, *offerAnswer);
         }
         break;

      case On2xxOffer:
         transition(UAC_Answered);
         handleFinalResponse(msg);
         mProposedRemoteOfferAnswer = InviteSession::makeOfferAnswer(*offerAnswer);
         handler->onNewSession(getHandle(), InviteSession::Offer, msg);
         resip_assert(mProposedLocalOfferAnswer.get() == 0);
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         if(!isTerminated())  
         {
            handler->onOffer(getSessionHandle(), msg, *offerAnswer);
            if(!isTerminated())   //?jf? can this be terminated here but not above? .slg. yes application can call end()
            {
               onConnectedAspect(getHandle(), msg);  
            }
         }
         break;

      case On2xxAnswer:
         transition(Connected);
         sendAck();
         handleFinalResponse(msg);
         setCurrentLocalOfferAnswer(msg);
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         mCurrentRemoteOfferAnswer = InviteSession::makeOfferAnswer(*offerAnswer);
         handler->onNewSession(getHandle(), InviteSession::Answer, msg);
         if(!isTerminated())  // onNewSession callback may call end() or reject()
         {
            handler->onAnswer(getSessionHandle(), msg, *offerAnswer);
            if(!isTerminated())  // onAnswer callback may call end() or reject()
            {
               onConnectedAspect(getHandle(), msg);
            }
         }
         break;

      case On2xx:
      {
         sendAck();
         sendBye();
         InfoLog (<< "Failure:  2xx with no answer: " << msg.brief());
         transition(Terminated);
         onFailureAspect(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         break;
      }

      case OnRedirect: // Redirects are handled by the DialogSet - if a 3xx gets here then it's because the redirect was intentionaly not handled and should be treated as an INVITE failure      
      case OnInviteFailure:
      case OnGeneralFailure:
      case On422Invite:
      case On487Invite:
      case On491Invite:
         InfoLog (<< "Failure:  error response: " << msg.brief());
         transition(Terminated);
         onFailureAspect(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         mDum.destroy(this);
         break;

      case OnBye:
         dispatchBye(msg);
         break;

      default:
         // !kh!
         // should not assert here for peer sent us garbage.
         WarningLog (<< "Don't know what this is : " << msg);
         break;
   }
}

void
ClientInviteSession::dispatchEarly (const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   switch (toEvent(msg, offerAnswer.get()))
   {
      case On1xx:
         transition(UAC_Early);
         handleProvisional(msg);
         sendPrackIfNeeded(msg);
         break;

      case On1xxEarly: // only unreliable
         transition(UAC_Early);
         handleProvisional(msg);
         if(!isTerminated())  
         {
            mEarlyMedia = InviteSession::makeOfferAnswer(*offerAnswer);
            handler->onEarlyMedia(getHandle(), msg, *offerAnswer);
         }
         break;

      case On1xxOffer:
         transition(UAC_EarlyWithOffer);
         handle1xxOffer(msg, *offerAnswer);
         break;

      case On1xxAnswer:
         transition(UAC_EarlyWithAnswer);
         handle1xxAnswer(msg, *offerAnswer);
         break;

      case On2xxOffer:
         transition(UAC_Answered);
         handleFinalResponse(msg);

         resip_assert(mProposedLocalOfferAnswer.get() == 0);
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         mProposedRemoteOfferAnswer = InviteSession::makeOfferAnswer(*offerAnswer);

         handler->onOffer(getSessionHandle(), msg, *offerAnswer);
         if(!isTerminated())  
         {
            onConnectedAspect(getHandle(), msg);   
         }
         break;

      case On2xxAnswer:
         transition(Connected);
         sendAck();
         handleFinalResponse(msg);
         setCurrentLocalOfferAnswer(msg);
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         mCurrentRemoteOfferAnswer = InviteSession::makeOfferAnswer(*offerAnswer);
         handler->onAnswer(getSessionHandle(), msg, *offerAnswer);
         if(!isTerminated())  // onNewSession callback may call end() or reject()
         {
            onConnectedAspect(getHandle(), msg);
         }
         break;

      case On2xx:
      {
         sendAck();
         sendBye();
         InfoLog (<< "Failure:  2xx with no answer: " << msg.brief());
         transition(Terminated);
         onFailureAspect(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         break;
      }

      case OnRedirect: // Redirects are handled by the DialogSet - if a 3xx gets here then it's because the redirect was intentionaly not handled and should be treated as an INVITE failure      
      case OnInviteFailure:
      case OnGeneralFailure:
      case On422Invite:
      case On487Invite:
         InfoLog (<< "Failure:  error response: " << msg.brief());
         transition(Terminated);
         onFailureAspect(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         mDum.destroy(this);
         break;

      case OnBye:
         dispatchBye(msg);
         break;

      case OnUpdateOffer:
         if(mProposedLocalOfferAnswer.get())
         {
            WarningLog (<< "Invalid UPDATE with offer received in early state with pending offer: " << msg.brief());
            SharedPtr<SipMessage> response(new SipMessage);
            mDialog.makeResponse(*response, msg, 500);  // RFC3311 - section 5.2
            InfoLog (<< "Sending " << response->brief());
            send(response);
         }
         else
         {
            *mLastRemoteSessionModification = msg;
            transition(UAC_ReceivedUpdateEarly);
            mCurrentEncryptionLevel = getEncryptionLevel(msg);
            mProposedRemoteOfferAnswer = InviteSession::makeOfferAnswer(*offerAnswer);
            handler->onOffer(getSessionHandle(), msg, *offerAnswer);
         }
         break;

      case OnUpdate:
      {
         // ?slg? no offerAnswer in update - just respond immediately - do we need a callback?
         SharedPtr<SipMessage> response(new SipMessage);
         *mLastRemoteSessionModification = msg;
         mDialog.makeResponse(*response, msg, 200);
         send(response);
         break;
      }
     
      case On200Prack:
         break;

      default:
         // !kh!
         // should not assert here for peer sent us garbage.
         WarningLog (<< "Don't know what this is : " << msg);
         break;
   }
}

void
ClientInviteSession::dispatchAnswered (const SipMessage& msg)
{
   //InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   switch (toEvent(msg, offerAnswer.get()))
   {
      case On1xx:
      case On1xxEarly:
      case On1xxOffer:
         // late, so ignore
         break;

      case On2xxOffer:
      case On2xx:
      case On2xxAnswer:
         // retransmission
         break;

      case OnRedirect:
         // too late
         break;

      // .slg. This doesn't really make sense (after a 2xx)
      case OnGeneralFailure:
      case On422Invite:
         break;
#if 0 // !jf! don't think this is right
      {
         sendBye();
         InfoLog (<< "Failure:  error response: " << msg.brief());
         transition(Terminated);
         onFailureAspect(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         mDum.destroy(this);
         break;
      }
#endif
      case OnBye:
         dispatchBye(msg);
         break;

      default:
         // !kh!
         // should not assert here for peer sent us garbage.
         WarningLog (<< "Don't know what this is : " << msg);         
         break;
   }

}

void
ClientInviteSession::dispatchEarlyWithOffer (const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   switch (toEvent(msg, offerAnswer.get()))
   {
      case On1xx: // must be reliable
         handleProvisional(msg);
         sendPrackIfNeeded(msg);
         break;

      case On2xx:
      case On2xxAnswer:
         sendAck();
         sendBye();
         InfoLog (<< "Failure:  no answer sent: " << msg.brief());
         transition(Terminated);
         onFailureAspect(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         break;

      case OnRedirect: // Redirects are handled by the DialogSet - if a 3xx gets here then it's because the redirect was intentionaly not handled and should be treated as an INVITE failure      
      case OnInviteFailure:
      case OnGeneralFailure:
      case On422Invite:
      case On487Invite:
      case On491Invite:
         InfoLog (<< "Failure:  error response: " << msg.brief());
         transition(Terminated);
         onFailureAspect(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         mDum.destroy(this);
         break;

      case OnBye:
         dispatchBye(msg);
         break;

      case OnUpdateOffer:
      {
         WarningLog (<< "Invalid UPDATE with offer received in early state with pending offer: " << msg.brief());
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, msg, 500);  // RFC3311 - section 5.2
         InfoLog (<< "Sending " << response->brief());
         send(response);
         break;
      }

      case OnUpdate:
      {
         // ?slg? no offerAnswer in update - just respond immediately - do we need a callback?
         SharedPtr<SipMessage> response(new SipMessage);
         *mLastRemoteSessionModification = msg;
         mDialog.makeResponse(*response, msg, 200);
         send(response);
         break;
      }

      default:
         // !kh!
         // should not assert here for peer sent us garbage.
         WarningLog (<< "Don't know what this is : " << msg);
         break;
   }
}

void
ClientInviteSession::dispatchSentAnswer (const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   switch (toEvent(msg, offerAnswer.get()))
   {
      case On200Prack:
         transition(UAC_EarlyWithAnswer);
         break;

      case On2xx:
         transition(Connected);
         sendAck();
         handleFinalResponse(msg);
         onConnectedAspect(getHandle(), msg);
         break;

      case On2xxOffer:
         {
             if (*offerAnswer == *mCurrentRemoteOfferAnswer)
             {
                 InfoLog (<< "Ignoring illegal offer identical with current remote offer/answer");
                 transition(Connected);
                 sendAck();
                 handleFinalResponse(msg);
                 onConnectedAspect(getHandle(), msg);
                 break;
             }
         }
         // fall through to next label
      case On2xxAnswer:
      case On1xxAnswer:
      case On1xxOffer:
         sendAck();
         sendBye();
         InfoLog (<< "Failure:  illegal offer/answer: " << msg.brief());
         transition(Terminated);
         onFailureAspect(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         break;

      case On1xx:
         handleProvisional(msg);
         sendPrackIfNeeded(msg);
         break;

      case OnRedirect: // Redirects are handled by the DialogSet - if a 3xx gets here then it's because the redirect was intentionaly not handled and should be treated as an INVITE failure
      case OnInviteFailure:
      case OnGeneralFailure:
      case On422Invite:
      case On487Invite:
      case On491Invite:
         InfoLog (<< "Failure:  error response: " << msg.brief());
         transition(Terminated);
         onFailureAspect(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         mDum.destroy(this);
         break;

      case OnBye:
         dispatchBye(msg);
         break;

      default:
         // !kh!
         // should not assert here for peer sent us garbage.
         WarningLog (<< "Don't know what this is : " << msg);
         break;
   }
}

void
ClientInviteSession::dispatchQueuedUpdate (const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   switch (toEvent(msg, offerAnswer.get()))
   {
      case On200Prack:
         transition(UAC_SentUpdateEarly);
         {
            mDialog.makeRequest(*mLastLocalSessionModification, UPDATE);
            InviteSession::setOfferAnswer(*mLastLocalSessionModification, mProposedLocalOfferAnswer.get());

            DumHelper::setOutgoingEncryptionLevel(*mLastLocalSessionModification, mProposedEncryptionLevel);
            send(mLastLocalSessionModification);
         }
         break;

      case On2xx:
         transition(SentUpdate);
         {
            sendAck();

            SharedPtr<SipMessage> update(new SipMessage);
            mDialog.makeRequest(*update, UPDATE);
            InviteSession::setOfferAnswer(*update, mProposedLocalOfferAnswer.get());
            DumHelper::setOutgoingEncryptionLevel(*update, mProposedEncryptionLevel);
            send(update);
         }
         handleFinalResponse(msg);
         onConnectedAspect(getHandle(), msg);
         break;

      case On2xxOffer:
         {
             if (*offerAnswer == *mCurrentRemoteOfferAnswer)
             {
                 InfoLog (<< "Ignoring illegal offer identical with current remote offer/answer");
                 transition(Connected);
                 sendAck();
                 handleFinalResponse(msg);
                 onConnectedAspect(getHandle(), msg);
                 break;
             }
         }
         // fall through to next label
      case On2xxAnswer:
      case On1xxAnswer:
      case On1xxOffer:
         sendAck();
         sendBye();
         InfoLog (<< "Failure:  illegal offer/answer: " << msg.brief());
         transition(Terminated);
         onFailureAspect(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         break;

      case On1xx:
         handleProvisional(msg);
         sendPrackIfNeeded(msg);
         break;

      case OnRedirect: // Redirects are handled by the DialogSet - if a 3xx gets here then it's because the redirect was intentionaly not handled and should be treated as an INVITE failure
      case OnInviteFailure:
      case OnGeneralFailure:
      case On422Invite:
      case On487Invite:
      case On491Invite:
         InfoLog (<< "Failure:  error response: " << msg.brief());
         transition(Terminated);
         onFailureAspect(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         mDum.destroy(this);
         break;

      case OnBye:
         dispatchBye(msg);
         break;

      default:
         // !kh!
         // should not assert here for peer sent us garbage.
         WarningLog (<< "Don't know what this is : " << msg);
         break;
   }
}



void
ClientInviteSession::dispatchEarlyWithAnswer (const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   switch (toEvent(msg, offerAnswer.get()))
   {
      case On1xx:
         handleProvisional(msg);
         sendPrackIfNeeded(msg);
         break;

      case On1xxOffer:
         if(!isTerminated())  
         {
            transition(UAC_EarlyWithOffer);
            handle1xxOffer(msg, *offerAnswer);
         }
         break;

      case On2xx:
         transition(Connected);
         sendAck();
         handleFinalResponse(msg);
         onConnectedAspect(getHandle(), msg);
         break;

      case On2xxOffer:
         {
             if (*offerAnswer == *mCurrentRemoteOfferAnswer)
             {
                 InfoLog (<< "Ignoring illegal offer identical with current remote offer/answer");
                 transition(Connected);
                 sendAck();
                 handleFinalResponse(msg);
                 onConnectedAspect(getHandle(), msg);
                 break;
             }
         }
         // fall through to next label
      case On2xxAnswer:
         sendAck();
         sendBye();
         InfoLog (<< "Failure:  illegal offer/answer: " << msg.brief());
         transition(Terminated);
         onFailureAspect(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         break;

      case OnUpdateOffer:
         *mLastRemoteSessionModification = msg;
         transition(UAC_ReceivedUpdateEarly);
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         mProposedRemoteOfferAnswer = InviteSession::makeOfferAnswer(*offerAnswer);
         handler->onOffer(getSessionHandle(), msg, *offerAnswer);
         break;

      case OnUpdate:
      {
         // ?slg? no offerAnswer in update - just respond immediately - do we need a callback?
         SharedPtr<SipMessage> response(new SipMessage);
         *mLastRemoteSessionModification = msg;
         mDialog.makeResponse(*response, msg, 200);
         send(response);
         break;
      }

      case OnRedirect: // Redirects are handled by the DialogSet - if a 3xx gets here then it's because the redirect was intentionaly not handled and should be treated as an INVITE failure
      case OnInviteFailure:
      case OnGeneralFailure:
      case On422Invite:
      case On487Invite:
      case On491Invite:
         InfoLog (<< "Failure:  error response: " << msg.brief());
         transition(Terminated);
         onFailureAspect(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         mDum.destroy(this);
         break;

      case OnBye:
         dispatchBye(msg);
         break;

      case On200Prack:
         // We may have sent a PRACK with an offer (if provideOffer was called from onAnswer 
         // from the first reliable provisional) - if so this will have SDP we need to call onAnswer
         if(offerAnswer.get() && mProposedLocalOfferAnswer.get())
         {
            setCurrentLocalOfferAnswer(msg);
            mCurrentEncryptionLevel = getEncryptionLevel(msg);
            mCurrentRemoteOfferAnswer = InviteSession::makeOfferAnswer(*offerAnswer);
            handler->onAnswer(getSessionHandle(), msg, *offerAnswer);
         }
         break;

      default:
         // !kh!
         // should not assert here for peer sent us garbage.
         WarningLog (<< "Don't know what this is : " << msg);
         break;
   }
}

void
ClientInviteSession::dispatchSentUpdateEarly (const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   switch (toEvent(msg, offerAnswer.get()))
   {
      case On200Update:
         transition(UAC_EarlyWithAnswer);
         setCurrentLocalOfferAnswer(msg);
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         mCurrentRemoteOfferAnswer = InviteSession::makeOfferAnswer(*offerAnswer);
         handler->onAnswer(getSessionHandle(), msg, *offerAnswer);
         break;
         
      case OnUpdateOffer:
         {
            SharedPtr<SipMessage> response(new SipMessage);
            mDialog.makeResponse(*response, msg, 491);
            send(response);
         }
         break;
         
      case OnUpdate:
      {
         // ?slg? no offerAnswer in update - just respond immediately - do we need a callback?
         SharedPtr<SipMessage> response(new SipMessage);
         *mLastRemoteSessionModification = msg;
         mDialog.makeResponse(*response, msg, 200);
         send(response);
         break;
      }

      case On491Update:
         transition(UAC_SentUpdateEarlyGlare);
         start491Timer();
         break;

      case On2xx:
         transition(SentUpdate);         
         sendAck();
         break;
         
      case OnRedirect: // Redirects are handled by the DialogSet - if a 3xx gets
                       // here then it's because the redirect was intentionaly
                       // not handled and should be treated as an INVITE failure
      case OnInviteFailure:
      case OnGeneralFailure:
      case On422Invite:
      case On487Invite:
         InfoLog (<< "Failure:  error response: " << msg.brief());
         transition(Terminated);
         onFailureAspect(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         mDum.destroy(this);
         break;

      case On200Prack:
         break;

      default:
         WarningLog (<< "Don't know what this is : " << msg);
         break;
   }
}

void
ClientInviteSession::dispatchSentUpdateEarlyGlare (const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   switch (toEvent(msg, offerAnswer.get()))
   {
      case OnUpdateOffer:
         handler->onOfferRejected(getSessionHandle(), &msg);
         //will cause transition to UAC_ReceivedUpdateEarly
         dispatchEarlyWithAnswer(msg);
         break;
         
      case On2xx:
         //transition to connected state machine
         transition(SentUpdateGlare);         
         sendAck();
         break;
         //!dcm! TODO This block below is repeated many, many times...refactor
         //!into a method. Prob. represents the effective ClientInvite superstate.
      case OnRedirect: // Redirects are handled by the DialogSet - if a 3xx gets
                       // here then it's because the redirect was intentionaly
                       // not handled and should be treated as an INVITE failure
      case OnInviteFailure:
      case OnGeneralFailure:
      case On422Invite:
      case On487Invite:
         InfoLog (<< "Failure:  error response: " << msg.brief());
         transition(Terminated);
         onFailureAspect(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         mDum.destroy(this);
         break;

      default:
         WarningLog (<< "Don't know what this is : " << msg);
         break;
   }
}

void
ClientInviteSession::dispatchReceivedUpdateEarly(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   switch (toEvent(msg, offerAnswer.get()))
   {
      case OnUpdate:
      case OnUpdateOffer:
         // If we receive an UPDATE before we have generated a final response to a previous UPDATE on the 
         // same dialog, then we MUST return a 500 response with a Retry-After header (random duration 0-10 seconds)
         {
            SharedPtr<SipMessage> u500(new SipMessage);
            mDialog.makeResponse(*u500, msg, 500);
            u500->header(h_RetryAfter).value() = Random::getRandom() % 10;
            send(u500);
         }
         break;

      default:
         WarningLog (<< "Don't know what this is : " << msg);
         break;
   }
   WarningLog (<< "Ignoring message received in ReceivedUpdateEarly: " << msg);
}

void
ClientInviteSession::dispatchCancelled (const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   switch (toEvent(msg, offerAnswer.get()))
   {
      case OnGeneralFailure:
      case OnCancelFailure:
      case On487Invite:
      case OnRedirect:
      case On422Invite:
      case On491Invite:
      case OnInviteFailure:
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::LocalCancel, &msg);
         mDum.destroy(this);
         break;

      case On2xx:
      case On2xxOffer:
      case On2xxAnswer:
      {
         // this is the 2xx crossing the CANCEL case
         sendAck();
         sendBye();
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::LocalCancel, &msg);
         mCancelledTimerSeq++;
         break;
      }

      case OnBye:
         dispatchBye(msg);
         break;

      default:
         break;
   }
}

//true if 180rel should be ignored. Saves rseq as a side effect.
bool 
ClientInviteSession::isBadRseq(const SipMessage& msg)
{
   int code = msg.isResponse() ? msg.header(h_StatusLine).statusCode() : 0;
   if (msg.method() == INVITE && code > 100 && code < 200)
   {
      if (msg.exists(h_RSeq))
      {
         // store state about the provisional if reliable, so we can detect retransmissions
         unsigned int rseq = (unsigned int) msg.header(h_RSeq).value();
         unsigned int lastRseq = (unsigned int) mRelRespInfo.rSequence();
         
         if (rseq == lastRseq)
         {
            DebugLog(<< "Discarding reliable 1xx retranmission with rseq " << rseq);
            return true;
         }
         else if (lastRseq != 0 && rseq > lastRseq + 1)
         {
            DebugLog(<< "Discarding out of order reliable 1xx with rseq " << rseq);
            return true;
         }
         mRelRespInfo.rSequence() = rseq;
         mRelRespInfo.cSequence() = msg.header(h_CSeq).sequence();
         mRelRespInfo.method() = msg.header(h_CSeq).method();
      }
   }
   return false;
}

void 
ClientInviteSession::onConnectedAspect(ClientInviteSessionHandle c, const SipMessage& msg)
{
   if (mDum.mDialogEventStateManager)
   {
      mDum.mDialogEventStateManager->onConfirmed(mDialog, getSessionHandle());
   }
   mDum.mInviteSessionHandler->onConnected(c, msg);
}

void 
ClientInviteSession::onProvisionalAspect(ClientInviteSessionHandle c, const SipMessage& msg)
{
   if (mDum.mDialogEventStateManager)
   {
      mDum.mDialogEventStateManager->onEarly(mDialog, getSessionHandle());
   }
   mDum.mInviteSessionHandler->onProvisional(c, msg);
}

void 
ClientInviteSession::onFailureAspect(ClientInviteSessionHandle c, const SipMessage& msg)
{
   if (mDum.mDialogEventStateManager)
   {
      InviteSessionHandler::TerminatedReason reason = InviteSessionHandler::Rejected;
      if (msg.isResponse())
      {
         if (msg.header(h_StatusLine).responseCode() == 408)
         {
            reason = InviteSessionHandler::Timeout;
         }
         else if (msg.header(h_StatusLine).responseCode() / 100 == 5)
         {
            reason = InviteSessionHandler::Error;
         }
      }
      mDum.mDialogEventStateManager->onTerminated(mDialog, msg, reason);
   }
   mDum.mInviteSessionHandler->onFailure(c, msg);
}
   
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
