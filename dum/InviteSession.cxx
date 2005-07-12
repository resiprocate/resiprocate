#include "resiprocate/MultipartMixedContents.hxx"
#include "resiprocate/MultipartAlternativeContents.hxx"
#include "resiprocate/SdpContents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/InviteSession.hxx"
#include "resiprocate/dum/ServerInviteSession.hxx"
#include "resiprocate/dum/ClientSubscription.hxx"
#include "resiprocate/dum/ServerSubscription.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/MasterProfile.hxx"
#include "resiprocate/dum/UsageUseException.hxx"
#include "resiprocate/os/Inserter.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Timer.hxx"
#include "resiprocate/os/Random.hxx"
#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"

// Remove warning about 'this' use in initiator list - pointer is only stored
#if defined(WIN32)
#pragma warning( disable : 4355 ) // using this in base member initializer list
#pragma warning( disable : 4800 ) // forcing value to bool (performance warning)
#endif

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM
#define THROW(msg)  throw DialogUsage::Exception(msg, __FILE__,__LINE__);

using namespace resip;
using namespace std;

InviteSession::InviteSession(DialogUsageManager& dum, Dialog& dialog)
   : DialogUsage(dum, dialog),
     mState(Undefined),
     mNitState(NitComplete),
     mCurrentRetransmit200(0),
     mSessionInterval(0),
     mMinSE(90), 
     mSessionRefresher(false),
     mSessionTimerSeq(0),
     mSentRefer(false),
     mCurrentEncryptionLevel(DialogUsageManager::None),
     mProposedEncryptionLevel(DialogUsageManager::None)
{
   DebugLog ( << "^^^ InviteSession::InviteSession " << this);
   assert(mDum.mInviteSessionHandler);
}

InviteSession::~InviteSession()
{
   DebugLog ( << "^^^ InviteSession::~InviteSession " << this);
   mDialog.mInviteSession = 0;
}

void 
InviteSession::dialogDestroyed(const SipMessage& msg)
{
   assert(0);
   
   // !jf! Is this correct? Merged from main...
   // !jf! what reason - guessed for now?
   //mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::PeerEnded, msg);   
   //delete this;   
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

const Data& 
InviteSession::getDialogId() const
{
   return mDialog.getId().getCallId();
}

InviteSessionHandle
InviteSession::getSessionHandle()
{
   return InviteSessionHandle(mDum, getBaseHandle().getId());
}

void InviteSession::storePeerCapabilities(const SipMessage& msg)
{
   // !slg! ToDo - add methods to get this data, App may be interested
   if (msg.exists(h_Allows))
   {
      mPeerSupportedMethods = msg.header(h_Allows);
   }
   if (msg.exists(h_Supporteds))
   {
      mPeerSupportedOptionTags = msg.header(h_Supporteds);
   }
   if (msg.exists(h_AcceptEncodings))
   {
      mPeerSupportedEncodings = msg.header(h_AcceptEncodings);
   }
   if (msg.exists(h_AcceptLanguages))
   {
      mPeerSupportedLanguages = msg.header(h_AcceptLanguages);
   }
   if (msg.exists(h_Accepts))
   {
      mPeerSupportedMimeTypes = msg.header(h_Accepts);
   }
}

bool
InviteSession::updateMethodSupported() const
{
   // Check if Update is supported locally
   if(mDum.getMasterProfile()->isMethodSupported(UPDATE))
   {
       // Check if peer supports UPDATE
       return mPeerSupportedMethods.find(Token("UPDATE"));
   }
   return false;
}

const NameAddr&
InviteSession::myAddr() const
{
   return mDialog.mLocalNameAddr;
}

const NameAddr&
InviteSession::peerAddr() const
{
   return mDialog.mRemoteNameAddr;
}

bool
InviteSession::isConnected() const
{
   switch (mState)
   {
      case Connected:
      case SentUpdate:
      case SentUpdateGlare:
      case SentReinvite:
      case SentReinviteGlare:
      case ReceivedUpdate:
      case ReceivedReinvite:
      case ReceivedReinviteNoOffer:
      case Answered:
      case WaitingToOffer:
         return true;

      default:
         return false;
   }
}

bool
InviteSession::isEarly() const
{
   switch (mState)
   {
      case UAC_Start:
      case UAC_Early:
      case UAC_EarlyWithOffer:
      case UAC_EarlyWithAnswer:
         //case UAC_Answered:
         //case UAC_Terminated:
      case UAC_SentUpdateEarly:
      case UAC_SentUpdateConnected:
      case UAC_ReceivedUpdateEarly:
         //case UAC_SentAnswer:
      case UAC_QueuedUpdate:
         return true;

      default:
         return false;
   }
}


bool
InviteSession::isTerminated() const
{
   switch (mState)
   {
      case Terminated:
      case WaitingToTerminate:
      case UAC_Cancelled:
      case UAS_WaitingToTerminate:
      case UAS_WaitingToHangup:
         return true;
      default:
         return false;
   }
}

std::ostream&
InviteSession::dump(std::ostream& strm) const
{
   strm << "INVITE: " << mId
        << " " << toData(mState)
        << " ADDR=" << myAddr()
        << " PEER=" << peerAddr();
   return strm;
}

void
InviteSession::provideOffer(const SdpContents& offer,
                            DialogUsageManager::EncryptionLevel level,
                            const SdpContents* alternative)
{
   switch (mState)
   {
      case Connected:
      case WaitingToOffer:
      case UAS_WaitingToOffer:
         if (updateMethodSupported())
         {
            transition(SentUpdate);
            mDialog.makeRequest(mLastSessionModification, UPDATE);
         }
         else
         {
            transition(SentReinvite);
            mDialog.makeRequest(mLastSessionModification, INVITE);
         }
         setSessionTimerHeaders(mLastSessionModification);

         InfoLog (<< "Sending " << mLastSessionModification.brief());
         InviteSession::setSdp(mLastSessionModification, offer, alternative);
         mProposedLocalSdp = InviteSession::makeSdp(offer, alternative);
         mProposedEncryptionLevel = level;
         mDialog.send(mLastSessionModification, mProposedEncryptionLevel);
         break;

      case Answered:
         // queue the offer to be sent after the ACK is received
         transition(WaitingToOffer);
         mProposedEncryptionLevel = level;
         mProposedLocalSdp = InviteSession::makeSdp(offer, alternative);
         break;

      // !slg! Can we handle all of the states listed in isConnected() ???
      default:
         WarningLog (<< "Can't provideOffer when not in Connected state");
         throw DialogUsage::Exception("Can't provide an offer", __FILE__,__LINE__);
   }
}

void
InviteSession::provideOffer(const SdpContents& offer)
{
   return provideOffer(offer, mCurrentEncryptionLevel, 0);
}

void
InviteSession::provideAnswer(const SdpContents& answer)
{
   switch (mState)
   {
      case ReceivedReinvite:
         transition(Connected);
         mDialog.makeResponse(mInvite200, mLastSessionModification, 200);
         handleSessionTimerRequest(mInvite200, mLastSessionModification);
         InviteSession::setSdp(mInvite200, answer, 0);
         mCurrentLocalSdp = InviteSession::makeSdp(answer);
         mCurrentRemoteSdp = mProposedRemoteSdp;
         InfoLog (<< "Sending " << mInvite200.brief());
         mDialog.send(mInvite200, mCurrentEncryptionLevel);
         startRetransmit200Timer();
         break;

      case ReceivedUpdate: // same as ReceivedReinvite case.
      {
         transition(Connected);

         SipMessage response;
         mDialog.makeResponse(response, mLastSessionModification, 200);
         handleSessionTimerRequest(response, mLastSessionModification);
         InviteSession::setSdp(response, answer, 0);
         mCurrentLocalSdp = InviteSession::makeSdp(answer);
         mCurrentRemoteSdp = mProposedRemoteSdp;
         InfoLog (<< "Sending " << response.brief());
         mDialog.send(response, mCurrentEncryptionLevel);
         break;
      }

      default:
         WarningLog (<< "Can't provideAnswer when not in Connected state");
         throw DialogUsage::Exception("Can't provide an offer", __FILE__,__LINE__);
   }
}

void
InviteSession::end()
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;

   switch (mState)
   {
      case Connected:
      {
         // !jf! do we need to store the BYE somewhere?
         sendBye();
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Ended); 
         break;
      }

      case SentUpdate:
         sendBye();
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Ended); 
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
         InfoLog (<< "Sending " << response.brief());
         mDialog.send(response);

         sendBye();
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Ended); 
         break;
      }

      case WaitingToTerminate:
      {
         sendBye();
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Ended); 
         break;
      }

      case Terminated:
         // no-op.
         break;

      default:
         assert(0);
         break;
   }
}

