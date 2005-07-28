#include "resiprocate/MultipartMixedContents.hxx"
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
     mSessionRefresherUAS(false),
     mSessionTimerSeq(0),
     mSentRefer(false)
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
InviteSession::provideOffer(const SdpContents& offer)
{
   switch (mState)
   {
      case Connected:
      case WaitingToOffer:
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

         if(mSessionInterval >= 90)  // If mSessionInterval is 0 then SessionTimers are considered disabled
         {
            mLastSessionModification.header(h_SessionExpires).value() = mSessionInterval;
            mLastSessionModification.header(h_SessionExpires).param(p_refresher) = Data(mSessionRefresherUAS ? "uas" : "uac");
         }
         else
         {
            mLastSessionModification.remove(h_SessionExpires);

            mLastSessionModification.remove(h_MinSE);
         }

         InfoLog (<< "Sending " << mLastSessionModification.brief());
         mProposedLocalSdp = InviteSession::makeSdp(offer);
         InviteSession::setSdp(mLastSessionModification, offer);
         mDialog.send(mLastSessionModification);
         break;

      case Answered:
         // queue the offer to be sent after the ACK is received
         transition(WaitingToOffer);
         mProposedLocalSdp = InviteSession::makeSdp(offer);
         break;

      // !slg! Can we handle all of the states listed in isConnected() ???
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
         transition(Connected);
         mCurrentLocalSdp = InviteSession::makeSdp(answer);
         mCurrentRemoteSdp = mProposedRemoteSdp;
         mDialog.makeResponse(mInvite200, mLastSessionModification, 200);
         handleSessionTimerRequest(mInvite200, mLastSessionModification);
         InviteSession::setSdp(mInvite200, answer);
         InfoLog (<< "Sending " << mInvite200.brief());
         mDialog.send(mInvite200);
         startRetransmit200Timer();
         break;

      case ReceivedUpdate: // same as ReceivedReinvite case.
      {
         transition(Connected);

         SipMessage response;
         mDialog.makeResponse(response, mLastSessionModification, 200);
         handleSessionTimerRequest(response, mLastSessionModification);
         mCurrentLocalSdp = InviteSession::makeSdp(answer);
         mCurrentRemoteSdp = mProposedRemoteSdp;
         InviteSession::setSdp(response, answer);
         InfoLog (<< "Sending " << response.brief());
         mDialog.send(response);
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
         transition(Terminated);

         // !jf! do we need to store the BYE somewhere?
         SipMessage bye;
         mDialog.makeRequest(bye, BYE);
         InfoLog (<< "Sending " << bye.brief());
         mDialog.send(bye);
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
         transition(Terminated);

         SipMessage response;
         mDialog.makeResponse(response, mLastSessionModification, 488);
         InfoLog (<< "Sending " << response.brief());
         mDialog.send(response);

         SipMessage bye;
         mDialog.makeRequest(bye, BYE);
         InfoLog (<< "Sending " << bye.brief());
         mDialog.send(bye);
         break;
      }

      case WaitingToTerminate:
      {
         transition(Terminated);

         SipMessage bye;
         mDialog.makeRequest(bye, BYE);
         InfoLog (<< "Sending " << bye.brief());
         mDialog.send(bye);
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
   //InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   //handler->onTerminated(getSessionHandle());
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
         mDialog.send(info);
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
         mDialog.send(mInvite200);
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
      }
   }
   else if (timeout.type() == DumTimeout::Glare)
   {
      if (mState == SentUpdateGlare)
      {
         transition(SentUpdate);

         InfoLog (<< "Retransmitting the UPDATE (glare condition timer)");
         mDialog.send(mLastSessionModification);
      }
      else if (mState == SentReinviteGlare)
      {
         transition(SentReinvite);

         InfoLog (<< "Retransmitting the reINVITE (glare condition timer)");
         mDialog.send(mLastSessionModification);
      }
   }
   else if (timeout.type() == DumTimeout::SessionExpiration)
   {
      if(timeout.seq() == mSessionTimerSeq)
      {
         mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::SessionExpired);
         end();  // end expired session
      }
   }
   else if (timeout.type() == DumTimeout::SessionRefresh)
   {
     if(timeout.seq() == mSessionTimerSeq)
      {
         if(isConnected())  // !slg! this check isn't 100% safe - ie. provideOffer will throw if in some states - ie. SentReInvite, etc.
         {
            provideOffer(*mCurrentLocalSdp);
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
         //handler->onDialogModified(getSessionHandle(), Offer, msg);
         handler->onOffer(getSessionHandle(), msg, *sdp);
         break;

      case On2xx:
      case On2xxOffer:
      case On2xxAnswer:
         // retransmission of 200I
         {
            // !jf! Need to include the answer here.
            SipMessage ack;
            mDialog.makeRequest(ack, ACK);
            // ack.header(h_CSeq).sequence() = msg.header(h_CSeq).sequence(); // !slg! not required since change in Dialog.cxx to store ACK CSeq when 200 to INVITE is received
            mDialog.send(ack);
         }
         break;

      case OnUpdate:
         transition(ReceivedUpdate);

         //  !kh!
         //  Find out if it's an UPDATE requiring state change.
         //  See rfc3311 5.2, 4th paragraph.
         mLastSessionModification = msg;
         handler->onOffer(getSessionHandle(), msg, *sdp);
         break;

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
            mCurrentLocalSdp = mProposedLocalSdp;
            mCurrentRemoteSdp = InviteSession::makeSdp(*sdp);
            handler->onAnswer(getSessionHandle(), msg, *sdp);
         }
         else
         {
            handler->onIllegalNegotiation(getSessionHandle(), msg);
         }
         break;

      case On491Update:
         transition(SentReinviteGlare);
         start491Timer();
         break;

      case OnUpdateRejected:
         // !jf!
         assert(0);
         break;

      case OnGeneralFailure:
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure, &msg);
         end();
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
         mCurrentLocalSdp = mProposedLocalSdp;
         mCurrentRemoteSdp = InviteSession::makeSdp(*sdp);
         // !jf! I need to potentially include an answer in the ACK here
         SipMessage ack;
         mDialog.makeRequest(ack, ACK);
         mDialog.send(ack);

         handler->onAnswer(getSessionHandle(), msg, *sdp);


         // !jf! do I need to allow a reINVITE overlapping the retransmission of
         // the ACK when a 200I is received? If yes, then I need to store all
         // ACK messages for 64*T1
         break;
      }
      case On2xx:
      {
         transition(Connected);
         SipMessage ack;
         mDialog.makeRequest(ack, ACK);
         mDialog.send(ack);

         handleSessionTimerResponse(msg);
         handler->onIllegalNegotiation(getSessionHandle(), msg);

         break;
      }

      case On491Invite:
         transition(SentUpdateGlare);
         start491Timer();
         break;

      case OnGeneralFailure:
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure, &msg);
         end();
         break;

      case OnInviteFailure:
         transition(Connected);
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
      transition(ReceivedReinvite);
      handler->onOfferRejected(getSessionHandle(), msg);
   }
   else if (method == UPDATE && msg.isRequest())
   {
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
      std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);
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
      transition(Terminated);

      // !jf! Need to include the answer here.
      SipMessage ack;
      mDialog.makeRequest(ack, ACK);
      // ack.header(h_CSeq).sequence() = msg.header(h_CSeq).sequence(); // !slg! not required since change in Dialog.cxx to store ACK CSeq when 200 to INVITE is received
      mDialog.send(ack);
   }
   else if (msg.isResponse() && msg.header(h_CSeq).method() == INVITE)
   {
      end();
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
      case NOTIFY:
         dispatchNotify(msg);
         break;
      case REFER:
         dispatchRefer(msg);
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
InviteSession::dispatchNotify(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   if (msg.isRequest())
   {
      if (!msg.exists(h_Event) || msg.header(h_Event).value() == "refer")
      {
         DebugLog (<< "Making subscription from refer: " << msg);
         ClientSubscription* sub = mDialog.makeClientSubscription(msg);
         mDialog.mClientSubscriptions.push_back(sub);
         ClientSubscriptionHandle client = sub->getHandle();
         sub->dispatch(msg);
         if (client.isValid())
         {
            handler->onReferAccepted(getSessionHandle(), client, msg);
         }
         else
         {
            handler->onReferRejected(getSessionHandle(), msg);
         }
      }
      else
      {
         SipMessage response;
         mDialog.makeResponse(response, msg, 406);
         mDialog.send(response);
      }
   }
}

void
InviteSession::dispatchRefer(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   if (msg.isRequest())
   {
      ServerSubscription* server = mDialog.makeServerSubscription(msg);
      mDialog.mServerSubscriptions.push_back(server);
      ServerSubscriptionHandle serverHandle = server->getHandle();
      server->dispatch(msg);
      if (serverHandle.isValid())
      {
         handler->onRefer(getSessionHandle(), serverHandle, msg);
      }
   }
   else if (msg.isResponse())
   {
      int code = msg.header(h_StatusLine).statusCode();
      if (code < 300)
      {
         // throw it away
         return;
      }
      else
      {
         handler->onReferRejected(getSessionHandle(), msg);
      }
   }
}

void
InviteSession::dispatchUnhandledInvite(const SipMessage& msg)
{
   assert(msg.isRequest());
   assert(msg.header(h_CSeq).method() == INVITE);

   transition(Terminated);

   // If we get an INVITE request from the wire and we are not in
   // Connected state, reject the request and send a BYE
   SipMessage response;
   mDialog.makeResponse(response, msg, 400); // !jf! what code to use?
   InfoLog (<< "Sending " << response.brief());
   mDialog.send(response);

   SipMessage bye;
   mDialog.makeRequest(bye, BYE);
   InfoLog (<< "Sending " << bye.brief());
   mDialog.send(bye);
}

void
InviteSession::dispatchPrack(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;

   assert(msg.header(h_CSeq).method() == PRACK);
   if(msg.isRequest())
   {
      transition(Terminated);

      // !jf! should we make some other callback here
      handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure, &msg);

      SipMessage rsp;
      mDialog.makeResponse(rsp, msg, 481);
      mDialog.send(rsp);

      SipMessage bye;
      mDialog.makeRequest(bye, BYE);
      InfoLog (<< "Sending " << bye.brief());
      mDialog.send(bye);

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
      transition(Terminated);

      // !jf! should we make some other callback here
      handler->onTerminated(getSessionHandle(), InviteSessionHandler::PeerEnded, &msg);

      SipMessage rsp;
      mDialog.makeResponse(rsp, msg, 200);
      mDialog.send(rsp);

      SipMessage bye;
      mDialog.makeRequest(bye, BYE);
      InfoLog (<< "Sending " << bye.brief());
      mDialog.send(bye);

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
      transition(Terminated);

      SipMessage rsp;
      InfoLog (<< "Received " << msg.brief());
      mDialog.makeResponse(rsp, msg, 200);
      mDialog.send(rsp);

      // !jf! should we make some other callback here
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
      SipMessage response;
      mDialog.makeResponse(response, msg, 200);
      mDialog.send(response);
      handler->onInfo(getSessionHandle(), msg);
   }
   else
   {
      assert(mNitState == NitProceeding);
      mNitState = NitComplete;

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
InviteSession::handleSessionTimerResponse(const SipMessage& msg)
{
   assert(msg.header(h_CSeq).method() == INVITE || msg.header(h_CSeq).method() == UPDATE);

   // If session timers are locally supported then handle response
   if(mDum.getMasterProfile()->getSupportedOptionTags().find(Token(Symbols::Timer)))
   {
      Profile::SessionTimerMode mode = mDialog.mDialogSet.getUserProfile()->getDefaultSessionTimerMode();
      bool fUAS = dynamic_cast<ServerInviteSession*>(this) != NULL;

      // Set Defaults
      mSessionInterval = mDialog.mDialogSet.getUserProfile()->getDefaultSessionTime();  // Used only if response didn't request a time
      switch(mDialog.mDialogSet.getUserProfile()->getDefaultSessionTimerMode())
      {
      case Profile::PreferLocalRefreshes:
         mSessionRefresherUAS = fUAS;   // Default refresher is Local
         break;
      case Profile::PreferRemoteRefreshes:
         mSessionRefresherUAS = !fUAS;  // Default refresher is Remote
         break;
      case Profile::PreferUACRefreshes:
         mSessionRefresherUAS = false;  // Default refresher is UAC
         break;
      case Profile::PreferUASRefreshes:
         mSessionRefresherUAS = true;   // Default refresher is UAS
         break;
      }

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
             mSessionRefresherUAS = (msg.header(h_SessionExpires).param(p_refresher) == Data("uas"));
         }
      }
      else
      {
         // Note:  If no Requires or Session-Expires, then UAS does not support Session Timers
         // - we are free to use our SessionInterval settings (set above as a default)
         // If far end doesn't support then refresher must be local
         mSessionRefresherUAS = fUAS;
      }

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
      else
      {
          ++mSessionTimerSeq;  // increment seq, incase old timers are running and now session timers are disabled
      }
   }
}

