#include "resiprocate/MultipartMixedContents.hxx"
#include "resiprocate/SdpContents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/InviteSession.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/UsageUseException.hxx"
#include "resiprocate/os/Inserter.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Timer.hxx"
#include "resiprocate/os/Random.hxx"
#include "resiprocate/os/compat.hxx"

//#include "resiprocate/dum/ServerInviteSession.hxx"
//#include "resiprocate/dum/ClientInviteSession.hxx"

// Remove warning about 'this' use in initiator list - pointer is only stored
#if defined(WIN32)
#pragma warning( disable : 4355 ) // using this in base member initializer list
#endif

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM
#define THROW(msg)  throw DialogUsage::Exception(msg, __FILE__,__LINE__);

using namespace resip;
using namespace std;

InviteSession::InviteSession(DialogUsageManager& dum, Dialog& dialog)
   : DialogUsage(dum, dialog),
     mState(Undefined),
     mNitState(NitComplete),
     mCurrentRetransmit200(Timer::T1),
     mSessionInterval(0),
     mSessionRefresherUAS(false),
     mSessionTimerSeq(0),
     mDestroyer(this)
{
   DebugLog ( << "^^^ InviteSession::InviteSession " << this);
   assert(mDum.mInviteSessionHandler);
}

InviteSession::~InviteSession()
{
   DebugLog ( << "^^^ InviteSession::~InviteSession " << this);
   mDialog.mInviteSession = 0;
}


const SdpContents&
InviteSession::getLocalSdp() const
{
   return *mCurrentLocalSdp;
}

const SdpContents&
InviteSession::getRemoteSdp() const
{
   return *mCurrentRemoteSdp;
}

InviteSessionHandle
InviteSession::getSessionHandle()
{
   return InviteSessionHandle(mDum, getBaseHandle().getId());
}

bool
InviteSession::peerSupportsUpdateMethod() const
{
   // !jf!
   return false;
}

void
InviteSession::provideOffer(const SdpContents& offer)
{
   switch (mState)
   {
      case Connected:
      case WaitingToOffer:
         if (peerSupportsUpdateMethod())
         {
            mDialog.makeRequest(mLastSessionModification, UPDATE);
            transition(SentUpdate);
         }
         else
         {
            mDialog.makeRequest(mLastSessionModification, INVITE);
            transition(SentReinvite);
         }

         // !jf! should I check if value > 90? 
         mLastSessionModification.header(h_SessionExpires).value() = mSessionInterval;
         mLastSessionModification.header(h_SessionExpires).param(p_refresher) = Data(mSessionRefresherUAS ? "uas" : "uac");
         
         InfoLog (<< "Sending " << mLastSessionModification.brief());
         mProposedLocalSdp = InviteSession::makeSdp(offer);
         InviteSession::setSdp(mLastSessionModification, offer);
         mDum.send(mLastSessionModification);
         break;

      case Answered:
         // queue the offer to be sent after the ACK is received
         mProposedLocalSdp = InviteSession::makeSdp(offer);
         transition(WaitingToOffer);
         break;
         
      default:
         WarningLog (<< "Can't provideOffer when not in Connected state");
         throw DialogUsage::Exception("Can't provide an offer", __FILE__,__LINE__);
   }
}

void
InviteSession::provideAnswer(const SdpContents& answer)
{
   switch (mState)
   {
      case ReceivedReinvite:
         mDialog.makeResponse(mInvite200, mLastSessionModification, 200);
         mCurrentLocalSdp = InviteSession::makeSdp(answer);
         mCurrentRemoteSdp = mProposedRemoteSdp;
         InviteSession::setSdp(mInvite200, answer);
         transition(Connected);
         InfoLog (<< "Sending " << mInvite200.brief());
         mDum.send(mInvite200);
         startRetransmitTimer();
         break;
         
      case ReceivedUpdate: // same as ReceivedReinvite case.
      {
         SipMessage response;
         mDialog.makeResponse(response, mLastSessionModification, 200);
         mCurrentLocalSdp = InviteSession::makeSdp(answer);
         mCurrentRemoteSdp = mProposedRemoteSdp;
         InviteSession::setSdp(response, answer);
         transition(Connected);
         InfoLog (<< "Sending " << response.brief());
         mDum.send(response);
         break;
      }
      
      default:
         WarningLog (<< "Can't provideOffer when not in Connected state");
         throw DialogUsage::Exception("Can't provide an offer", __FILE__,__LINE__);
   }
}

void
InviteSession::end()
{
   switch (mState)
   {
      case Connected:
      {
         // !jf! do we need to store the BYE somewhere? 
         SipMessage bye;
         mDialog.makeRequest(bye, BYE);
         transition(Terminated);
         InfoLog (<< "Sending " << bye.brief());
         mDum.send(bye);
         break;
      }

      case SentUpdate:
         transition(Terminated);
         break;
         
      case SentReinvite:
         transition(WaitingToTerminate);
         break;

      case ReceivedUpdate:
      case ReceivedReinvite:
      case ReceivedReinviteNoOffer:
      {
         SipMessage response;
         mDialog.makeResponse(response, mLastSessionModification, 488);
         transition(Terminated);
         InfoLog (<< "Sending " << response.brief());
         mDum.send(response);

         SipMessage bye;
         mDialog.makeRequest(bye, BYE);
         InfoLog (<< "Sending " << bye.brief());
         mDum.send(bye);
         break;
      }

      case WaitingToTerminate:
      {
         SipMessage bye;
         mDialog.makeRequest(bye, BYE);
         transition(Terminated);
         InfoLog (<< "Sending " << bye.brief());
         mDum.send(bye);
         break;
      }
      
      case Terminated:
         // no-op.
         break;
         
      default:
         assert(0);
         break;
   }

   // Notify the application right away that the session is kaput but let dum
   // worry about the cleanup issues
   mDum.mInviteSessionHandler->onTerminated(getSessionHandle());
}

void
InviteSession::reject(int statusCode)
{
   switch (mState)
   {
      case ReceivedUpdate:
      case ReceivedReinvite:
      case ReceivedReinviteNoOffer:
      {
         SipMessage response;
         mDialog.makeResponse(response, mLastSessionModification, statusCode);
         transition(Connected);
         InfoLog (<< "Sending " << response.brief());
         mDum.send(response);
         break;
      }

      default:
         assert(0);
         break;
   }
}

void
InviteSession::targetRefresh(const NameAddr& localUri)
{
   // !jf! add interface to Dialog
   //mDialog.setLocalContact(localUri);
   provideOffer(*mCurrentLocalSdp);
}

void
InviteSession::refer(const NameAddr& referTo)
{
   SipMessage refer;
   mDialog.makeRequest(refer, REFER);
   refer.header(h_ReferTo) = referTo;
   mDum.send(refer);
}

void
InviteSession::refer(const NameAddr& referTo, InviteSessionHandle sessionToReplace)
{
   if (!sessionToReplace.isValid())
   {
      throw UsageUseException("Attempted to make a refer w/ and invalid replacement target", __FILE__, __LINE__);
   }

   SipMessage refer;
   mDialog.makeRequest(refer, REFER);

   refer.header(h_ReferTo) = referTo;
   CallId replaces;
   DialogId id = sessionToReplace->mDialog.getId();
   replaces.value() = id.getCallId();
   replaces.param(p_toTag) = id.getRemoteTag();
   replaces.param(p_fromTag) = id.getLocalTag();

   refer.header(h_ReferTo).uri().embedded().header(h_Replaces) = replaces;
   mDum.send(refer);
}