void
InviteSession::reject(int statusCode, WarningCategory *warning)
{
   switch (mState)
   {
      case ReceivedUpdate:
      case ReceivedReinvite:
      case ReceivedReinviteNoOffer:
      {
         transition(Connected);

         SipMessage response;
         mDialog.makeResponse(response, mLastSessionModification, statusCode);
         if(warning)
         {
            response.header(h_Warnings).push_back(*warning);
         }
         InfoLog (<< "Sending " << response.brief());
         mDialog.send(response);
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
   if (isConnected()) // !slg! likely not safe in any state except Connected - what should behaviour be if state is ReceivedReinvite?
   {
      // !jf! add interface to Dialog
      //mDialog.setLocalContact(localUri);
      provideOffer(*mCurrentLocalSdp);
   }
   else
   {
      WarningLog (<< "Can't targetRefresh before Connected");
      assert(0);
      throw UsageUseException("targetRefresh not allowed in this context", __FILE__, __LINE__);
   }
}

void
InviteSession::refer(const NameAddr& referTo)
{
   if (mSentRefer)
   {
      throw UsageUseException("Attempted to send overlapping refer", __FILE__, __LINE__);
   }

   if (isConnected()) // !slg! likely not safe in any state except Connected - what should behaviour be if state is ReceivedReinvite?
   {
      mSentRefer = true;
      SipMessage refer;
      mDialog.makeRequest(refer, REFER);
      refer.header(h_ReferTo) = referTo;
      refer.header(h_ReferredBy) = mDialog.mLocalContact; // !slg! is it ok to do this - should it be an option?
      mDialog.send(refer);
   }
   else
   {
      WarningLog (<< "Can't refer before Connected");
      assert(0);
      throw UsageUseException("REFER not allowed in this context", __FILE__, __LINE__);
   }
}

void
InviteSession::refer(const NameAddr& referTo, InviteSessionHandle sessionToReplace)
{
   if (!sessionToReplace.isValid())
   {
      throw UsageUseException("Attempted to make a refer w/ and invalid replacement target", __FILE__, __LINE__);
   }

   if (mSentRefer)
   {
      throw UsageUseException("Attempted to send overlapping refer", __FILE__, __LINE__);
   }

   if (isConnected())  // !slg! likely not safe in any state except Connected - what should behaviour be if state is ReceivedReinvite?
   {
      mSentRefer = true;
      SipMessage refer;
      mDialog.makeRequest(refer, REFER);

      refer.header(h_ReferTo) = referTo;
      refer.header(h_ReferredBy) = mDialog.mLocalContact; // !slg! is it ok to do this - should it be an option?
      CallId replaces;
      DialogId id = sessionToReplace->mDialog.getId();
      replaces.value() = id.getCallId();
      replaces.param(p_toTag) = id.getRemoteTag();
      replaces.param(p_fromTag) = id.getLocalTag();

      refer.header(h_ReferTo).uri().embedded().header(h_Replaces) = replaces;
      mDialog.send(refer);
   }
   else
   {
      WarningLog (<< "Can't refer before Connected");
      assert(0);
      throw UsageUseException("REFER not allowed in this context", __FILE__, __LINE__);
   }
}

void
InviteSession::info(const Contents& contents)
{
   if (mNitState == NitComplete)
   {
      if (isConnected())  // !slg! likely not safe in any state except Connected - what should behaviour be if state is ReceivedReinvite?
      {
         mNitState = NitProceeding;
         SipMessage info;
         mDialog.makeRequest(info, INFO);
         // !jf! handle multipart here
         info.setContents(&contents);
         mDialog.send(info, mCurrentEncryptionLevel);
      }
      else
      {
         WarningLog (<< "Can't send INFO before Connected");
         assert(0);
         throw UsageUseException("Can't send INFO before Connected", __FILE__, __LINE__);
      }
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
   // !jf! do we need to handle 3xx here or is it handled elsewhere?
   switch (mState)
   {
      case Connected:
         dispatchConnected(msg);
         break;
      case SentUpdate:
         dispatchSentUpdate(msg);
         break;
      case SentReinvite:
         dispatchSentReinvite(msg);
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
         dispatchWaitingToOffer(msg);
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
      if (mCurrentRetransmit200)
      {
         InfoLog (<< "Retransmitting: " << endl << mInvite200);
         mDialog.send(mInvite200, DialogUsageManager::None);
         mCurrentRetransmit200 *= 2;
         mDum.addTimerMs(DumTimeout::Retransmit200, resipMin(Timer::T2, mCurrentRetransmit200), getBaseHandle(),  timeout.seq());
      }
   }
   else if (timeout.type() == DumTimeout::WaitForAck)
   {
      if(mCurrentRetransmit200)  // If retransmit200 timer is active then ACK is not received yet
      {
         mCurrentRetransmit200 = 0; // stop the 200 retransmit timer

         // this is so the app can decided to ignore this. default implementation
         // will call end next
         mDum.mInviteSessionHandler->onAckNotReceived(getSessionHandle());

         // If we are waiting for an Ack and it times out, then end with a BYE
         if(mState == UAS_WaitingToHangup)
         {
             sendBye();
             transition(Terminated);
             mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::Ended); 
         }
      }
   }
   else if (timeout.type() == DumTimeout::Glare)
   {
      if (mState == SentUpdateGlare)
      {
         transition(SentUpdate);

         InfoLog (<< "Retransmitting the UPDATE (glare condition timer)");
         mDialog.send(mLastSessionModification, DialogUsageManager::None);
      }
      else if (mState == SentReinviteGlare)
      {
         transition(SentReinvite);

         InfoLog (<< "Retransmitting the reINVITE (glare condition timer)");
         mDialog.send(mLastSessionModification, DialogUsageManager::None);
      }
   }
   else if (timeout.type() == DumTimeout::SessionExpiration)
   {
      if(timeout.seq() == mSessionTimerSeq)
      {
         // this is so the app can decided to ignore this. default implementation
         // will call end next
         mDum.mInviteSessionHandler->onSessionExpired(getSessionHandle());
      }
   }
   else if (timeout.type() == DumTimeout::SessionRefresh)
   {
     if(timeout.seq() == mSessionTimerSeq)
      {
         if(mState == Connected)  // Note:  If not connected then we must be issueing a reinvite/update or receiving one - in either case the session timer stuff will get reset/renegotiated - thus just ignore this referesh
         {
            sessionRefresh();
         }
      }
   }
}

void
InviteSession::dispatchConnected(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
   {
      case OnInvite:
      case OnInviteReliable:
         mLastSessionModification = msg;
         transition(ReceivedReinviteNoOffer);
         //handler->onDialogModified(getSessionHandle(), None, msg);
         handler->onOfferRequired(getSessionHandle(), msg);
         break;

      case OnInviteOffer:
      case OnInviteReliableOffer:
         mLastSessionModification = msg;
         transition(ReceivedReinvite);
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         //handler->onDialogModified(getSessionHandle(), Offer, msg);
         handler->onOffer(getSessionHandle(), msg, *sdp);
         break;

      case On2xx:
      case On2xxOffer:
      case On2xxAnswer:
         // retransmission of 200I
         // !jf! Need to include the answer here.
         sendAck();
         break;

      case OnUpdateOffer:
         transition(ReceivedUpdate);

         //  !kh!
         //  Find out if it's an UPDATE requiring state change.
         //  See rfc3311 5.2, 4th paragraph.
         mLastSessionModification = msg;
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         handler->onOffer(getSessionHandle(), msg, *sdp);
         break;

      case OnUpdate:
      {
         // !slg! no sdp in update - just responsd immediately (likely session timer) - do we need a callback?
         SipMessage response;
         mLastSessionModification = msg;
         mDialog.makeResponse(response, mLastSessionModification, 200);
         handleSessionTimerRequest(response, mLastSessionModification);
         send(response);
         break;
      }

      case OnUpdateRejected:
      case On200Update:
            WarningLog (<< "DUM delivered an UPDATE response in an incorrect state " << endl << msg);
            assert(0);
         break;

      case OnAck:
         mCurrentRetransmit200 = 0; // stop the 200 retransmit timer
         break;

      default:
         dispatchOthers(msg);
         break;
   }
}

void
InviteSession::dispatchSentUpdate(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
   {
      case OnInvite:
      case OnInviteReliable:
      case OnInviteOffer:
      case OnInviteReliableOffer:
      case OnUpdate:
      case OnUpdateOffer:
      {
         // glare
         SipMessage response;
         mDialog.makeResponse(response, msg, 491);
         send(response);
         break;
      }

      case On200Update:
         transition(Connected);
         handleSessionTimerResponse(msg);
         if (sdp.get())
         {
            mCurrentEncryptionLevel = getEncryptionLevel(msg);
            setCurrentLocalSdp(msg);
            mCurrentRemoteSdp = InviteSession::makeSdp(*sdp);
            handler->onAnswer(getSessionHandle(), msg, *sdp);
         }
         else if(mProposedLocalSdp.get()) 
         {
            // If we sent an offer in the Update Request and no answer is received
            handler->onIllegalNegotiation(getSessionHandle(), msg);
            mProposedLocalSdp.release();
            mProposedEncryptionLevel = DialogUsageManager::None;
         }
         break;

      case On491Update:
         transition(SentUpdateGlare);
         start491Timer();
         break;

      case On422Update: // session timer
         if(msg.exists(h_MinSE))
         {
            // Change interval to min from 422 response
            mSessionInterval = msg.header(h_MinSE).value();
            mMinSE = mSessionInterval;
            sessionRefresh();
         }
         else
         {
            // Response must contact Min_SE - if not - just ignore
            // !slg! callback?
            transition(Connected);
            mProposedLocalSdp.release();
            mProposedEncryptionLevel = DialogUsageManager::None;
         }
         break;

      case OnUpdateRejected:
         // !jf! - callback?
         mProposedLocalSdp.release();
         mProposedEncryptionLevel = DialogUsageManager::None;
         transition(Connected);
         break;

      case OnGeneralFailure:
         sendBye();
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure, &msg);
         break;

      default:
         dispatchOthers(msg);
         break;
   }
}

void
InviteSession::dispatchSentReinvite(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
   {
      case OnInvite:
      case OnInviteReliable:
      case OnInviteOffer:
      case OnInviteReliableOffer:
      case OnUpdate:
      case OnUpdateOffer:
      {
         SipMessage response;
         mDialog.makeResponse(response, msg, 491);
         send(response);
         break;
      }

      case On1xx:
      case On1xxEarly:
         // !slg! Some UA's send a 100 response to a ReInvite - just ignore it
         break;

      case On2xxAnswer:
      case On2xxOffer:
      {
         transition(Connected);
         handleSessionTimerResponse(msg);
         setCurrentLocalSdp(msg);
         mCurrentRemoteSdp = InviteSession::makeSdp(*sdp);
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         // !jf! I need to potentially include an answer in the ACK here
         sendAck();
         handler->onAnswer(getSessionHandle(), msg, *sdp);


         // !jf! do I need to allow a reINVITE overlapping the retransmission of
         // the ACK when a 200I is received? If yes, then I need to store all
         // ACK messages for 64*T1
         break;
      }
      case On2xx:
         sendAck();
         transition(Connected);
         handleSessionTimerResponse(msg);
         handler->onIllegalNegotiation(getSessionHandle(), msg);
         mProposedLocalSdp.release();
         mProposedEncryptionLevel = DialogUsageManager::None;
         break;

      case On422Invite:
         if(msg.exists(h_MinSE))
         {
            // Change interval to min from 422 response
            mSessionInterval = msg.header(h_MinSE).value();
            mMinSE = mSessionInterval;
            sessionRefresh();
         }
         else
         {
            // Response must contact Min_SE - if not - just ignore
            // !slg! callback?
            transition(Connected);
            mProposedLocalSdp.release();
            mProposedEncryptionLevel = DialogUsageManager::None;
         }
         break;

      case On491Invite:
         transition(SentReinviteGlare);
         start491Timer();
         break;

      case OnGeneralFailure:
         sendBye();
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure, &msg);
         break;

      case OnInviteFailure:
         transition(Connected);
         mProposedLocalSdp.release();
         handler->onOfferRejected(getSessionHandle(), msg);
         break;

      default:
         dispatchOthers(msg);
         break;
   }
}