void
InviteSession::handleSessionTimerRequest(SipMessage &response, const SipMessage& request)
{
   assert(request.header(h_CSeq).method() == INVITE || request.header(h_CSeq).method() == UPDATE);

   // If session timers are locally supported then add necessary headers to response
   if(mDum.getMasterProfile()->getSupportedOptionTags().find(Token(Symbols::Timer)))
   {
      bool fUAS = dynamic_cast<ServerInviteSession*>(this) != NULL;

      mSessionInterval = mDialog.mDialogSet.getUserProfile()->getDefaultSessionTime();  // Used only if remote doesn't request a time
      switch(mDialog.mDialogSet.getUserProfile()->getDefaultSessionTimerMode())
      {
      case Profile::PreferLocalRefreshes:
         mSessionRefresherUAS = fUAS;   // Default refresher is Local
         break;
      case Profile::PreferRemoteRefreshes:
         mSessionRefresherUAS = !fUAS;  // Default refresher is Remote
         break;
      case Profile::PreferUACRefreshes:
         mSessionRefresherUAS = false;  // Default refresher is UAC
         break;
      case Profile::PreferUASRefreshes:
         mSessionRefresherUAS = true;   // Default refresher is UAS
         break;
      }

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
                mSessionRefresherUAS = (request.header(h_SessionExpires).param(p_refresher) == Data("uas"));
            }
         }
      }
      else
      {
         // If far end doesn't support then refresher must be local
         mSessionRefresherUAS = fUAS;
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
      else
      {
          ++mSessionTimerSeq;  // increment seq, incase old timers are running and now session timers are disabled
      }
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

void
InviteSession::setSdp(SipMessage& msg, const SdpContents& sdp)
{
   // !jf! should deal with multipart here

   // This will clone the sdp since the InviteSession also wants to keep its own
   // copy of the sdp around for the application to access
   msg.setContents(&sdp);
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
   else if (method == INVITE && code > 100 && code < 200)
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
      return OnUpdate;
   }
   else if (method == UPDATE && code / 200 == 1)
   {
      return On200Update;
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
      //assert(0); !slg! need to remove to allow Refer to be handled by dispatchOthers
      // !slg! should there be OnRefer events returned here???
      return Unknown;
   }
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