void
InviteSession::info(const Contents& contents)
{
   if (mNitState == NitComplete)
   {
      mNitState = NitProceeding;
      SipMessage info;
      mDialog.makeRequest(info, INFO);
      // !jf! handle multipart here
      info.setContents(&contents); 
      mDum.send(info);
   }
   else
   {
      throw UsageUseException("Cannot start a non-invite transaction until the previous one has completed",
                              __FILE__, __LINE__);
   }
}

void
InviteSession::dispatch(const SipMessage& msg)
{
   Destroyer::Guard guard(mDestroyer);
   const SdpContents* sdp = InviteSession::getSdp(msg);
   
   // !jf! do we need to handle 3xx here or is it handled elsewhere?
   switch (mState)
   {
      case Connected:
         dispatchConnected(msg, sdp);
         break;
      case SentUpdate:
         dispatchSentUpdate(msg, sdp);
         break;
      case SentReinvite:
         dispatchSentReinvite(msg, sdp);
         break;
      case SentUpdateGlare:
      case SentReinviteGlare:
         // The behavior is the same except for timer which is handled in dispatch(Timer)
         dispatchGlare(msg);
         break;
      case ReceivedUpdate:
      case ReceivedReinvite:
      case ReceivedReinviteNoOffer:
         dispatchReceivedUpdateOrReinvite(msg);
         break;
      case Answered:
         dispatchAnswered(msg);
         break;
      case WaitingToOffer:
         dispatchWaitingToOffer(msg, sdp);
         break;
      case WaitingToTerminate:
         dispatchWaitingToTerminate(msg);
         break;
      case Terminated:
         dispatchTerminated(msg);
         break;
      case Undefined:
      default:
         assert(0);
         break;
   }
}

void
InviteSession::dispatch(const DumTimeout& timeout)
{
   if (timeout.type() == DumTimeout::Retransmit200)
   {
      mDum.send(mInvite200);
      if (mCurrentRetransmit200)
      {
         mCurrentRetransmit200 *= 2;
         mDum.addTimerMs(DumTimeout::Retransmit200, resipMin(Timer::T2, mCurrentRetransmit200), getBaseHandle(),  timeout.seq());
      }
   }
   else if (timeout.type() == DumTimeout::WaitForAck)
   {
      mCurrentRetransmit200 = 0; // stop the timer

      // !jf! what is this and why do I need to call it? 
      mDum.mInviteSessionHandler->onAckNotReceived(getSessionHandle(), mInvite200);
      end();
   }
   else if (timeout.type() == DumTimeout::Glare)
   {
      if (mState == SentUpdateGlare)
      {
         InfoLog (<< "Retransmitting the UPDATE (glare condition timer)");
         mDum.send(mLastSessionModification);
         transition(SentUpdate);
      }
      else if (mState == SentReinviteGlare)
      {
         InfoLog (<< "Retransmitting the reINVITE (glare condition timer)");
         mDum.send(mLastSessionModification);
         transition(SentReinvite);
      }
   }
   else if (timeout.type() == DumTimeout::SessionExpiration)
   {
      if(timeout.seq() == mSessionTimerSeq)
      {
         end();  // end expired session
      }
   }
   else if (timeout.type() == DumTimeout::SessionRefresh)
   {
      if(timeout.seq() == mSessionTimerSeq)
      {
         if(mState == Connected)
         {
            provideOffer(*mCurrentLocalSdp);
         }
      }
   }
}


void
InviteSession::dispatchConnected(const SipMessage& msg, const SdpContents* sdp )
{
   switch (msg.header(h_CSeq).method())
   {
      case INVITE:
         if (msg.isRequest())
         {
            mLastSessionModification = msg;
            if (sdp == 0)
            {
               transition(ReceivedReinviteNoOffer);
               mDum.mInviteSessionHandler->onDialogModified(getSessionHandle(), Offer, msg);
               mDum.mInviteSessionHandler->onOfferRequired(getSessionHandle(), msg);
            }
            else
            {
               transition(ReceivedReinvite);
               mDum.mInviteSessionHandler->onOffer(getSessionHandle(), msg, sdp);
            }
         }
         else // retransmission of 200I
         {
            // We can construct the ACK using the dialog and then override the
            // CSeq by looking at the value in the 200I. We don't need to store
            // all the ACK messages sent which saves us from having to also have
            // a timer to clean them up after 64T1
            // might want to add a makeAck method in Dialog so we don't
            // increment CSeq unnecessarily

            // !jf! Need to include the answer here. 
            SipMessage ack;
            mDialog.makeRequest(ack, ACK);
            ack.header(h_CSeq).sequence() = msg.header(h_CSeq).sequence();
            mDum.send(ack);
         }
         break;

      case UPDATE:
         if (msg.isRequest())
         {
            InfoLog (<< "Received " << msg.brief());
            
            //  !kh!
            //  Find out if it's an UPDATE requiring state change.
            //  See rfc3311 5.2, 4th paragraph.
            mLastSessionModification = msg;
            transition(ReceivedUpdate);
            mDum.mInviteSessionHandler->onOffer(getSessionHandle(), msg, sdp);
         }
         else
         {
            WarningLog (<< "DUM delivered an UPDATE/200 in an incorrect state " << endl << msg);
            assert(0);
         }
         break;
         
      default:
         dispatchOthers(msg);
         break;
   }
}


void
InviteSession::dispatchSentUpdate(const SipMessage& msg, const SdpContents* sdp)
{
   MethodTypes method = msg.header(h_CSeq).method();
   if (method == INVITE || method == UPDATE && msg.isRequest())
   {
      SipMessage response;
      mDialog.makeResponse(response, msg, 491);
      send(response);
   }
   else if (method == UPDATE)
   {
      int code = msg.header(h_StatusLine).statusCode();
      if(code / 200 == 1)
      {
         mCurrentLocalSdp = mProposedLocalSdp;
         mCurrentRemoteSdp = InviteSession::makeSdp(*sdp);
         mDum.mInviteSessionHandler->onAnswer(getSessionHandle(), msg, sdp);
         transition(Connected);
      }
      else if (code == 491)
      {
         start491Timer();
         transition(SentUpdateGlare);
      }
      else if (code == 408 || code == 481)
      {
         mDum.mInviteSessionHandler->onTerminated(getSessionHandle());
         end();
      }
      else if (code >= 300)
      {
         mDum.mInviteSessionHandler->onOfferRejected(getSessionHandle(), msg);
         transition(Connected);
      }
   }
   else
   {
      dispatchOthers(msg);
   }
}