void
InviteSession::dispatchGlare(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   MethodTypes method = msg.header(h_CSeq).method();
   if (method == INVITE && msg.isRequest())
   {
      // Received inbound reinvite, when waiting to resend outbound reinvite or update
      transition(ReceivedReinvite);
      handler->onOfferRejected(getSessionHandle(), msg);
   }
   else if (method == UPDATE && msg.isRequest())
   {
      // Received inbound update, when waiting to resend outbound reinvite or update
      transition(ReceivedUpdate);
      handler->onOfferRejected(getSessionHandle(), msg);
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
      mDialog.send(response);
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
      mCurrentRetransmit200 = 0; // stop the 200 retransmit timer
      transition(Connected);
   }
   else
   {
      dispatchOthers(msg);
   }
}

void
InviteSession::dispatchWaitingToOffer(const SipMessage& msg)
{
   if (msg.isRequest() && msg.header(h_RequestLine).method() == ACK)
   {

      assert(mProposedLocalSdp.get());
      mCurrentRetransmit200 = 0; // stop the 200 retransmit timer
      if (dynamic_cast<MultipartAlternativeContents*>(mProposedLocalSdp.get()))
      {
         provideOffer( *(dynamic_cast<SdpContents*>((dynamic_cast<MultipartAlternativeContents*>(mProposedLocalSdp.get()))->parts().back())),
                       mProposedEncryptionLevel,
                       dynamic_cast<SdpContents*>((dynamic_cast<MultipartAlternativeContents*>(mProposedLocalSdp.get()))->parts().front()));
      }
      else
      {
         provideOffer(*(dynamic_cast<SdpContents*>(mProposedLocalSdp.get())), mProposedEncryptionLevel, 0);
      }
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
       msg.header(h_CSeq).method() == INVITE)
   {
      if(msg.header(h_StatusLine).statusCode() / 200 == 1)  // Note: stack ACK's non-2xx final responses only
      {
         // !jf! Need to include the answer here.
         sendAck();
      }
      sendBye();
      transition(Terminated);
      mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::Ended); 
   }
}