void
InviteSession::dispatchSentReinvite(const SipMessage& msg, const SdpContents* sdp)
{
   MethodTypes method = msg.header(h_CSeq).method();
   if (method == INVITE || method == UPDATE && msg.isRequest())
   {
      SipMessage response;
      mDialog.makeResponse(response, msg, 491);
      send(response);
   }
   else if (method == INVITE)
   {
      int code = msg.header(h_StatusLine).statusCode();
      if(code / 200 == 1)
      {
         mCurrentLocalSdp = mProposedLocalSdp;
         mCurrentRemoteSdp = InviteSession::makeSdp(*sdp);
         mDum.mInviteSessionHandler->onAnswer(getSessionHandle(), msg, sdp);

         // !jf! I need to potentially include an answer in the ACK here
         SipMessage ack;
         mDialog.makeRequest(ack, ACK);
         mDum.send(ack);
         
         // !jf! do I need to allow a reINVITE overlapping the retransmission of
         // the ACK when a 200I is received? If yes, then I need to store all
         // ACK messages for 64*T1
         transition(Connected);
      }
      else if (code == 491)
      {
         start491Timer();
         transition(SentUpdateGlare);
      }
      else if (code == 408 || code == 481)
      {
         mDum.mInviteSessionHandler->onTerminated(getSessionHandle());
         end();
      }
      else if (code >= 300)
      {
         mDum.mInviteSessionHandler->onOfferRejected(getSessionHandle(), msg);
         transition(Connected);
      }
   }
   else
   {
      dispatchOthers(msg);
   }
}

void
InviteSession::dispatchGlare(const SipMessage& msg)
{
   MethodTypes method = msg.header(h_CSeq).method();
   if (method == INVITE && msg.isRequest())
   {
      mDum.mInviteSessionHandler->onOfferRejected(getSessionHandle(), msg);
      transition(ReceivedReinvite);
   }
   else if (method == UPDATE && msg.isRequest())
   {
      mDum.mInviteSessionHandler->onOfferRejected(getSessionHandle(), msg);
      transition(ReceivedUpdate);
   }
   else
   {
      dispatchOthers(msg);
   }
}

void
InviteSession::dispatchReceivedUpdateOrReinvite(const SipMessage& msg)
{
   MethodTypes method = msg.header(h_CSeq).method();
   if (method == INVITE || method == UPDATE)
   {
      // Means that the UAC has sent us a second reINVITE or UPDATE before we
      // responded to the first one. Bastard!
      SipMessage response;
      mDialog.makeResponse(response, msg, 500);
      response.header(h_RetryAfter).value() = Random::getRandom() % 10;
      mDum.send(response);
   }
   else
   {
      dispatchOthers(msg);
   }      
}


void
InviteSession::dispatchAnswered(const SipMessage& msg)
{
   if (msg.isRequest() && msg.header(h_RequestLine).method() == ACK)
   {
      transition(Connected);
   }
   else
   {
      dispatchOthers(msg);
   }      
}

void
InviteSession::dispatchWaitingToOffer(const SipMessage& msg, const SdpContents* sdp)
{
   if (msg.isRequest() && msg.header(h_RequestLine).method() == ACK)
   {
      provideOffer(*sdp);
   }
   else
   {
      dispatchOthers(msg);
   }      
}

void
InviteSession::dispatchWaitingToTerminate(const SipMessage& msg)
{
   if (msg.isResponse() && 
       msg.header(h_CSeq).sequence() / 200 == 1 && 
       msg.header(h_CSeq).method() == INVITE)
   {
      // !jf! Need to include the answer here. 
      SipMessage ack;
      mDialog.makeRequest(ack, ACK);
      ack.header(h_CSeq).sequence() = msg.header(h_CSeq).sequence();
      mDum.send(ack);
      transition(Terminated);
   }
   else if (msg.isResponse() && msg.header(h_CSeq).method() == INVITE)
   {
      end();
   }
}

void
InviteSession::dispatchTerminated(const SipMessage& msg)
{
   Destroyer::Guard guard(mDestroyer);
   if (msg.isRequest())
   {
      SipMessage response;
      mDialog.makeResponse(response, msg, 481);
      mDum.send(response);
      guard.destroy();
   }
   else 
   {
      guard.destroy();
   }
}

void
InviteSession::dispatchOthers(const SipMessage& msg)
{
   switch (msg.header(h_CSeq).method())
   {
      case CANCEL:
         dispatchCancel(msg);
         break;
      case BYE:
         dispatchBye(msg);
         break;
      case INFO:
         dispatchInfo(msg);
         break;
      case REFER:
      default:
         // handled in Dialog
         WarningLog (<< "DUM delivered a REFER to the InviteSession " << endl << msg);
         assert(0);
         break;
   }
}

void
InviteSession::dispatchUnhandledInvite(const SipMessage& msg)
{
   assert(msg.isRequest());
   assert(msg.header(h_CSeq).method() == INVITE);

   // If we get an INVITE request from the wire and we are not in
   // Connected state, reject the request and send a BYE
   SipMessage response;
   mDialog.makeResponse(response, msg, 400); // !jf! what code to use?
   InfoLog (<< "Sending " << response.brief());
   mDum.send(response);

   SipMessage bye;
   mDialog.makeRequest(bye, BYE);
   InfoLog (<< "Sending " << bye.brief());
   mDum.send(bye);

   transition(Terminated);
}

void
InviteSession::dispatchCancel(const SipMessage& msg)
{
   assert(msg.header(h_CSeq).method() == CANCEL);
   if(msg.isRequest())
   {
      SipMessage rsp;
      mDialog.makeResponse(rsp, msg, 200);
      mDum.send(rsp);

      SipMessage bye;
      mDialog.makeRequest(bye, BYE);
      InfoLog (<< "Sending " << bye.brief());
      mDum.send(bye);

      transition(Terminated);

      // !jf! should we make some other callback here
      mDum.mInviteSessionHandler->onTerminated(getSessionHandle());
   }
   else
   {
      WarningLog (<< "DUM let me send a CANCEL at an incorrect state " << endl << msg);
      assert(0);
   }
}

void
InviteSession::dispatchBye(const SipMessage& msg)
{
   if (msg.isRequest())
   {
      Destroyer::Guard guard(mDestroyer);
      SipMessage rsp;
      InfoLog (<< "Received " << msg.brief());
      mDialog.makeResponse(rsp, msg, 200);
      mDum.send(rsp);
      transition(Terminated);

      // !jf! should we make some other callback here
      mDum.mInviteSessionHandler->onTerminated(getSessionHandle());
      guard.destroy();
   }
   else
   {
      WarningLog (<< "DUM let me send a BYE at an incorrect state " << endl << msg);
      assert(0);
   }
}

void
InviteSession::dispatchInfo(const SipMessage& msg)
{
   if (msg.isRequest())
   {
      InfoLog (<< "Received " << msg.brief());
      SipMessage response;
      mDialog.makeResponse(response, msg, 200);
      send(response);
      mDum.mInviteSessionHandler->onInfo(getSessionHandle(), msg);
   }
   else
   {
      assert(mNitState == NitProceeding);
      mNitState = NitComplete;
      
      if (msg.header(h_StatusLine).statusCode() >= 300)
      {
         mDum.mInviteSessionHandler->onInfoFailure(getSessionHandle(), msg);
      }
      else if (msg.header(h_StatusLine).statusCode() >= 200)
      {
         mDum.mInviteSessionHandler->onInfoSuccess(getSessionHandle(), msg);
      }
   }
}

void
InviteSession::startRetransmitTimer()
{
   mCurrentRetransmit200 = Timer::T1;
   int seq = mLastSessionModification.header(h_CSeq).sequence();
   mDum.addTimerMs(DumTimeout::Retransmit200, mCurrentRetransmit200, getBaseHandle(), seq);
   mDum.addTimerMs(DumTimeout::WaitForAck, Timer::TH, getBaseHandle(), seq);
}

void
InviteSession::start491Timer()
{
   int seq = mLastSessionModification.header(h_CSeq).sequence();
   int timer = Random::getRandom() % 4000;
   mDum.addTimerMs(DumTimeout::Glare, timer, getBaseHandle(), seq);
}

Data
InviteSession::toData(State state)
{
   switch (state)
   {
      case Undefined:
         return "InviteSession::Undefined";
      case Connected:
         return "InviteSession::Connected";
      case SentUpdate:
         return "InviteSession::SentUpdate";
      case SentUpdateGlare:
         return "InviteSession::SentUpdateGlare";
      case SentReinvite:
         return "InviteSession::SentReinvite";
      case SentReinviteGlare:
         return "InviteSession::SentReinviteGlare";
      case ReceivedUpdate:
         return "InviteSession::ReceivedUpdate";
      case ReceivedReinvite:
         return "InviteSession::ReceivedReinvite";
      case ReceivedReinviteNoOffer:
         return "InviteSession::ReceivedReinviteNoOffer";
      case Answered:
         return "InviteSession::Answered";
      case WaitingToOffer:
         return "InviteSession::WaitingToOffer";
      case WaitingToTerminate:
         return "InviteSession::WaitingToTerminate";
      case Terminated:
         return "InviteSession::Terminated";

      case UAC_Start:
         return "UAC_Start";
      case UAS_Offer:
         return "UAS_Offer";
      case UAS_Early:
         return "UAS_Early";
      case UAS_Accepted:
         return "UAS_Accepted";
      case UAS_NoOffer:
         return "UAS_NoOffer";
      case UAS_EarlyNoOffer:
         return "UAS_EarlyNoOffer";
      case UAS_AcceptedWaitingAnswer:
         return "UAS_AcceptedWaitingAnswer";
      case UAC_Early:
         return "UAC_Early";
      case UAC_EarlyWithOffer:
         return "UAC_EarlyWithOffer";
      case UAC_EarlyWithAnswer:
         return "UAC_EarlyWithAnswer";
      case UAC_WaitingForAnswerFromApp:
         return "UAC_WaitingForAnswerFromApp";
      case UAC_Terminated:
         return "UAC_Terminated";
      case UAC_SentUpdateEarly:
         return "UAC_SentUpdateEarly";
      case UAC_ReceivedUpdateEarly:
         return "UAC_ReceivedUpdateEarly";
      case UAC_PrackAnswerWait:
         return "UAC_PrackAnswerWait";
      case UAC_Canceled:
         return "UAC_Canceled";

      case UAS_Start:
         return "UAS_Start";
      case UAS_OfferReliable:
         return "UAS_OfferReliable";
      case UAS_NoOfferReliable:
         return "UAS_NoOfferReliable";
      case UAS_FirstSentOfferReliable:
         return "UAS_FirstSentOfferReliable";
      case UAS_FirstEarlyReliable:
         return "UAS_FirstEarlyReliable";
      case UAS_EarlyReliable:
         return "UAS_EarlyReliable";
      case UAS_SentUpdate:
         return "UAS_SentUpdate";
      case UAS_SentUpdateAccepted:
         return "UAS_SentUpdateAccepted";
      case UAS_ReceivedUpdate:
         return "UAS_ReceivedUpdate";
      case UAS_ReceivedUpdateWaitingAnswer:
         return "UAS_ReceivedUpdateWaitingAnswer";
      case UAS_WaitingToTerminate:
         return "UAS_WaitingToTerminate";
      case UAS_WaitingToHangup:
         return "UAS_WaitingToHangup";

      default:
         assert(0);
         return "Undefined";
   }
}


void
InviteSession::transition(State target)
{
   InfoLog (<< "Transition " << toData(mState) << " -> " << toData(target));
   mState = target;
}

bool
InviteSession::isReliable(const SipMessage& msg)
{
   return msg.header(h_Supporteds).find(Token(Symbols::C100rel));
}

const SdpContents*
InviteSession::getSdp(const SipMessage& msg) 
{
   const SdpContents* sdp=0;
   MultipartMixedContents* mixed = dynamic_cast<MultipartMixedContents*>(msg.getContents());
   if ( mixed )
   {
      // Look for first SDP Contents in a multipart contents
      MultipartMixedContents::Parts& parts = mixed->parts();
      for( MultipartMixedContents::Parts::const_iterator i = parts.begin();
           i != parts.end(); ++i)
      {
	     sdp = dynamic_cast<const SdpContents*>(*i);
		 if(sdp)
         {
            break;  // Found SDP contents
         }
      }
   }
   else
   {
      sdp = dynamic_cast<const SdpContents*>(msg.getContents());
   }

   return sdp;
}

std::auto_ptr<SdpContents>
InviteSession::makeSdp(const SdpContents& sdp)
{
   // This unfortunate utility is required to do the conversion from
   // auto_ptr<Contents> to auto_ptr<SdpContents> - uses a static_cast so don't
   // call it on something you don't know is an SdpContents!!!
   return std::auto_ptr<SdpContents>(static_cast<SdpContents*>(sdp.clone()));
}

void
InviteSession::setSdp(SipMessage& msg, const SdpContents& sdp)
{
   // !jf! should deal with multipart here

   // This will clone the sdp since the InviteSession also wants to keep its own
   // copy of the sdp around for the application to access
   msg.setContents(&sdp);
}


#if 0
//////////////////////////////////////////
// OLD CODE follows - deprecated
//////////////////////////////////////////