void
InviteSession::dispatchTerminated(const SipMessage& msg)
{
   InfoLog (<< "InviteSession::dispatchTerminated " << msg.brief());

   if (msg.isRequest())
   {
      SipMessage response;
      mDialog.makeResponse(response, msg, 481);
      mDialog.send(response);

      // !jf! means the peer sent BYE while we are waiting for response to BYE
      //mDum.destroy(this);
   }
   else
   {
      mDum.destroy(this);
   }
}

void
InviteSession::dispatchOthers(const SipMessage& msg)
{
   // handle OnGeneralFailure
   // handle OnRedirect

   switch (msg.header(h_CSeq).method())
   {
      case PRACK:
         dispatchPrack(msg);
         break;
      case CANCEL:
         dispatchCancel(msg);
         break;
      case BYE:
         dispatchBye(msg);
         break;
      case INFO:
         dispatchInfo(msg);
         break;
	  case ACK:
		  // Ignore duplicate ACKs from 2xx reTransmissions
		  break;
      default:
         // handled in Dialog
         WarningLog (<< "DUM delivered a "
                     << msg.header(h_CSeq).unknownMethodName()
                     << " to the InviteSession "
                     << endl
                     << msg);
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
   mDialog.send(response);

   sendBye();
   transition(Terminated);
   mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure, &msg); 
}