void
InviteSession::dispatch(const SipMessage& msg)
{
   Destroyer::Guard guard(mDestroyer);
   std::pair<OfferAnswerType, const SdpContents*> offans;
   offans = InviteSession::getOfferOrAnswer(msg);

   //ugly. non-invite-transactions(nit) don't interact with the invite
   //transaction state machine(for now we have a separate INFO state machine)
   //it's written as a gerneric NIT satet machine, but method isn't checked, and
   //info is the only NIT so far. This should eventually live in Dialog, with a
   //current method to determine valid responses.
   if (msg.header(h_CSeq).method() == INFO)
   {
      if (msg.isRequest())
      {
         SipMessage response;
         mDialog.makeResponse(response, msg, 200);
         send(response);
         mDum.mInviteSessionHandler->onInfo(getSessionHandle(), msg);
      }
      else
      {
         if (mNitState == NitProceeding)
         {
            int code = msg.header(h_StatusLine).statusCode();
            if (code < 200)
            {
               //ignore
            }
            else if (code < 300)
            {
               mNitState = NitComplete;
               mDum.mInviteSessionHandler->onInfoSuccess(getSessionHandle(), msg);
            }
            else
            {
               mNitState = NitComplete;
               mDum.mInviteSessionHandler->onInfoFailure(getSessionHandle(), msg);
            }
         }
      }
   }
   else if (msg.isRequest() && msg.header(h_RequestLine).method() == ACK &&
            (mState == Connected || mState == SentReinvite))
   {
      //quench 200 retransmissions
      mFinalResponseMap.erase(msg.header(h_CSeq).sequence());
      if (msg.header(h_CSeq).sequence() == mLastIncomingRequest.header(h_CSeq).sequence())
      {
         // BYE could be queued if end() is called when we are still waiting for far end ACK to be received
         if (mQueuedBye)
         {
            mState = Terminated;
            mLastRequest = *mQueuedBye;
            delete mQueuedBye;
            mQueuedBye = 0;
            send(mLastRequest);
            return;
         }

         if (offans.first != None)
         {
            if (mOfferState == Answered)
            {
               //SDP in invite and in ACK.
               mDum.mInviteSessionHandler->onIllegalNegotiation(getSessionHandle(), msg);
            }
            else
            {
               //delaying onConnected until late SDP
               InviteSession::incomingSdp(msg, offans.second);
               if (!mUserConnected)
               {
                  mUserConnected = true;
                  mDum.mInviteSessionHandler->onConnected(getSessionHandle(), msg);
               }
            }
         }
         //temporary hack
         else if (mState != SentReinvite && mOfferState != Answered)
         {
            //no SDP in ACK when one is required
            mDum.mInviteSessionHandler->onIllegalNegotiation(getSessionHandle(), msg);
         }
      }
   }

   switch(mState)
   {
      case Terminated:
         //!dcm! -- 481 behaviour here, should pretty much die on anything
         //eventually 200 to BYE could be handled further out
         if (msg.isResponse())
         {
            int code = msg.header(h_StatusLine).statusCode();
            if ((code  == 200 && msg.header(h_CSeq).method() == BYE) || code > 399)
            {
               mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), msg);
               guard.destroy();
               return;
            }
         }
         else
         {
            //make a function to do this & the occurences of this in DialogUsageManager
            SipMessage failure;
            mDum.makeResponse(failure, msg, 481);
            failure.header(h_AcceptLanguages) = mDum.mProfile->getSupportedLanguages();
            mDum.sendResponse(failure);
         }
         break;
      case Connected:
         if (msg.isRequest())
         {
            switch(msg.header(h_RequestLine).method())
            {
		       // reINVITE
               case INVITE:
                  if (mOfferState == Answered)
                  {
                     mState = SentReinvite;
                     mDialog.update(msg);
                     mLastIncomingRequest = msg;
                     mDum.mInviteSessionHandler->onDialogModified(getSessionHandle(), offans.first, msg);
                     if (offans.first != None)
                     {
                        incomingSdp(msg, offans.second);
                     }
                     else
                     {
                        mDum.mInviteSessionHandler->onOfferRequired(getSessionHandle(), msg);
                     }
                  }
                  else
                  {
                     //4??
                     SipMessage failure;
                     mDialog.makeResponse(failure, msg, 491);
                     InfoLog (<< "Sending 491 - overlapping Invite transactions");
                     mDum.sendResponse(failure);
                  }
                  break;
               case BYE:
                  mState = Terminated;
                  mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), msg);
                  mDialog.makeResponse(mLastResponse, msg, 200);
                  send(mLastResponse);
                  break;

               case UPDATE:
                  assert(0);
                  break;

               case INFO:
                  mDum.mInviteSessionHandler->onInfo(getSessionHandle(), msg);
                  break;
               case REFER:
                  //handled in Dialog
                  assert(0);
                  break;

			   case CANCEL:
				  // A Cancel can get received in an established dialog if it crosses with our 200 response
				  // on the wire - it should be responsed to, but should not effect the dialog state (InviteSession).
				  // RFC3261 Section 9.2
                  mDialog.makeResponse(mLastResponse, msg, 200);
                  send(mLastResponse);
				  break;

               default:
                  InfoLog (<< "Ignoring request in an INVITE dialog: " << msg.brief());
                  break;
            }
         }
         else
         {
            if ( msg.header(h_StatusLine).statusCode() == 200 && msg.header(h_CSeq).method() == INVITE)
            {
               CSeqToMessageMap::iterator it = mAckMap.find(msg.header(h_CSeq).sequence());
               if (it != mAckMap.end())
               {
                  mDum.send(it->second);
               }
            }
         }
         break;
      case SentReinvite:
         if (msg.header(h_CSeq).method() == INVITE)
         {
            if (msg.isResponse())
            {
               int code = msg.header(h_StatusLine).statusCode();
               if (code < 200)
               {
                  return;
               }
               else if (code < 300)
               {
                  if (msg.header(h_CSeq).sequence() == mLastRequest.header(h_CSeq).sequence())
                  {
                     mState = Connected;
                     //user has called end, so no more callbacks relating to
                     //this usage other than onTerminated
                     if (mQueuedBye)
                     {
                        send(makeAck());   // ACK the 200 first then send BYE
                        mState = Terminated;
                        mLastRequest = *mQueuedBye;
                        delete mQueuedBye;
                        mQueuedBye = 0;
                        send(mLastRequest);
                        return;
                     }

                     // Handle any Session Timer headers in response
                     handleSessionTimerResponse(msg);

                     if (offans.first != None)
                     {
                        if (offans.first == Answer)
                        {
                           //no late media required, so just send the ACK
                           send(makeAck());
                        }
                        incomingSdp(msg, offans.second);
                     }
                     else
                     {
                        //no offer or answer in 200, this will eventually be
                        //legal with PRACK/UPDATE
                        send(makeAck());
                        if (mOfferState != Answered)
                        {
                           //reset the sdp state machine
                           incomingSdp(msg, 0);
                           mDum.mInviteSessionHandler->onIllegalNegotiation(getSessionHandle(), msg);
                        }
                     }
                  }
                  else //200 retransmission that overlaps with this Invite transaction
                  {
                     CSeqToMessageMap::iterator it = mAckMap.find(msg.header(h_CSeq).sequence());
                     if (it != mAckMap.end())
                     {
                        mDum.send(it->second);
                     }
                  }
               }
               else if(code == 408 || code == 481)
               {
                   // If ReInvite response is Timeout (408) or Transaction Does not Exits (481) - end dialog
                   mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), msg);
                   guard.destroy();
               }
               else
               {
                  // !slg! handle 491 response and retry???

                  mState = Connected;
                  //user has called end, so no more callbacks relating to
                  //this usage other than onTerminated
                  if (mQueuedBye)
                  {
                     send(makeAck());   // ACK the 200 first then send BYE
                     mState = Terminated;
                     mLastRequest = *mQueuedBye;
                     delete mQueuedBye;
                     mQueuedBye = 0;
                     send(mLastRequest);
                     return;
                  }
                  //reset the sdp state machine
                  incomingSdp(msg, 0);
                  mDum.mInviteSessionHandler->onOfferRejected(getSessionHandle(), msg);
               }
            }
            else
            {
               SipMessage failure;
               mDialog.makeResponse(failure, msg, 491);
               InfoLog (<< "Sending 491 - overlapping Invite transactions");
               mDum.sendResponse(failure);
               return;
            }
         }
         else if(msg.header(h_CSeq).method() == BYE && msg.isRequest())
         {
	        // Inbound BYE crosses with outbound REINVITE

	        mState = Terminated;



            mDum.mInviteSessionHandler->onTerminated(getSessionHandle(),msg);

	        mDialog.makeResponse(mLastResponse, msg, 200);

            send(mLastResponse);

         }
         else
         {
            ErrLog ( << "Spurious message sent to UAS " << msg );
            return;
         }
         break;
      default:
         DebugLog ( << "Throwing away strange message: " << msg );
         //throw message away
//         assert(0);  //all other cases should be handled in base classes

   }
}

void
InviteSession::dispatch(const DumTimeout& timeout)
{
   Destroyer::Guard guard(mDestroyer);
   if (timeout.type() == DumTimeout::Retransmit200)
   {
      CSeqToMessageMap::iterator it = mFinalResponseMap.find(timeout.seq());
      if (it != mFinalResponseMap.end())
      {
         mDum.send(it->second);
         mCurrentRetransmit200 *= 2;
         mDum.addTimerMs(DumTimeout::Retransmit200, resipMin(Timer::T2, mCurrentRetransmit200), getBaseHandle(),  timeout.seq());
      }
   }
   else if (timeout.type() == DumTimeout::WaitForAck)
   {
      CSeqToMessageMap::iterator it = mFinalResponseMap.find(timeout.seq());
      if (it != mFinalResponseMap.end())
      {
         // BYE could be queued if end() is called when we are still waiting for far end ACK to be received
         if (mQueuedBye)
         {
            mState = Terminated;
            mLastRequest = *mQueuedBye;
            delete mQueuedBye;
            mQueuedBye = 0;
            send(mLastRequest);
         }
         else
         {
            mDum.mInviteSessionHandler->onAckNotReceived(getSessionHandle(), it->second);
         }

         mFinalResponseMap.erase(it);
      }
   }
   else if (timeout.type() == DumTimeout::CanDiscardAck)
   {
      assert(mAckMap.find(timeout.seq()) != mFinalResponseMap.end());
      mAckMap.erase(timeout.seq());
   }
   else if (timeout.type() == DumTimeout::SessionExpiration)
   {
      if(timeout.seq() == mSessionTimerSeq)
      {
          if(mState != Terminated)
          {
             end();  // end expired session
          }
      }
   }
   else if (timeout.type() == DumTimeout::SessionRefresh)
   {
      if(timeout.seq() == mSessionTimerSeq)
      {
         if(mState == Connected)
         {
            mState = SentReinvite;
            setOffer(mCurrentLocalSdp);
            // Should likely call targetRefresh when implemented - for now only ReInvites are used
            mDialog.makeRequest(mLastRequest, INVITE);
            if(mSessionInterval >= 90)
            {
               mLastRequest.header(h_SessionExpires).value() = mSessionInterval;

               mLastRequest.header(h_SessionExpires).param(p_refresher) = Data(mSessionRefresherUAS ? "uas" : "uac");
            }
            send(mLastRequest);
         }
      }
   }
}


SipMessage&
InviteSession::modifySession()
{
   DebugLog( << "InviteSession::modifySession: " << mDialog.getId());
   if (mNextOfferOrAnswerSdp == 0 || mState != Connected || mOfferState != Answered)
   {
      throw UsageUseException("Must be in the connected state and have propsed an offer to call modifySession",
                                  __FILE__, __LINE__);
   }
   mState = SentReinvite;
   mDialog.makeRequest(mLastRequest, INVITE);
   return mLastRequest;
}

SipMessage&
InviteSession::makeFinalResponse(int code)
{
   int cseq = mLastIncomingRequest.header(h_CSeq).sequence();
   SipMessage& finalResponse = mFinalResponseMap[cseq];
   mDialog.makeResponse(finalResponse, mLastIncomingRequest, 200);

   // Add Session Timer info to response (if required)
   handleSessionTimerRequest(mLastIncomingRequest, finalResponse);

   // Check if we should add our capabilites to the invite success response
   if(mDum.getProfile()->isAdvertisedCapability(Headers::Allow)) finalResponse.header(h_Allows) = mDum.getProfile()->getAllowedMethods();
   if(mDum.getProfile()->isAdvertisedCapability(Headers::AcceptEncoding)) finalResponse.header(h_AcceptEncodings) = mDum.getProfile()->getSupportedEncodings();
   if(mDum.getProfile()->isAdvertisedCapability(Headers::AcceptLanguage)) finalResponse.header(h_AcceptLanguages) = mDum.getProfile()->getSupportedLanguages();
   if(mDum.getProfile()->isAdvertisedCapability(Headers::Supported)) finalResponse.header(h_Supporteds) = mDum.getProfile()->getSupportedOptionTags();

   return finalResponse;
}

SipMessage&
InviteSession::acceptDialogModification(int statusCode)
{
   if (mNextOfferOrAnswerSdp == 0 || mState != SentReinvite)
   {
      throw UsageUseException("Must be in the SentReinvite state and have propsed an answer to call answerModifySession",
                                  __FILE__, __LINE__);
   }
   mState = Connected;
   return makeFinalResponse(statusCode);
}

void
InviteSession::setOffer(const SdpContents* sdp)
{
   DebugLog( << "InviteSession::setOffer: " << mDialog.getId());
   if (mProposedRemoteSdp)
   {
      throw UsageUseException("Cannot set an offer with an oustanding remote offer", __FILE__, __LINE__);
   }
   assert(mNextOfferOrAnswerSdp == 0);
   mNextOfferOrAnswerSdp = static_cast<SdpContents*>(sdp->clone());
}

void
InviteSession::setAnswer(const SdpContents* sdp)
{
   DebugLog( << "InviteSession::setAnswer: " << mDialog.getId());
   if (mProposedLocalSdp )
   {
      throw UsageUseException("Cannot set an answer with an oustanding offer", __FILE__, __LINE__);
   }
   assert(mNextOfferOrAnswerSdp == 0);
   mNextOfferOrAnswerSdp = static_cast<SdpContents*>(sdp->clone());
}


void
InviteSession::handleSessionTimerResponse(const SipMessage& msg)
{
   // If session timers are locally supported then handle response
   if(mDum.getProfile()->getSupportedOptionTags().find(Token(Symbols::Timer)))
   {
      bool fUAS = dynamic_cast<ServerInviteSession*>(this) != NULL;

      // Process Session Timer headers
      if(msg.exists(h_Requires) && msg.header(h_Requires).find(Token(Symbols::Timer)))
      {
         if(msg.exists(h_SessionExpires))
         {
            mSessionInterval = msg.header(h_SessionExpires).value();
            mSessionRefresherUAS = fUAS;  // Default to us as refresher
            if(msg.header(h_SessionExpires).exists(p_refresher))
            {
                mSessionRefresherUAS = (msg.header(h_SessionExpires).param(p_refresher) == Data("uas"));
            }
         }
         else
         {
            // If no Session Expires in response then session timer is to be 'turned off'
            mSessionInterval = 0;
         }
      }
      else if(msg.exists(h_SessionExpires))  // If UAS decides to be the refresher - then he MAY not set the Requires header to timer
      {
         mSessionInterval = msg.header(h_SessionExpires).value();
         mSessionRefresherUAS = fUAS;  // Default to us as refresher
         if(msg.header(h_SessionExpires).exists(p_refresher))
         {
             mSessionRefresherUAS = (msg.header(h_SessionExpires).param(p_refresher) == Data("uas"));
         }
      }
      // Note:  If no Requires or Session-Expires, then UAS does not support Session Timers - we are free to use our settings

      if(mSessionInterval >= 90)  // 90 is the absolute minimum
      {
         // Check if we are the refresher
         if((fUAS && mSessionRefresherUAS) || (!fUAS && !mSessionRefresherUAS))
         {
            // Start Session-Refresh Timer to mSessionInterval / 2 (recommended by draft-ietf-sip-session-timer-15)
            mDum.addTimer(DumTimeout::SessionRefresh, mSessionInterval / 2, getBaseHandle(), ++mSessionTimerSeq);
         }
         else
         {
            // Start Session-Expiration Timer to mSessionInterval - BYE should be sent a minimum of 32 or SessionInterval/3 seconds before the session expires (recommended by draft-ietf-sip-session-timer-15)
            mDum.addTimer(DumTimeout::SessionExpiration, mSessionInterval - resipMin(32,mSessionInterval/3), getBaseHandle(), ++mSessionTimerSeq);
         }
      }
   }
}