void
InviteSession::dispatchPrack(const SipMessage& msg)
{
   assert(msg.header(h_CSeq).method() == PRACK);
   if(msg.isRequest())
   {
      SipMessage rsp;
      mDialog.makeResponse(rsp, msg, 481);
      mDialog.send(rsp);

      sendBye();
      // !jf! should we make some other callback here
      transition(Terminated);
      mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure, &msg);
   }
   else
   {
      // ignore. could be PRACK/200
   }
}

void
InviteSession::dispatchCancel(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   assert(msg.header(h_CSeq).method() == CANCEL);
   if(msg.isRequest())
   {
      SipMessage rsp;
      mDialog.makeResponse(rsp, msg, 200);
      mDialog.send(rsp);

      sendBye();
      // !jf! should we make some other callback here
      transition(Terminated);
      handler->onTerminated(getSessionHandle(), InviteSessionHandler::PeerEnded, &msg);
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
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;

   if (msg.isRequest())
   {

      SipMessage rsp;
      InfoLog (<< "Received " << msg.brief());
      mDialog.makeResponse(rsp, msg, 200);
      mDialog.send(rsp);

      // !jf! should we make some other callback here
      transition(Terminated);
      handler->onTerminated(getSessionHandle(), InviteSessionHandler::PeerEnded, &msg);
      mDum.destroy(this);
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
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   if (msg.isRequest())
   {
      InfoLog (<< "Received " << msg.brief());
      mDialog.makeResponse(mLastNitResponse, msg, 200);
      handler->onInfo(getSessionHandle(), msg);
   }
   else
   {
      assert(mNitState == NitProceeding);
      mNitState = NitComplete;
      //!dcm! -- toss away 1xx to an info?
      if (msg.header(h_StatusLine).statusCode() >= 300)
      {
         handler->onInfoFailure(getSessionHandle(), msg);
      }
      else if (msg.header(h_StatusLine).statusCode() >= 200)
      {
         handler->onInfoSuccess(getSessionHandle(), msg);
      }
   }
}

void
InviteSession::acceptInfo(int statusCode)
{
   if (statusCode / 100  != 2)
   {
      throw UsageUseException("Must accept with a 2xx", __FILE__, __LINE__);
   }

   mLastNitResponse.header(h_StatusLine).statusCode() = statusCode;   
   send(mLastNitResponse);   
} 

void
InviteSession::rejectInfo(int statusCode)
{
   if (statusCode < 400)
   {
      throw UsageUseException("Must reject with a >= 4xx", __FILE__, __LINE__);
   }
   mLastNitResponse.header(h_StatusLine).statusCode() = statusCode;   
   send(mLastNitResponse);
}

void
InviteSession::startRetransmit200Timer()
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

void 
InviteSession::setSessionTimerHeaders(SipMessage &msg)
{
   if(mSessionInterval >= 90)  // If mSessionInterval is 0 then SessionTimers are considered disabled
   {
      msg.header(h_SessionExpires).value() = mSessionInterval;
      if(msg.isRequest())
      {
         msg.header(h_SessionExpires).param(p_refresher) = Data(mSessionRefresher ? "uac" : "uas");
      }
      else
      {
         msg.header(h_SessionExpires).param(p_refresher) = Data(mSessionRefresher ? "uas" : "uac");
      }
      msg.header(h_MinSE).value() = mMinSE;
   }
   else
   {
      msg.remove(h_SessionExpires);
      msg.remove(h_MinSE);
   }
}

void
InviteSession::sessionRefresh()
{
   if (updateMethodSupported())
   {
      transition(SentUpdate);
      mDialog.makeRequest(mLastSessionModification, UPDATE);
      mLastSessionModification.releaseContents();  // Don't send SDP
   }
   else
   {
      transition(SentReinvite);
      mDialog.makeRequest(mLastSessionModification, INVITE);
      InviteSession::setSdp(mLastSessionModification, mCurrentLocalSdp.get());
      mProposedLocalSdp = InviteSession::makeSdp(*mCurrentLocalSdp.get(), 0);
   }
   setSessionTimerHeaders(mLastSessionModification);

   InfoLog (<< "sessionRefresh: Sending " << mLastSessionModification.brief());
   mDialog.send(mLastSessionModification, mCurrentEncryptionLevel);
}

void
InviteSession::setSessionTimerPreferences()
{
   mSessionInterval = mDialog.mDialogSet.getUserProfile()->getDefaultSessionTime();  // Used only if remote doesn't request a time
   if(mSessionInterval != 0)
   {
       // If session timers are no disabled then ensure interval is greater than or equal to MinSE
       mSessionInterval = resipMax(mMinSE, mSessionInterval);
   }
   switch(mDialog.mDialogSet.getUserProfile()->getDefaultSessionTimerMode())
   {
   case Profile::PreferLocalRefreshes:
      mSessionRefresher = true;   // Default refresher is Local
      break;
   case Profile::PreferRemoteRefreshes:
      mSessionRefresher = false;  // Default refresher is Remote
      break;
   case Profile::PreferUASRefreshes:  
      mSessionRefresher = dynamic_cast<ServerInviteSession*>(this) != NULL; // Default refresher is UAS (for the session) - callee
      break;
   case Profile::PreferUACRefreshes:
      mSessionRefresher = dynamic_cast<ClientInviteSession*>(this) != NULL; // Default refresher is UAC (for the session) - caller
      break;
   }
}

void
InviteSession::startSessionTimer()
{
   if(mSessionInterval >= 90)  // 90 is the absolute minimum - RFC4028
   {
      // Check if we are the refresher
      if(mSessionRefresher)
      {
         // Start Session-Refresh Timer to mSessionInterval / 2 (recommended by RFC4028)
         mDum.addTimer(DumTimeout::SessionRefresh, mSessionInterval / 2, getBaseHandle(), ++mSessionTimerSeq);
      }
      else
      {
         // Start Session-Expiration Timer to mSessionInterval - BYE should be sent a minimum of 32 and one third of the SessionInterval, seconds before the session expires (recommended by RFC4028)
         mDum.addTimer(DumTimeout::SessionExpiration, mSessionInterval - resipMin(32,mSessionInterval/3), getBaseHandle(), ++mSessionTimerSeq);
      }
   }
   else  // Session Interval less than 90 - consider timers disabled
   {
       ++mSessionTimerSeq;  // increment seq, incase old timers are running and now session timers are disabled
   }
}

void
InviteSession::handleSessionTimerResponse(const SipMessage& msg)
{
   assert(msg.header(h_CSeq).method() == INVITE || msg.header(h_CSeq).method() == UPDATE);

   // If session timers are locally supported then handle response
   if(mDum.getMasterProfile()->getSupportedOptionTags().find(Token(Symbols::Timer)))
   {
      setSessionTimerPreferences();

      if(msg.exists(h_Requires) && msg.header(h_Requires).find(Token(Symbols::Timer))
         && !msg.exists(h_SessionExpires))
      {
         // If no Session Expires in response and Requires header is present then session timer is to be 'turned off'
         mSessionInterval = 0;
      }
      // Process Session Timer headers
      else if(msg.exists(h_SessionExpires))
      {
         mSessionInterval = msg.header(h_SessionExpires).value();
         if(msg.header(h_SessionExpires).exists(p_refresher))
         {
             // Remote end specified refresher preference
             mSessionRefresher = (msg.header(h_SessionExpires).param(p_refresher) == Data("uac"));
         }
      }
      else
      {
         // Note:  If no Requires or Session-Expires, then UAS does not support Session Timers
         // - we are free to use our SessionInterval settings (set above as a default)
         // If far end doesn't support then refresher must be local
         mSessionRefresher = true;
      }

      // Update MinSE if specified and longer than current value
      if(msg.exists(h_MinSE))
      {
          mMinSE = resipMax(mMinSE, msg.header(h_MinSE).value());
      }

      startSessionTimer();
   }
}

void
InviteSession::handleSessionTimerRequest(SipMessage &response, const SipMessage& request)
{
   assert(request.header(h_CSeq).method() == INVITE || request.header(h_CSeq).method() == UPDATE);

   // If session timers are locally supported then add necessary headers to response
   if(mDum.getMasterProfile()->getSupportedOptionTags().find(Token(Symbols::Timer)))
   {
      setSessionTimerPreferences();

      // Check if far-end supports
      bool farEndSupportsTimer = false;
      if(request.exists(h_Supporteds) && request.header(h_Supporteds).find(Token(Symbols::Timer)))
      {
         farEndSupportsTimer = true;
         if(request.exists(h_SessionExpires))
         {
            // Use Session Interval requested by remote - if none then use local settings
            mSessionInterval = request.header(h_SessionExpires).value();
            if(request.header(h_SessionExpires).exists(p_refresher))
            {
                mSessionRefresher = (request.header(h_SessionExpires).param(p_refresher) == Data("uas"));
            }
         }

         // Update MinSE if specified and longer than current value
         if(request.exists(h_MinSE))
         {
             mMinSE = resipMax(mMinSE, request.header(h_MinSE).value());
         }
      }
      else
      {
         // If far end doesn't support then refresher must be local
         mSessionRefresher = true;
      }

      // Add Session-Expires to response if required
      if(mSessionInterval >= 90)
      {
         if(farEndSupportsTimer)
         {
            // If far end supports session-timer then require it, if not already present
            if(!response.header(h_Requires).find(Token(Symbols::Timer)))
            {
                response.header(h_Requires).push_back(Token(Symbols::Timer));
            }
         }
         setSessionTimerHeaders(response);
      }

      startSessionTimer();
   }
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
      case UAS_OfferProvidedAnswer:
         return "UAS_OfferProvidedAnswer";
      case UAS_EarlyOffer:
         return "UAS_EarlyOffer";
      case UAS_EarlyProvidedAnswer:
         return "UAS_EarlyProvidedAnswer";
      case UAS_NoOffer:
         return "UAS_NoOffer";
      case UAS_ProvidedOffer:
         return "UAS_ProvidedOffer";
      case UAS_EarlyNoOffer:
         return "UAS_EarlyNoOffer";
      case UAS_EarlyProvidedOffer:
         return "UAS_EarlyProvidedOffer";
      case UAS_Accepted:
         return "UAS_Accepted";
      case UAS_WaitingToOffer:
         return "UAS_WaitingToOffer";
      case UAS_AcceptedWaitingAnswer:
         return "UAS_AcceptedWaitingAnswer";
      case UAC_Early:
         return "UAC_Early";
      case UAC_EarlyWithOffer:
         return "UAC_EarlyWithOffer";
      case UAC_EarlyWithAnswer:
         return "UAC_EarlyWithAnswer";
      case UAC_Answered:
         return "UAC_Answered";
      case UAC_SentUpdateEarly:
         return "UAC_SentUpdateEarly";
      case UAC_SentUpdateConnected:
         return "UAC_SentUpdateConnected";
      case UAC_ReceivedUpdateEarly:
         return "UAC_ReceivedUpdateEarly";
      case UAC_SentAnswer:
         return "UAC_SentAnswer";
      case UAC_QueuedUpdate:
         return "UAC_QueuedUpdate";
      case UAC_Cancelled:
         return "UAC_Cancelled";

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
   }
   assert(0);
   return "Undefined";
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
   // Ensure supported both locally and remotely
   return msg.exists(h_Supporteds) && msg.header(h_Supporteds).find(Token(Symbols::C100rel)) &&
          mDum.getMasterProfile()->getSupportedOptionTags().find(Token(Symbols::C100rel));
}

std::auto_ptr<SdpContents>
InviteSession::getSdp(const SipMessage& msg)
{
   // !jf! this code doesn't yet work - definitely if USE_SSL=false
   //Helper::ContentsSecAttrs attrs = Helper::extractFromPkcs7(msg, mDum.getSecurity());
   //return std::auto_ptr<SdpContents>(dynamic_cast<SdpContents*>(attrs.mContents.get()));
   SdpContents* sdp = dynamic_cast<SdpContents*>(msg.getContents());
   if (sdp)
   {
      SdpContents* cloned = static_cast<SdpContents*>(sdp->clone());
      return std::auto_ptr<SdpContents>(cloned);
   }
   else
   {
      static std::auto_ptr<SdpContents> empty;
      return empty;
   }
}

std::auto_ptr<SdpContents>
InviteSession::makeSdp(const SdpContents& sdp)
{
   return std::auto_ptr<SdpContents>(static_cast<SdpContents*>(sdp.clone()));
}

auto_ptr<Contents>
InviteSession::makeSdp(const SdpContents& sdp,
                       const SdpContents* alternative)
{
   if (alternative)
   {
      MultipartAlternativeContents* mac = new MultipartAlternativeContents;
      mac->parts().push_back(alternative->clone());
      mac->parts().push_back(sdp.clone());
      return auto_ptr<Contents>(mac);
   }
   else
   {
      return auto_ptr<Contents>(sdp.clone());
   }
}

void
InviteSession::setSdp(SipMessage& msg, const SdpContents& sdp, const SdpContents* alternative)
{
   // !jf! should deal with multipart here

   // This will clone the sdp since the InviteSession also wants to keep its own
   // copy of the sdp around for the application to access
   if (alternative)
   {
      MultipartAlternativeContents* mac = new MultipartAlternativeContents;
      mac->parts().push_back(alternative->clone());
      mac->parts().push_back(sdp.clone());
      msg.setContents(auto_ptr<Contents>(mac));
   }
   else
   {
      msg.setContents(&sdp);
   }
}

void
InviteSession::setSdp(SipMessage& msg, const Contents* sdp)
{
   assert(sdp);
   msg.setContents(sdp);
}

InviteSession::Event
InviteSession::toEvent(const SipMessage& msg, const SdpContents* sdp)
{
   MethodTypes method = msg.header(h_CSeq).method();
   int code = msg.isResponse() ? msg.header(h_StatusLine).statusCode() : 0;
   bool reliable = isReliable(msg);
   bool sentOffer = mProposedLocalSdp.get();

   if (code == 481 || code == 408)
   {
      return OnGeneralFailure;
   }
   else if (code >= 300 && code <= 399)
   {
      return OnRedirect;
   }
   else if (method == INVITE && code == 0)
   {
      if (sdp)
      {
         if (reliable)
         {
            return OnInviteReliableOffer;
         }
         else
         {
            return OnInviteOffer;
         }
      }
      else
      {
         if (reliable)
         {
            return OnInviteReliable;
         }
         else
         {
            return OnInvite;
         }
      }
   }
   else if (method == INVITE && code > 100 && code < 200)   // !kh! 100 is handled by transaction layer.
   {
      if (reliable)
      {
         if (sdp)
         {
            if (sentOffer)
            {
               return On1xxAnswer;
            }
            else
            {
               return On1xxOffer;
            }
         }
         else
         {
            return On1xx;
         }
      }
      else
      {
         if (sdp)
         {
            return On1xxEarly;
         }
         else
         {
            return On1xx;
         }
      }
   }
   else if (method == INVITE && code >= 200 && code < 300)
   {
      if (sdp)
      {
         if (sentOffer)
         {
            return On2xxAnswer;
         }
         else
         {
            return On2xxOffer;
         }
      }
      else
      {
         return On2xx;
      }
   }
   else if (method == INVITE && code == 422)
   {
      return On422Invite;
   }
   else if (method == INVITE && code == 487)
   {
      return On487Invite;
   }
   else if (method == INVITE && code == 489)
   {
      return On489Invite;
   }
   else if (method == INVITE && code == 491)
   {
      return On491Invite;
   }
   else if (method == INVITE && code >= 400)
   {
      return OnInviteFailure;
   }
   else if (method == ACK)
   {
      if (sdp)
      {
         return OnAckAnswer;
      }
      else
      {
         return OnAck;
      }
   }
   else if (method == CANCEL && code == 0)
   {
      return OnCancel;
   }
   else if (method == CANCEL && code / 200 == 1)
   {
      return On200Cancel;
   }
   else if (method == CANCEL && code >= 400)
   {
      return OnCancelFailure;
   }
   else if (method == BYE && code == 0)
   {
      return OnBye;
   }
   else if (method == BYE && code / 200 == 1)
   {
      return On200Bye;
   }
   else if (method == PRACK && code == 0)
   {
      return OnPrack;
   }
   else if (method == PRACK && code / 200 == 1)
   {
      return On200Prack;
   }
   else if (method == UPDATE && code == 0)
   {
      if (sdp)
      {
          return OnUpdateOffer;
      }
      else
      {
          return OnUpdate;
      }
   }
   else if (method == UPDATE && code / 200 == 1)
   {
      return On200Update;
   }
   else if (method == UPDATE && code == 422)
   {
      return On422Update;
   }
   else if (method == UPDATE && code == 489)
   {
      return On489Update;
   }
   else if (method == UPDATE && code == 491)
   {
      return On491Update;
   }
   else if (method == UPDATE && code >= 400)
   {
      return OnUpdateRejected;
   }
   else
   {
      assert(0); 
      return Unknown;
   }
}

void InviteSession::sendAck(const SdpContents *sdp)
{
   SipMessage ack;
   mDialog.makeRequest(ack, ACK);
   if(sdp != 0)
   {
      setSdp(ack, *sdp);
   }
   InfoLog (<< "Sending " << ack.brief());
   mDialog.send(ack);
}

void InviteSession::sendBye()
{
   SipMessage bye;
   mDialog.makeRequest(bye, BYE);
   InfoLog (<< "Sending " << bye.brief());
   mDialog.send(bye);
}

DialogUsageManager::EncryptionLevel InviteSession::getEncryptionLevel(const SipMessage& msg)
{
   DialogUsageManager::EncryptionLevel level = DialogUsageManager::None;
   const SecurityAttributes* secAttr = msg.getSecurityAttributes();
   if (secAttr)
   {
      SignatureStatus sig = secAttr->getSignatureStatus();
      bool sign = (SignatureTrusted == sig || SignatureCATrusted == sig || SignatureSelfSigned == sig);
      bool encrypted = secAttr->isEncrypted();
      if (encrypted && sign ) level = DialogUsageManager::SignAndEncrypt;
      else if (encrypted) level = DialogUsageManager::Encrypt;
      else if (sign) level = DialogUsageManager::Sign;
   }
   return level;
}

void InviteSession::setCurrentLocalSdp(const SipMessage& msg)
{
   assert(mProposedLocalSdp.get());
   if (dynamic_cast<MultipartAlternativeContents*>(mProposedLocalSdp.get()))
   {
      if (DialogUsageManager::Encrypt == getEncryptionLevel(msg) || DialogUsageManager::SignAndEncrypt == getEncryptionLevel(msg))
      {
         mCurrentLocalSdp = auto_ptr<SdpContents>(static_cast<SdpContents*>((dynamic_cast<MultipartAlternativeContents*>(mProposedLocalSdp.get()))->parts().back()->clone()));
      }
      else
      {
         mCurrentLocalSdp = auto_ptr<SdpContents>(static_cast<SdpContents*>((dynamic_cast<MultipartAlternativeContents*>(mProposedLocalSdp.get()))->parts().front()->clone()));
      }
   }
   else
   {
      mCurrentLocalSdp = auto_ptr<SdpContents>(static_cast<SdpContents*>(mProposedLocalSdp.get()->clone()));
   }
   mProposedLocalSdp.release();
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