void
InviteSession::handleSessionTimerRequest(const SipMessage& request, SipMessage &response)
{
   // If session timers are locally supported then add necessary headers to response
   if(mDum.getProfile()->getSupportedOptionTags().find(Token(Symbols::Timer)))
   {
      bool fUAS = dynamic_cast<ServerInviteSession*>(this) != NULL;

      // Check if we are the refresher
      if((fUAS && mSessionRefresherUAS) || (!fUAS && !mSessionRefresherUAS))
      {
         // If we receive a reinvite, but we are the refresher - don't process for session timers (probably just a TargetRefresh or hold request)
         return;
      }

      mSessionInterval = mDum.getProfile()->getDefaultSessionTime();  // Used only if UAC doesn't request a time
      mSessionRefresherUAS = true;  // Used only if UAC doesn't request a time

      // Check if far-end supports
      bool farEndSupportsTimer = false;
      if(request.exists(h_Supporteds) && request.header(h_Supporteds).find(Token(Symbols::Timer)))
      {
         farEndSupportsTimer = true;
         if(request.exists(h_SessionExpires))
         {
            // Use Session Interval requested by UAC - if none then use local settings
            mSessionInterval = request.header(h_SessionExpires).value();
            mSessionRefresherUAS = fUAS;  // Default to us as refresher
            if(request.header(h_SessionExpires).exists(p_refresher))
            {
                mSessionRefresherUAS = (request.header(h_SessionExpires).param(p_refresher) == Data("uas"));
            }
         }
      }

      // Add Session-Expires if required
      if(mSessionInterval >= 90)
      {
         if(farEndSupportsTimer)
         {
            response.header(h_Requires).push_back(Token(Symbols::Timer));
         }
         response.header(h_SessionExpires).value() = mSessionInterval;
         response.header(h_SessionExpires).param(p_refresher) = Data(mSessionRefresherUAS ? "uas" : "uac");

         // Check if we are the refresher
         if((fUAS && mSessionRefresherUAS) || (!fUAS && !mSessionRefresherUAS))
         {
            // Start Session-Refresh Timer to mSessionInterval / 2 (recommended by draft-ietf-sip-session-timer-15)
            mDum.addTimer(DumTimeout::SessionRefresh, mSessionInterval / 2, getBaseHandle(), ++mSessionTimerSeq);
         }
         else
         {
            // Start Session-Expiration Timer to mSessionInterval - BYE should be sent a minimum of 32 or SessionInterval/3 seconds before the session expires (recommended by draft-ietf-sip-session-timer-15)
            mDum.addTimer(DumTimeout::SessionExpiration, mSessionInterval - resipMin(32,mSessionInterval/3), getBaseHandle(), ++mSessionTimerSeq);
         }
      }
   }
}


SipMessage&
InviteSession::makeInfo(auto_ptr<Contents> contents)
{
   if (mNitState == NitProceeding)
   {
      throw UsageUseException("Cannot start a non-invite transaction until the previous one has completed",
                                  __FILE__, __LINE__);
   }
   mNitState = NitProceeding;
   mDialog.makeRequest(mLastNit, INFO);
   mLastNit.releaseContents();
   mLastNit.setContents(contents);
   return mLastNit;
}

SipMessage&
InviteSession::makeRefer(const NameAddr& referTo)
{
   mDialog.makeRequest(mLastRequest, REFER);
   mLastRequest.header(h_ReferTo) = referTo;
//   mLastRequest.header(h_ReferTo).param(p_method) = getMethodName(INVITE);
   return mLastRequest;
}

SipMessage&
InviteSession::makeRefer(const NameAddr& referTo, InviteSessionHandle sessionToReplace)
{
   if (!sessionToReplace.isValid())
   {
      throw UsageUseException("Attempted to make a refer w/ and invalid replacement target", __FILE__, __LINE__);
   }

   mDialog.makeRequest(mLastRequest, REFER);
   mLastRequest.header(h_ReferTo) = referTo;
   CallId replaces;
   DialogId id = sessionToReplace->mDialog.getId();
   replaces.value() = id.getCallId();
   replaces.param(p_toTag) = id.getRemoteTag();
   replaces.param(p_fromTag) = id.getLocalTag();

   mLastRequest.header(h_ReferTo).uri().embedded().header(h_Replaces) = replaces;
   return mLastRequest;
}

void
InviteSession::end()
{
   InfoLog ( << "InviteSession::end, state: " << mState);
   switch (mState)
   {
      case Terminated:
         throw UsageUseException("Cannot end a session that has already been cancelled.", __FILE__, __LINE__);
         break;
      case Connected:
         // Check state of 200 retrans map to see if we have recieved an ACK or not yet
         if (mFinalResponseMap.find(mLastIncomingRequest.header(h_CSeq).sequence()) != mFinalResponseMap.end())
         {
            if(!mQueuedBye)
            {
               // No ACK yet - send BYE after ACK is received
               mQueuedBye = new SipMessage(mLastRequest);
               mDialog.makeRequest(*mQueuedBye, BYE);
            }
            else
            {
               throw UsageUseException("Cannot end a session that has already been cancelled.", __FILE__, __LINE__);
            }
         }
         else
         {
            mDialog.makeRequest(mLastRequest, BYE);
            //new transaction
            assert(mLastRequest.header(h_Vias).size() == 1);
            mState = Terminated;
            InfoLog ( << "InviteSession::end, Connected " << mLastRequest.brief() );
            send(mLastRequest);
            mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), msg);
         }
         break;
      case SentReinvite:
         if(!mQueuedBye)
         {
            mQueuedBye = new SipMessage(mLastRequest);
            mDialog.makeRequest(*mQueuedBye, BYE);
         }
         else
         {
            throw UsageUseException("Cannot end a session that has already been cancelled.", __FILE__, __LINE__);
         }
         break;
      default:
         assert(0); // out of states
   }
}

// If sdp==0, it means the last offer failed
// !dcm! -- eventually handle confused UA's that send offers/answers at
// inappropriate times, probably with a different callback
void
InviteSession::incomingSdp(const SipMessage& msg, const SdpContents* sdp)
{
   switch (mOfferState)
   {
      case Nothing:
         assert(mCurrentLocalSdp == 0);
         assert(mCurrentRemoteSdp == 0);
         assert(mProposedLocalSdp == 0);
         assert(mProposedRemoteSdp == 0);
         mProposedRemoteSdp = static_cast<SdpContents*>(sdp->clone());
         mOfferState = Offerred;
         mDum.mInviteSessionHandler->onOffer(getSessionHandle(), msg, sdp);
         break;

      case Offerred:
         assert(mCurrentLocalSdp == 0);
         assert(mCurrentRemoteSdp == 0);
         mCurrentLocalSdp = mProposedLocalSdp;
         mCurrentRemoteSdp = static_cast<SdpContents*>(sdp->clone());
		 delete mProposedRemoteSdp;
         mProposedLocalSdp = 0;
         mProposedRemoteSdp = 0;
         mOfferState = Answered;
         mDum.mInviteSessionHandler->onAnswer(getSessionHandle(), msg, sdp);
         break;

      case Answered:
         assert(mProposedLocalSdp == 0);
         assert(mProposedRemoteSdp == 0);
         mProposedRemoteSdp = static_cast<SdpContents*>(sdp->clone());
         mOfferState = CounterOfferred;
         mDum.mInviteSessionHandler->onOffer(getSessionHandle(), msg, sdp);
         break;

      case CounterOfferred:
         assert(mCurrentLocalSdp);
         assert(mCurrentRemoteSdp);
         mOfferState = Answered;
         if (sdp)  // !slg! There currenlty doesn't seem to be anyone calling this with sdp == 0
         {
            delete mCurrentLocalSdp;
            delete mCurrentRemoteSdp;
            mCurrentLocalSdp = mProposedLocalSdp;
            mCurrentRemoteSdp = static_cast<SdpContents*>(sdp->clone());
			delete mProposedRemoteSdp;
            mProposedLocalSdp = 0;
            mProposedRemoteSdp = 0;
            mOfferState = Answered;
            mDum.mInviteSessionHandler->onAnswer(getSessionHandle(), msg, sdp);
         }
         else
         {
			delete mProposedLocalSdp;
			delete mProposedRemoteSdp;
            mProposedLocalSdp = 0;
            mProposedRemoteSdp = 0;
            // !jf! is this right?
//            mDum.mInviteSessionHandler->onOfferRejected(getSessionHandle(), msg);
         }
         break;
   }
}

void
InviteSession::send(SipMessage& msg)
{
   Destroyer::Guard guard(mDestroyer);
   //handle NITs separately
   if (msg.header(h_CSeq).method() == INFO)
   {
      mDum.send(msg);
      return;
   }

   msg.releaseContents();
   if (mQueuedBye && (mQueuedBye == &msg))
   {
      //queued
      return;
   }

   if (msg.isRequest())
   {
      switch(msg.header(h_RequestLine).getMethod())
      {
         case INVITE:
         case UPDATE:
         case ACK:
            if (mNextOfferOrAnswerSdp)
            {
               msg.setContents(mNextOfferOrAnswerSdp);
               sendSdp(mNextOfferOrAnswerSdp);
               mNextOfferOrAnswerSdp = 0;
            }
            break;
         default:
            break;
      }
      mDum.send(msg);
   }
   else
   {
      int code = msg.header(h_StatusLine).statusCode();
      //!dcm! -- probably kill this object earlier, handle 200 to bye in
      //DialogUsageManager...very soon
      if (msg.header(h_CSeq).method() == BYE && code == 200) //!dcm! -- not 2xx?

      {
         mState = Terminated;
         mDum.send(msg);
	     //mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), msg);      // This is actually called when recieving the BYE message so that the BYE message can be passed to onTerminated
         guard.destroy();
      }
      else if (code >= 200 && code < 300 && msg.header(h_CSeq).method() == INVITE)
      {
         int seq = msg.header(h_CSeq).sequence();
         mCurrentRetransmit200 = Timer::T1;
         mDum.addTimerMs(DumTimeout::Retransmit200, mCurrentRetransmit200, getBaseHandle(), seq);
         mDum.addTimerMs(DumTimeout::WaitForAck, Timer::TH, getBaseHandle(), seq);

         //!dcm! -- this should be mFinalResponse...maybe assign here in
         //case the user wants to be very strange
         if (mNextOfferOrAnswerSdp)
         {
            msg.setContents(mNextOfferOrAnswerSdp);
            sendSdp(mNextOfferOrAnswerSdp);
            mNextOfferOrAnswerSdp = 0;
         }
         mDum.send(msg);
      }
      else
      {
         mDum.send(msg);
      }
   }
}

void
InviteSession::sendSdp(SdpContents* sdp)
{
   switch (mOfferState)
   {
      case Nothing:
         assert(mCurrentLocalSdp == 0);
         assert(mCurrentRemoteSdp == 0);
         mProposedLocalSdp = sdp;
         mOfferState = Offerred;
         break;

      case Offerred:
         assert(mCurrentLocalSdp == 0);
         assert(mCurrentRemoteSdp == 0);
         mCurrentLocalSdp = sdp;
         mCurrentRemoteSdp = mProposedRemoteSdp;
		 delete mProposedLocalSdp;
         mProposedLocalSdp = 0;
         mProposedRemoteSdp = 0;
         mOfferState = Answered;
         break;

      case Answered:
         assert(mProposedLocalSdp == 0);
         assert(mProposedRemoteSdp == 0);
         mProposedLocalSdp = sdp;
         mOfferState = CounterOfferred;
         break;

      case CounterOfferred:
         assert(mCurrentLocalSdp);
         assert(mCurrentRemoteSdp);
         if (sdp)
         {
            delete mCurrentLocalSdp;
            delete mCurrentRemoteSdp;
            mCurrentLocalSdp = sdp;
            mCurrentRemoteSdp = mProposedRemoteSdp;
			delete mProposedLocalSdp;
			mProposedLocalSdp = 0;
			mProposedRemoteSdp = 0;
         }
		 else
		 {
			delete mProposedLocalSdp;
			delete mProposedRemoteSdp;
			mProposedLocalSdp = 0;
			mProposedRemoteSdp = 0;
		 }
         mOfferState = Answered;
         break;
   }
}


SipMessage&
InviteSession::rejectDialogModification(int statusCode)
{
   if (statusCode < 400)
   {
      throw UsageUseException("Must reject with a 4xx", __FILE__, __LINE__);
   }
   mDialog.makeResponse(mLastResponse, mLastIncomingRequest, statusCode);
   mState = Connected;
   sendSdp(0);
   return mLastResponse;
}

SipMessage&
InviteSession::targetRefresh(const NameAddr& localUri)
{
   assert(0);
   return mLastRequest;
}

void
InviteSession::send()
{
   InfoLog ( << "InviteSession::send(void)");
   if (mOfferState == Answered || mState != Connected)
   {
      throw UsageUseException("Cannot call send when there it no Offer/Answer negotiation to do", __FILE__, __LINE__);
   }
   send(makeAck());
}

SipMessage&
InviteSession::makeAck()
{
   InfoLog ( << "InviteSession::makeAck" );

   int cseq = mLastRequest.header(h_CSeq).sequence();
   if (mAckMap.find(cseq) != mAckMap.end())
   {
      InfoLog ( << "CSeq collision in ack map: " << Inserter(mAckMap) );
   }

   assert(mAckMap.find(cseq) == mAckMap.end());
   SipMessage& ack = mAckMap[cseq];
   ack = mLastRequest;
   mDialog.makeRequest(ack, ACK);
   mDum.addTimerMs(DumTimeout::CanDiscardAck, Timer::TH, getBaseHandle(), cseq);

   assert(ack.header(h_Vias).size() == 1);

//    if (mNextOfferOrAnswerSdp)
//    {
//       ack.setContents(mNextOfferOrAnswerSdp);
//       sendSdp(mNextOfferOrAnswerSdp);
//       mNextOfferOrAnswerSdp = 0;
//    }
   return ack;
}

#endif

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
