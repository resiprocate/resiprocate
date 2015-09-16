#include "resip/stack/MultipartMixedContents.hxx"
#include "resip/stack/MultipartAlternativeContents.hxx"
#include "resip/stack/SdpContents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/dum/BaseCreator.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/DialogEventStateManager.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/InviteSession.hxx"
#include "resip/dum/ServerInviteSession.hxx"
#include "resip/dum/ClientSubscription.hxx"
#include "resip/dum/ServerSubscription.hxx"
#include "resip/dum/ClientInviteSession.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/UsageUseException.hxx"
#include "resip/dum/DumHelper.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Timer.hxx"
#include "rutil/Random.hxx"
#include "rutil/compat.hxx"
#include "rutil/WinLeakCheck.hxx"

// Remove warning about 'this' use in initiator list - pointer is only stored
#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4355 ) // using this in base member initializer list
#pragma warning( disable : 4800 ) // forcing value to bool (performance warning)
#endif

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM
#define THROW(msg)  throw DialogUsage::Exception(msg, __FILE__,__LINE__);

using namespace resip;
using namespace std;

Data EndReasons[] =
{
   "Not Specified",
   "User Hung Up",
   "Application Rejected Sdp(usually no common codec)",
   "Illegal Sdp Negotiation",
   "ACK not received",
   "Session Timer Expired",
   "Stale re-Invite"
};

const Data& InviteSession::getEndReasonString(InviteSession::EndReason reason)
{
   if(reason != InviteSession::UserSpecified)
   {
      resip_assert(reason >= InviteSession::NotSpecified && reason < InviteSession::ENDREASON_MAX); //!dcm! -- necessary?
      return EndReasons[reason];
   }
   else
   {
      return mUserEndReason;
   }
}

InviteSession::InviteSession(DialogUsageManager& dum, Dialog& dialog)
   : DialogUsage(dum, dialog),
     mState(Undefined),
     mNitState(NitComplete),
     mServerNitState(NitComplete),
     mLastLocalSessionModification(new SipMessage),
     mLastRemoteSessionModification(new SipMessage),
     mInvite200(new SipMessage),
     mLastNitResponse(new SipMessage),
     mCurrentRetransmit200(0),
     mStaleReInviteTimerSeq(1),
     mSessionInterval(0),
     mMinSE(90), 
     mSessionRefresher(false),
     mSessionTimerSeq(0),
     mSessionRefreshReInvite(false),
     mReferSub(true),
     mCurrentEncryptionLevel(DialogUsageManager::None),
     mProposedEncryptionLevel(DialogUsageManager::None),
     mEndReason(NotSpecified)
{
   DebugLog ( << "^^^ InviteSession::InviteSession " << this);
   resip_assert(mDum.mInviteSessionHandler);
}

InviteSession::~InviteSession()
{
   DebugLog ( << "^^^ InviteSession::~InviteSession " << this);
   mDialog.mInviteSession = 0;
   while(!mNITQueue.empty())
   {
      delete mNITQueue.front();
      mNITQueue.pop();
   }
}

bool
InviteSession::hasLocalOfferAnswer() const
{
   return (mCurrentLocalOfferAnswer.get());
}

const Contents&
InviteSession::getLocalOfferAnswer() const
{
   if(mCurrentLocalOfferAnswer.get())
   {
      return *mCurrentLocalOfferAnswer;
   }
   else
   {
      return SdpContents::Empty;
   }
}

bool
InviteSession::hasRemoteOfferAnswer() const
{
   return (mCurrentRemoteOfferAnswer.get());
}

const Contents&
InviteSession::getRemoteOfferAnswer() const
{
   if(mCurrentRemoteOfferAnswer.get())
   {
      return *mCurrentRemoteOfferAnswer;
   }
   else
   {
      return SdpContents::Empty;
   }
}

bool
InviteSession::hasProposedRemoteOfferAnswer() const
{
   return (mProposedRemoteOfferAnswer.get());
}

const Contents&
InviteSession::getProposedRemoteOfferAnswer() const
{
   if(mProposedRemoteOfferAnswer.get())
   {
      return *mProposedRemoteOfferAnswer;
   }
   else
   {
      return SdpContents::Empty;
   }
}

bool
InviteSession::hasLocalSdp() const
{
   resip_assert(!mDum.mInviteSessionHandler->isGenericOfferAnswer());
   return (mCurrentLocalOfferAnswer.get());
}

const SdpContents&
InviteSession::getLocalSdp() const
{
   resip_assert(!mDum.mInviteSessionHandler->isGenericOfferAnswer());
   if(mCurrentLocalOfferAnswer.get())
   {
      const SdpContents* sdp = dynamic_cast<const SdpContents*>(mCurrentLocalOfferAnswer.get());
      resip_assert(sdp);
      return *sdp;
   }
   else
   {
      return SdpContents::Empty;
   }
}

bool
InviteSession::hasRemoteSdp() const
{
   resip_assert(!mDum.mInviteSessionHandler->isGenericOfferAnswer());
   return (mCurrentRemoteOfferAnswer.get());
}

const SdpContents&
InviteSession::getRemoteSdp() const
{
   resip_assert(!mDum.mInviteSessionHandler->isGenericOfferAnswer());
   if(mCurrentRemoteOfferAnswer.get())
   {
      const SdpContents* sdp = dynamic_cast<const SdpContents*>(mCurrentRemoteOfferAnswer.get());
      resip_assert(sdp);
      return *sdp;
   }
   else
   {
      return SdpContents::Empty;
   }
}

bool
InviteSession::hasProposedRemoteSdp() const
{
   resip_assert(!mDum.mInviteSessionHandler->isGenericOfferAnswer());
   return (mProposedRemoteOfferAnswer.get());
}

const SdpContents&
InviteSession::getProposedRemoteSdp() const
{
   resip_assert(!mDum.mInviteSessionHandler->isGenericOfferAnswer());
   if(mProposedRemoteOfferAnswer.get())
   {
      const SdpContents* sdp = dynamic_cast<const SdpContents*>(mProposedRemoteOfferAnswer.get());
      resip_assert(sdp);
      return *sdp;
   }
   else
   {
      return SdpContents::Empty;
   }
}

InviteSessionHandle
InviteSession::getSessionHandle()
{
   return InviteSessionHandle(mDum, getBaseHandle().getId());
}

void InviteSession::storePeerCapabilities(const SipMessage& msg)
{
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
   if (msg.exists(h_AllowEvents))
   {
      mPeerAllowedEvents = msg.header(h_AllowEvents);
   }
   if (msg.exists(h_Accepts))
   {
      mPeerSupportedMimeTypes = msg.header(h_Accepts);
   }
   if (msg.exists(h_UserAgent))
   {
      mPeerUserAgent = msg.header(h_UserAgent).value();
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
      case SentReinviteNoOffer:
      case SentReinviteAnswered:
      case SentReinviteNoOfferGlare:
      case ReceivedUpdate:
      case ReceivedReinvite:
      case ReceivedReinviteNoOffer:
      case ReceivedReinviteSentOffer:
      case Answered:
      case WaitingToOffer:
      case WaitingToRequestOffer:
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
      case UAC_Early:
      case UAC_EarlyWithOffer:
      case UAC_EarlyWithAnswer:
      case UAC_SentUpdateEarly:
      case UAC_ReceivedUpdateEarly:
      case UAC_SentAnswer:
      case UAC_QueuedUpdate:
         return true;
      default:
         return false;
   }
}

bool 
InviteSession::isAccepted() const
{
   switch (mState)
   {
      case UAS_Start:
      case UAS_Offer:
      case UAS_OfferProvidedAnswer:
      case UAS_EarlyOffer:
      case UAS_EarlyProvidedAnswer:

      case UAS_NoOffer:
      case UAS_ProvidedOffer:
      case UAS_EarlyNoOffer:
      case UAS_EarlyProvidedOffer:
      //case UAS_Accepted:              // Obvious
      //case UAS_WaitingToOffer:        // We have accepted here and are waiting for ACK to Offer
      //case UAS_WaitingToRequestOffer: // We have accepted here and are waiting for ACK to request an offer

      //case UAS_AcceptedWaitingAnswer: // Obvious
      case UAS_OfferReliable:
      case UAS_OfferReliableProvidedAnswer:
      case UAS_NoOfferReliable:
      case UAS_ProvidedOfferReliable:
      case UAS_FirstSentOfferReliable:
      case UAS_FirstSentAnswerReliable:
      case UAS_NoAnswerReliableWaitingPrack:
      case UAS_NegotiatedReliable:
      case UAS_NoAnswerReliable:
      case UAS_SentUpdate:
      //case UAS_SentUpdateAccepted:    // we have accepted here
      case UAS_SentUpdateGlare:
      case UAS_ReceivedUpdate:
      //case UAS_ReceivedUpdateWaitingAnswer:  // happens only after accept is called
      //case UAS_WaitingToHangup:       // always from an accepted state
         return false;

      default:
         return true;
   }
}

bool
InviteSession::isTerminated() const
{
   switch (mState)
   {
      case Terminated:
      case WaitingToTerminate:
      case WaitingToHangup:
      case UAC_Cancelled:
      case UAS_WaitingToHangup:
         return true;
      default:
         return false;
   }
}

EncodeStream&
InviteSession::dump(EncodeStream& strm) const
{
   strm << "INVITE: " << mId
        << " " << toData(mState)
        << " ADDR=" << myAddr()
        << " PEER=" << peerAddr();
   return strm;
}

void
InviteSession::requestOffer()
{
   switch (mState)
   {
      case Connected:
      case WaitingToRequestOffer:
      case UAS_WaitingToRequestOffer:
         transition(SentReinviteNoOffer);
         mDialog.makeRequest(*mLastLocalSessionModification, INVITE);
         startStaleReInviteTimer();
         mLastLocalSessionModification->setContents(0);		// Clear the contents from the INVITE
         setSessionTimerHeaders(*mLastLocalSessionModification);

         InfoLog (<< "Sending " << mLastLocalSessionModification->brief());

         // call send to give app an chance to adorn the message.
         send(mLastLocalSessionModification);
         break;

      case Answered:
         // queue the offer to be sent after the ACK is received
         transition(WaitingToRequestOffer);
         break;        
         
      // ?slg? Can we handle all of the states listed in isConnected() ???
      default:
         WarningLog (<< "Can't requestOffer when not in Connected state");
         throw DialogUsage::Exception("Can't request an offer", __FILE__,__LINE__);
   }
}

void
InviteSession::provideOffer(const Contents& offer,
                            DialogUsageManager::EncryptionLevel level,
                            const Contents* alternative)
{
   switch (mState)
   {
      case Connected:
      case WaitingToOffer:
      case UAS_WaitingToOffer:
         transition(SentReinvite);
         mDialog.makeRequest(*mLastLocalSessionModification, INVITE);
         startStaleReInviteTimer();

         setSessionTimerHeaders(*mLastLocalSessionModification);

         InfoLog (<< "Sending " << mLastLocalSessionModification->brief());
         InviteSession::setOfferAnswer(*mLastLocalSessionModification, offer, alternative);
         mProposedLocalOfferAnswer = InviteSession::makeOfferAnswer(offer, alternative);
         mProposedEncryptionLevel = level;
         DumHelper::setOutgoingEncryptionLevel(*mLastLocalSessionModification, mProposedEncryptionLevel);

         // call send to give app an chance to adorn the message.
         send(mLastLocalSessionModification);
         break;

      case Answered:
         // queue the offer to be sent after the ACK is received
         transition(WaitingToOffer);
         mProposedEncryptionLevel = level;
         mProposedLocalOfferAnswer = InviteSession::makeOfferAnswer(offer, alternative);
         break;

      case ReceivedReinviteNoOffer:
         resip_assert(!mProposedRemoteOfferAnswer.get());
         transition(ReceivedReinviteSentOffer);
         mDialog.makeResponse(*mInvite200, *mLastRemoteSessionModification, 200);
         handleSessionTimerRequest(*mInvite200, *mLastRemoteSessionModification);
         InviteSession::setOfferAnswer(*mInvite200, offer, 0);
         mProposedLocalOfferAnswer  = InviteSession::makeOfferAnswer(offer);

         InfoLog (<< "Sending " << mInvite200->brief());
         DumHelper::setOutgoingEncryptionLevel(*mInvite200, mCurrentEncryptionLevel);
         send(mInvite200);
         startRetransmit200Timer();
         break;
                  
      default:
         WarningLog (<< "Incorrect state to provideOffer: " << toData(mState));
         throw DialogUsage::Exception("Can't provide an offer", __FILE__,__LINE__);
   }
}

class InviteSessionProvideOfferExCommand : public DumCommandAdapter
{
public:
   InviteSessionProvideOfferExCommand(const InviteSessionHandle& inviteSessionHandle, 
      const Contents& offer, 
      DialogUsageManager::EncryptionLevel level, 
      const Contents* alternative)
      : mInviteSessionHandle(inviteSessionHandle),
        mOffer(offer.clone()),
        mLevel(level),
		mAlternative(alternative ? alternative->clone() : 0)
   {
   }

   virtual void executeCommand()
   {
      if(mInviteSessionHandle.isValid())
      {
         mInviteSessionHandle->provideOffer(*mOffer, mLevel, mAlternative.get());
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "InviteSessionProvideOfferExCommand";
   }
private:
   InviteSessionHandle mInviteSessionHandle;
   std::auto_ptr<const Contents> mOffer;
   DialogUsageManager::EncryptionLevel mLevel;
   std::auto_ptr<const Contents> mAlternative;
};

void
InviteSession::provideOfferCommand(const Contents& offer, DialogUsageManager::EncryptionLevel level, const Contents* alternative)
{
   mDum.post(new InviteSessionProvideOfferExCommand(getSessionHandle(), offer, level, alternative));
}

void
InviteSession::provideOffer(const Contents& offer)
{
   return provideOffer(offer, mCurrentEncryptionLevel, 0);
}

class InviteSessionProvideOfferCommand : public DumCommandAdapter
{
public:
   InviteSessionProvideOfferCommand(const InviteSessionHandle& inviteSessionHandle, const Contents& offer)
      : mInviteSessionHandle(inviteSessionHandle),
      mOffer(offer.clone())
   {
   }

   virtual void executeCommand()
   {
      if(mInviteSessionHandle.isValid())
      {
         mInviteSessionHandle->provideOffer(*mOffer);
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "InviteSessionProvideOfferCommand";
   }
private:
   InviteSessionHandle mInviteSessionHandle;
   std::auto_ptr<const Contents> mOffer;
};

void
InviteSession::provideOfferCommand(const Contents& offer)
{
   mDum.post(new InviteSessionProvideOfferCommand(getSessionHandle(), offer));
}

void
InviteSession::provideAnswer(const Contents& answer)
{
   switch (mState)
   {
      case ReceivedReinvite:
         transition(Connected);
         mDialog.makeResponse(*mInvite200, *mLastRemoteSessionModification, 200);
         handleSessionTimerRequest(*mInvite200, *mLastRemoteSessionModification);
         InviteSession::setOfferAnswer(*mInvite200, answer, 0);
         mCurrentLocalOfferAnswer = InviteSession::makeOfferAnswer(answer);
         mCurrentRemoteOfferAnswer = mProposedRemoteOfferAnswer;
         InfoLog (<< "Sending " << mInvite200->brief());
         DumHelper::setOutgoingEncryptionLevel(*mInvite200, mCurrentEncryptionLevel);
         send(mInvite200);
         startRetransmit200Timer();
         break;

      case ReceivedUpdate: // same as ReceivedReinvite case.
      {
         transition(Connected);

         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, *mLastRemoteSessionModification, 200);
         handleSessionTimerRequest(*response, *mLastRemoteSessionModification);
         InviteSession::setOfferAnswer(*response, answer, 0);
         mCurrentLocalOfferAnswer = InviteSession::makeOfferAnswer(answer);
         mCurrentRemoteOfferAnswer = mProposedRemoteOfferAnswer;
         InfoLog (<< "Sending " << response->brief());
         DumHelper::setOutgoingEncryptionLevel(*response, mCurrentEncryptionLevel);
         send(response);
         break;
      }

      case SentReinviteAnswered:
         transition(Connected);
         sendAck(&answer);

         mCurrentRemoteOfferAnswer = mProposedRemoteOfferAnswer;
         mCurrentLocalOfferAnswer = InviteSession::makeOfferAnswer(answer);
         break;

      default:
         WarningLog (<< "Incorrect state to provideAnswer: " << toData(mState));
         throw DialogUsage::Exception("Can't provide an answer", __FILE__,__LINE__);
   }
}

class InviteSessionProvideAnswerCommand : public DumCommandAdapter
{
public:
   InviteSessionProvideAnswerCommand(const InviteSessionHandle& inviteSessionHandle, const Contents& answer)
      : mInviteSessionHandle(inviteSessionHandle),
        mAnswer(answer.clone())
   {
   }

   virtual void executeCommand()
   {
      if(mInviteSessionHandle.isValid())
      {
         mInviteSessionHandle->provideAnswer(*mAnswer);
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "InviteSessionProvideAnswerCommand";
   }
private:
   InviteSessionHandle mInviteSessionHandle;
   std::auto_ptr<const Contents> mAnswer;
};

void
InviteSession::provideAnswerCommand(const Contents& answer)
{
   mDum.post(new InviteSessionProvideAnswerCommand(getSessionHandle(), answer));
}

void
InviteSession::end()
{
   end(NotSpecified);
}

void
InviteSession::end(const Data& userReason)
{
   mUserEndReason = userReason;
   end(UserSpecified);
}

void
InviteSession::end(EndReason reason)
{
   if (mEndReason == NotSpecified)
   {
      mEndReason = reason;   
   }
   
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;

   switch (mState)
   {
      case Connected:
      case SentUpdate:
      case SentUpdateGlare:
      case SentReinviteGlare:
      case SentReinviteNoOfferGlare:
      case SentReinviteAnswered:
      {
         // !jf! do we need to store the BYE somewhere?
         // .dw. BYE message handled
         SharedPtr<SipMessage> msg = sendBye();
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::LocalBye, msg.get()); 
         break;
      }

      case SentReinvite:
      case SentReinviteNoOffer:
         transition(WaitingToTerminate);
         break;

      case Answered:
      case WaitingToOffer:
      case WaitingToRequestOffer:
      case ReceivedReinviteSentOffer:
         if(mCurrentRetransmit200)  // If retransmit200 timer is active then ACK is not received yet - wait for it
         {
            transition(WaitingToHangup);
         }
         else
         {
             // ACK has likely timedout - hangup immediately
             SharedPtr<SipMessage> msg = sendBye();
             transition(Terminated);
             mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::LocalBye, msg.get());
         }
         break;

      case ReceivedUpdate:
      case ReceivedReinvite:
      case ReceivedReinviteNoOffer:
      {
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, *mLastRemoteSessionModification, 488);
         InfoLog (<< "Sending " << response->brief());
         send(response);

         SharedPtr<SipMessage> msg = sendBye();
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::LocalBye, msg.get()); 
         break;
      }

      case WaitingToTerminate:  // ?slg?  Why is this here?
      {
         SharedPtr<SipMessage> msg = sendBye();
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::LocalBye, msg.get()); 
         break;
      }

      case Terminated:
         // no-op.
         break;

      default:
         resip_assert(0);
         break;
   }
}

class InviteSessionEndCommand : public DumCommandAdapter
{
public:
   InviteSessionEndCommand(const InviteSessionHandle& inviteSessionHandle, InviteSession::EndReason reason)
      : mInviteSessionHandle(inviteSessionHandle),
        mReason(reason)
   {
   }

   virtual void executeCommand()
   {
      if(mInviteSessionHandle.isValid())
      {
         mInviteSessionHandle->end(mReason);
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "InviteSessionEndCommand";
   }
private:
   InviteSessionHandle mInviteSessionHandle;
   InviteSession::EndReason mReason;
};

void
InviteSession::endCommand(EndReason reason)
{
   mDum.post(new InviteSessionEndCommand(getSessionHandle(), reason));
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

         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, *mLastRemoteSessionModification, statusCode);
         if(warning)
         {
            response->header(h_Warnings).push_back(*warning);
         }
         InfoLog (<< "Sending " << response->brief());
         send(response);
         break;
      }
      // Sent a reINVITE no offer and received a 200-offer.
      // Simply send an ACK without an answer and stay in Connected.
      case SentReinviteAnswered:
      {
         InfoLog (<< "Not sending " << statusCode << " error since transaction"
                     "already completed, sending answer-less ACK");
         transition(Connected);
         sendAck();
         break;
      }
      default:
         resip_assert(0);
         break;
   }
}

class InviteSessionRejectCommand : public DumCommandAdapter
{
public:
   InviteSessionRejectCommand(const InviteSessionHandle& inviteSessionHandle, int code, WarningCategory* warning)
      : mInviteSessionHandle(inviteSessionHandle),
        mCode(code),
        mWarning(warning?new WarningCategory(*warning):0)
   {
   }

   virtual void executeCommand()
   {
      if(mInviteSessionHandle.isValid())
      {
         mInviteSessionHandle->reject(mCode, mWarning.get());
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "InviteSessionRejectCommand";
   }
private:
   InviteSessionHandle mInviteSessionHandle;
   int mCode;
   std::auto_ptr<WarningCategory> mWarning;
};

void
InviteSession::rejectCommand(int code, WarningCategory *warning)
{
   mDum.post(new InviteSessionRejectCommand(getSessionHandle(), code, warning));
}

void
InviteSession::targetRefresh(const NameAddr& localUri)
{
   if (isConnected()) // ?slg? likely not safe in any state except Connected - what should behaviour be if state is ReceivedReinvite?
   {
      mDialog.mLocalContact = localUri;
      sessionRefresh();
   }
   else
   {
      WarningLog (<< "Can't targetRefresh before Connected");
      throw UsageUseException("targetRefresh not allowed in this context", __FILE__, __LINE__);
   }
}

void
InviteSession::refer(const NameAddr& referTo, bool referSub)
{
   refer(referTo,std::auto_ptr<resip::Contents>(0),referSub);
}
void
InviteSession::refer(const NameAddr& referTo, std::auto_ptr<resip::Contents> contents,bool referSub)
{
   if (isConnected()) // ?slg? likely not safe in any state except Connected - what should behaviour be if state is ReceivedReinvite?
   {
      SharedPtr<SipMessage> refer(new SipMessage());
      mDialog.makeRequest(*refer, REFER, mNitState == NitComplete);  // only increment CSeq if not going to queue NIT
      refer->header(h_ReferTo) = referTo;
      refer->header(h_ReferredBy) = myAddr(); 
      refer->header(h_ReferredBy).remove(p_tag);   // tag-param not permitted in rfc3892; not the same as generic-param
      refer->setContents(contents);
      if (!referSub)
      {
         refer->header(h_ReferSub).value() = "false";
         refer->header(h_Supporteds).push_back(Token(Symbols::NoReferSub));
      }

      if(mNitState == NitComplete)
      {
         mNitState = NitProceeding;
         mReferSub = referSub;
         mLastSentNITRequest = refer;
         send(refer);
         return;
      }
      mNITQueue.push(new QueuedNIT(refer,referSub));
      InfoLog(<< "refer - queuing NIT:" << refer->brief());
      return;
   }
   else
   {
      WarningLog (<< "Can't refer before Connected");
      throw UsageUseException("REFER not allowed in this context", __FILE__, __LINE__);
   }
}

const SharedPtr<SipMessage>
InviteSession::getLastSentNITRequest() const
{
   return mLastSentNITRequest;
}

void
InviteSession::nitComplete()
{
   mNitState = NitComplete;
   if (mNITQueue.size())
   {
      QueuedNIT *qn=mNITQueue.front();
      mNITQueue.pop();
      mNitState = NitProceeding;
      mReferSub = qn->referSubscription();
      mLastSentNITRequest = qn->getNIT();
      mDialog.setRequestNextCSeq(*mLastSentNITRequest.get());
      InfoLog(<< "checkNITQueue - sending queued NIT:" << mLastSentNITRequest->brief());
      send(mLastSentNITRequest);
      delete qn;
   }
}

class InviteSessionReferCommand : public DumCommandAdapter
{
public:
   InviteSessionReferCommand(const InviteSessionHandle& inviteSessionHandle, const NameAddr& referTo, bool referSub)
      : mInviteSessionHandle(inviteSessionHandle),
      mReferTo(referTo),
      mReferSub(referSub)
   {

   }

   virtual void executeCommand()
   {
      if(mInviteSessionHandle.isValid())
      {
         mInviteSessionHandle->refer(mReferTo, mReferSub);
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "InviteSessionReferCommand";
   }

private:
   InviteSessionHandle mInviteSessionHandle;
   NameAddr mReferTo;
   bool mReferSub;
};

void
InviteSession::referCommand(const NameAddr& referTo, bool referSub)
{
   mDum.post(new InviteSessionReferCommand(getSessionHandle(), referTo, referSub));
}

void
InviteSession::refer(const NameAddr& referTo, InviteSessionHandle sessionToReplace, bool referSub)
{
   refer(referTo,sessionToReplace,std::auto_ptr<resip::Contents>(0),referSub);
}

void
InviteSession::refer(const NameAddr& referTo, InviteSessionHandle sessionToReplace, std::auto_ptr<resip::Contents> contents, bool referSub)
{
   if (!sessionToReplace.isValid())
   {
      throw UsageUseException("Attempted to make a refer w/ and invalid replacement target", __FILE__, __LINE__);
   }

   CallId replaces;
   DialogId id = sessionToReplace->mDialog.getId();
   replaces.value() = id.getCallId();
   replaces.param(p_toTag) = id.getRemoteTag();
   replaces.param(p_fromTag) = id.getLocalTag();

   refer(referTo, replaces, contents, referSub);
}

void 
InviteSession::refer(const NameAddr& referTo, const CallId& replaces, bool referSub)
{
   refer(referTo,replaces,std::auto_ptr<resip::Contents>(0),referSub);
}

void 
InviteSession::refer(const NameAddr& referTo, const CallId& replaces, std::auto_ptr<resip::Contents> contents, bool referSub)
{
   if (isConnected())  // ?slg? likely not safe in any state except Connected - what should behaviour be if state is ReceivedReinvite?
   {
      SharedPtr<SipMessage> refer(new SipMessage());
      mDialog.makeRequest(*refer, REFER, mNitState == NitComplete);  // only increment CSeq if not going to queue NIT
      refer->setContents(contents);
      refer->header(h_ReferTo) = referTo;
      refer->header(h_ReferredBy) = myAddr();
      refer->header(h_ReferredBy).remove(p_tag);

      refer->header(h_ReferTo).uri().embedded().header(h_Replaces) = replaces;
      
      if (!referSub)
      {
         refer->header(h_ReferSub).value() = "false";
         refer->header(h_Supporteds).push_back(Token(Symbols::NoReferSub));
      }

      if(mNitState == NitComplete)
      {
         mNitState = NitProceeding;
         mReferSub = referSub;
         mLastSentNITRequest = refer;
         send(refer);
         return;
      }
      mNITQueue.push(new QueuedNIT(refer,referSub));
      InfoLog(<< "refer/replace - queuing NIT:" << refer->brief());
      return;
   }
   else
   {
      WarningLog (<< "Can't refer before Connected");
      resip_assert(0);
      throw UsageUseException("REFER not allowed in this context", __FILE__, __LINE__);
   }
}

class InviteSessionReferExCommand : public DumCommandAdapter
{
public:
   InviteSessionReferExCommand(const InviteSessionHandle& inviteSessionHandle, const NameAddr& referTo, InviteSessionHandle sessionToReplace, bool referSub)
      : mInviteSessionHandle(inviteSessionHandle),
      mSessionToReplace(sessionToReplace),
      mReferTo(referTo),
      mReferSub(referSub)
   {
   }

   virtual void executeCommand()
   {
      if(mInviteSessionHandle.isValid())
      {
         mInviteSessionHandle->refer(mReferTo, mSessionToReplace, mReferSub);
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "InviteSessionReferExCommand";
   }

private:
   InviteSessionHandle mInviteSessionHandle;
   InviteSessionHandle mSessionToReplace;
   NameAddr mReferTo;
   bool mReferSub;
};

void
InviteSession::referCommand(const NameAddr& referTo, InviteSessionHandle sessionToReplace, bool referSub)
{
   mDum.post(new InviteSessionReferExCommand(getSessionHandle(), referTo, sessionToReplace, referSub));
}

void
InviteSession::info(const Contents& contents)
{
   SharedPtr<SipMessage> info(new SipMessage());
   mDialog.makeRequest(*info, INFO, mNitState == NitComplete);  // only increment CSeq if not going to queue NIT
   // !jf! handle multipart here
   info->setContents(&contents);
   DumHelper::setOutgoingEncryptionLevel(*info, mCurrentEncryptionLevel);
   if (mNitState == NitComplete)
   {
      mNitState = NitProceeding;
      mLastSentNITRequest = info;
      send(info);
      return;
   }
   mNITQueue.push(new QueuedNIT(info));
   InfoLog(<< "info - queuing NIT:" << info->brief());
   return;
}

class InviteSessionInfoCommand : public DumCommandAdapter
{
public:
   InviteSessionInfoCommand(const InviteSessionHandle& inviteSessionHandle, const Contents& contents)
      : mInviteSessionHandle(inviteSessionHandle),
        mContents(contents.clone())
   {
   }

   virtual void executeCommand()
   {
      if(mInviteSessionHandle.isValid())
      {
         mInviteSessionHandle->info(*mContents);
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "InviteSessionInfoCommand";
   }
private:
   InviteSessionHandle mInviteSessionHandle;
   std::auto_ptr<Contents> mContents;
};

void
InviteSession::infoCommand(const Contents& contents)
{
   mDum.post(new InviteSessionInfoCommand(getSessionHandle(), contents));
}

void
InviteSession::message(const Contents& contents)
{
   SharedPtr<SipMessage> message(new SipMessage());
   mDialog.makeRequest(*message, MESSAGE, mNitState == NitComplete);  // only increment CSeq if not going to queue NIT
   // !jf! handle multipart here
   message->setContents(&contents);
   DumHelper::setOutgoingEncryptionLevel(*message, mCurrentEncryptionLevel);
   InfoLog (<< "Trying to send MESSAGE: " << message);
   if (mNitState == NitComplete)
   {
      mNitState = NitProceeding;
      mLastSentNITRequest = message;
      send(message);
      return;
   }
   mNITQueue.push(new QueuedNIT(message));
   InfoLog(<< "message - queuing NIT:" << message->brief());
   return;
}

class InviteSessionMessageCommand : public DumCommandAdapter
{
public:
   InviteSessionMessageCommand(const InviteSessionHandle& inviteSessionHandle, const Contents& contents)
      : mInviteSessionHandle(inviteSessionHandle),
        mContents(contents.clone())
   {
   }

   virtual void executeCommand()
   {
      if(mInviteSessionHandle.isValid())
      {
         mInviteSessionHandle->message(*mContents);
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "InviteSessionMessageCommand";
   }
private:
   InviteSessionHandle mInviteSessionHandle;
   std::auto_ptr<Contents> mContents;
};


void
InviteSession::messageCommand(const Contents& contents)
{
   mDum.post(new InviteSessionMessageCommand(getSessionHandle(), contents));
}

void
InviteSession::dispatch(const SipMessage& msg)
{
   // Look for 2xx retransmissions - resend ACK and filter out of state machine
   if(msg.header(h_CSeq).method() == INVITE && msg.isResponse() && msg.header(h_StatusLine).statusCode() / 100 == 2)
   {
      AckMap::iterator i = mAcks.find(msg.getTransactionId());
      if (i != mAcks.end())
      {
         send(i->second);  // resend ACK
         return;
      }
   }

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
      case SentReinviteNoOffer:
         dispatchSentReinviteNoOffer(msg);
         break;
      case SentReinviteAnswered:
         dispatchSentReinviteAnswered(msg);
         break;
      case SentUpdateGlare:
      case SentReinviteGlare:
         // The behavior is the same except for timer which is handled in dispatch(Timer)
         dispatchGlare(msg);
         break;
      case SentReinviteNoOfferGlare:
         dispatchReinviteNoOfferGlare(msg);
         break;
      case ReceivedUpdate:
      case ReceivedReinvite:
      case ReceivedReinviteNoOffer:
         dispatchReceivedUpdateOrReinvite(msg);
         break;
      case ReceivedReinviteSentOffer:
         dispatchReceivedReinviteSentOffer(msg);
         break;
      case Answered:
         dispatchAnswered(msg);
         break;
      case WaitingToOffer:
         dispatchWaitingToOffer(msg);
         break;
      case WaitingToRequestOffer:
         dispatchWaitingToRequestOffer(msg);
         break;
      case WaitingToTerminate:
         dispatchWaitingToTerminate(msg);
         break;
      case WaitingToHangup:
         dispatchWaitingToHangup(msg);
         break;
      case Terminated:
         dispatchTerminated(msg);
         break;
      case Undefined:
      default:
         resip_assert(0);
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
         InfoLog (<< "Retransmitting: " << endl << mInvite200->brief());
         //DumHelper::setOutgoingEncryptionLevel(*mInvite200, mCurrentEncryptionLevel);
         send(mInvite200);
         mCurrentRetransmit200 *= 2;
         mDum.addTimerMs(DumTimeout::Retransmit200, resipMin(Timer::T2, mCurrentRetransmit200), getBaseHandle(),  timeout.seq());
      }
   }
   else if (timeout.type() == DumTimeout::WaitForAck)
   {
      if(mCurrentRetransmit200)  // If retransmit200 timer is active then ACK is not received yet
      {
         if (timeout.seq() == mLastRemoteSessionModification->header(h_CSeq).sequence())
         {
            mCurrentRetransmit200 = 0; // stop the 200 retransmit timer

            // If we are waiting for an Ack and it times out, then end with a BYE
            if(mState == UAS_WaitingToHangup || 
               mState == WaitingToHangup)
            {
               SharedPtr<SipMessage> msg = sendBye();
               transition(Terminated);
               mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::LocalBye, msg.get()); 
            }
            else if(mState == ReceivedReinviteSentOffer)
            {
               transition(Connected);
               mProposedLocalOfferAnswer.reset();
               mProposedEncryptionLevel = DialogUsageManager::None;
               //!dcm! -- should this be onIllegalNegotiation?
               mDum.mInviteSessionHandler->onOfferRejected(getSessionHandle(), 0);
            }
            else if(mState == WaitingToOffer || 
                    mState == UAS_WaitingToOffer)
            {
               resip_assert(mProposedLocalOfferAnswer.get());
               mDum.mInviteSessionHandler->onAckNotReceived(getSessionHandle());
               if(!isTerminated())  
               {
                  provideProposedOffer(); 
               }
            }
            else if(mState == WaitingToRequestOffer ||
                    mState == UAS_WaitingToRequestOffer)
            {
               mDum.mInviteSessionHandler->onAckNotReceived(getSessionHandle());
               if(!isTerminated())  
               {
                  requestOffer(); 
               }
            }
            else
            {
               // this is so the app can decided to ignore this. default implementation
               // will call end next
               mDum.mInviteSessionHandler->onAckNotReceived(getSessionHandle());
            }
         }
      }
   }
   else if (timeout.type() == DumTimeout::CanDiscardAck)
   {
      AckMap::iterator i = mAcks.find(timeout.transactionId());
      if (i != mAcks.end())
      {
         mAcks.erase(i);
      }
   }
   else if (timeout.type() == DumTimeout::Glare)
   {
      if (mState == SentUpdateGlare)
      {
         transition(SentUpdate);

         InfoLog (<< "Retransmitting the UPDATE (glare condition timer)");
         mDialog.makeRequest(*mLastLocalSessionModification, UPDATE);  // increments CSeq
         send(mLastLocalSessionModification);
      }
      else if (mState == SentReinviteGlare)
      {
         transition(SentReinvite);

         InfoLog (<< "Retransmitting the reINVITE (glare condition timer)");
         mDialog.makeRequest(*mLastLocalSessionModification, INVITE); // increments CSeq
         startStaleReInviteTimer();
         send(mLastLocalSessionModification);
      }
      else if (mState == SentReinviteNoOfferGlare)
      {
         transition(SentReinviteNoOffer);

         InfoLog (<< "Retransmitting the reINVITE-nooffer (glare condition timer)");
         mDialog.makeRequest(*mLastLocalSessionModification, INVITE);  // increments CSeq
         startStaleReInviteTimer();
         send(mLastLocalSessionModification);
      }
   }
   else if (timeout.type() == DumTimeout::StaleReInvite)
   {
      if(timeout.seq() == mStaleReInviteTimerSeq)
      {
         if(mState == WaitingToTerminate)
         {
            SharedPtr<SipMessage> msg = sendBye();
            transition(Terminated);
            mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::LocalBye, msg.get()); 
         }
         else if(mState == SentReinvite ||
                 mState == SentReinviteNoOffer)
         {
            transition(Connected);
            mProposedLocalOfferAnswer.reset();
            mProposedEncryptionLevel = DialogUsageManager::None;

            // this is so the app can decide to ignore this. default implementation
            // will call end next - which will send a BYE
            mDum.mInviteSessionHandler->onStaleReInviteTimeout(getSessionHandle());
         }
      }
   }
   else if (timeout.type() == DumTimeout::SessionExpiration)
   {
      if(timeout.seq() == mSessionTimerSeq)
      {
         // this is so the app can decide to ignore this. default implementation
         // will call end next - which will send a BYE
         mDum.mInviteSessionHandler->onSessionExpired(getSessionHandle());
      }
   }
   else if (timeout.type() == DumTimeout::SessionRefresh)
   {
     if(timeout.seq() == mSessionTimerSeq)
     {
        // Note: If not connected then we must be issueing a reinvite/update or
        // receiving one - in either case the session timer stuff will get
        // reset/renegotiated - thus just ignore this referesh
        if(mState == Connected)  
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
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   switch (toEvent(msg, offerAnswer.get()))
   {
      case OnInvite:
      case OnInviteReliable:
         *mLastRemoteSessionModification = msg;
         transition(ReceivedReinviteNoOffer);
         handler->onOfferRequired(getSessionHandle(), msg);
         break;

      case OnInviteOffer:
      case OnInviteReliableOffer:
         *mLastRemoteSessionModification = msg;
         transition(ReceivedReinvite);
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         mProposedRemoteOfferAnswer = offerAnswer; 

         handler->onOffer(getSessionHandle(), msg, *mProposedRemoteOfferAnswer);
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
         *mLastRemoteSessionModification = msg;
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         mProposedRemoteOfferAnswer = offerAnswer; 
         handler->onOffer(getSessionHandle(), msg, *mProposedRemoteOfferAnswer);
         break;

      case OnUpdate:
      {
         // ?slg? no offerAnswer in update - just respond immediately (likely session timer) - do we need a callback?
         SharedPtr<SipMessage> response(new SipMessage);
         *mLastRemoteSessionModification = msg;
         mDialog.makeResponse(*response, *mLastRemoteSessionModification, 200);
         handleSessionTimerRequest(*response, *mLastRemoteSessionModification);
         send(response);
         break;
      }

      case OnUpdateRejected:
      case On200Update:
         WarningLog (<< "DUM delivered an UPDATE response in an incorrect state " << endl << msg);
         resip_assert(0);
         break;

      case OnAck:
      case OnAckAnswer: // .bwc. Don't drop ACK with SDP!
         mCurrentRetransmit200 = 0; // stop the 200 retransmit timer
         handler->onAckReceived(getSessionHandle(), msg);
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
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   switch (toEvent(msg, offerAnswer.get()))
   {
      case OnInvite:
      case OnInviteReliable:
      case OnInviteOffer:
      case OnInviteReliableOffer:
      case OnUpdate:
      case OnUpdateOffer:
      {
         // glare
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, msg, 491);
         send(response);
         break;
      }

      case On200Update:
         transition(Connected);
         handleSessionTimerResponse(msg);
         if (offerAnswer.get() && mProposedLocalOfferAnswer.get())
         {
            mCurrentEncryptionLevel = getEncryptionLevel(msg);
            setCurrentLocalOfferAnswer(msg);

            mCurrentRemoteOfferAnswer = offerAnswer; 
            handler->onAnswer(getSessionHandle(), msg, *mCurrentRemoteOfferAnswer);
         }
         else if(mProposedLocalOfferAnswer.get()) 
         {
            // If we sent an offer in the Update Request and no answer is received
            handler->onIllegalNegotiation(getSessionHandle(), msg);
            mProposedLocalOfferAnswer.reset();
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
            // Response must contain Min_SE - if not - just ignore
            // ?slg? callback?
            transition(Connected);
            mProposedLocalOfferAnswer.reset();
            mProposedEncryptionLevel = DialogUsageManager::None;
         }
         break;

      case OnUpdateRejected:
         transition(Connected);
         mProposedLocalOfferAnswer.reset();
         handler->onOfferRejected(getSessionHandle(), &msg);
         break;

      case OnGeneralFailure:
         sendBye();
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
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
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   switch (toEvent(msg, offerAnswer.get()))
   {
      case OnInvite:
      case OnInviteReliable:
      case OnInviteOffer:
      case OnInviteReliableOffer:
      case OnUpdate:
      case OnUpdateOffer:
      {
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, msg, 491);
         send(response);
         break;
      }

      case On1xx:
      case On1xxEarly:
         // Some UA's send a 100 response to a ReInvite - just ignore it
         break;

      case On2xxAnswer:
      case On2xxOffer:  // .slg. doesn't really make sense - should be in SentReinviteNoOffer to get this
      {
         mStaleReInviteTimerSeq++;
         transition(Connected);
         handleSessionTimerResponse(msg);
         setCurrentLocalOfferAnswer(msg);

         // !jf! I need to potentially include an answer in the ACK here
         sendAck();
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         
         if (mSessionRefreshReInvite)
         {
            mSessionRefreshReInvite = false;
         
            if (*mCurrentRemoteOfferAnswer != *offerAnswer)
            {
               mCurrentRemoteOfferAnswer = offerAnswer; 
               handler->onRemoteAnswerChanged(getSessionHandle(), msg, *mCurrentRemoteOfferAnswer);
            }
         }
         else
         {
            mCurrentRemoteOfferAnswer = offerAnswer; 
            handler->onAnswer(getSessionHandle(), msg, *mCurrentRemoteOfferAnswer);
         }
         
         // !jf! do I need to allow a reINVITE overlapping the retransmission of
         // the ACK when a 200I is received? If yes, then I need to store all
         // ACK messages for 64*T1
         break;
      }
      case On2xx:
         mStaleReInviteTimerSeq++;
         sendAck();
         transition(Connected);
         handleSessionTimerResponse(msg);
         handler->onIllegalNegotiation(getSessionHandle(), msg);
         mProposedLocalOfferAnswer.reset();
         mProposedEncryptionLevel = DialogUsageManager::None;
         break;

      case On422Invite:
         mStaleReInviteTimerSeq++;
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
            // ?slg? callback?
            transition(Connected);
            mProposedLocalOfferAnswer.reset();
            mProposedEncryptionLevel = DialogUsageManager::None;
         }
         break;

      case On491Invite:
         mStaleReInviteTimerSeq++;
         transition(SentReinviteGlare);
         start491Timer();
         break;

      case OnGeneralFailure:
         mStaleReInviteTimerSeq++;
         sendBye();
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         break;

      case OnInviteFailure:
      case On487Invite:
         mStaleReInviteTimerSeq++;
         transition(Connected);
         mProposedLocalOfferAnswer.reset();
         handler->onOfferRejected(getSessionHandle(), &msg);
         break;

      default:
         dispatchOthers(msg);
         break;
   }
}

void
InviteSession::dispatchSentReinviteNoOffer(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   switch (toEvent(msg, offerAnswer.get()))
   {
      case OnInvite:
      case OnInviteReliable:
      case OnInviteOffer:
      case OnInviteReliableOffer:
      case OnUpdate:
      case OnUpdateOffer:
      {
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, msg, 491);
         send(response);
         break;
      }

      case On1xx:
      case On1xxEarly:
         // Some UA's send a 100 response to a ReInvite - just ignore it
         break;

      case On2xxAnswer:  // .slg. doesn't really make sense - should be in SentReinvite to get this
      case On2xxOffer:
      {
         mStaleReInviteTimerSeq++;
         transition(SentReinviteAnswered);
         handleSessionTimerResponse(msg);
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         mProposedRemoteOfferAnswer = offerAnswer; 
         handler->onOffer(getSessionHandle(), msg, *mProposedRemoteOfferAnswer);
         break;
      }

      case On2xx:
         mStaleReInviteTimerSeq++;
         sendAck();
         transition(Connected);
         handleSessionTimerResponse(msg);
         handler->onIllegalNegotiation(getSessionHandle(), msg);
         mProposedLocalOfferAnswer.reset();
         mProposedEncryptionLevel = DialogUsageManager::None;
         break;

      case On422Invite:
         mStaleReInviteTimerSeq++;
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
            // ?slg? callback?
            transition(Connected);
            mProposedLocalOfferAnswer.reset();
            mProposedEncryptionLevel = DialogUsageManager::None;
         }
         break;

      case On491Invite:
         mStaleReInviteTimerSeq++;
         transition(SentReinviteNoOfferGlare);
         start491Timer();
         break;

      case OnGeneralFailure:
         mStaleReInviteTimerSeq++;
         sendBye();
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         break;

      case OnInviteFailure:
      case On487Invite:
         mStaleReInviteTimerSeq++;
         transition(Connected);
         mProposedLocalOfferAnswer.reset();
         handler->onOfferRejected(getSessionHandle(), &msg);
         break;

      default:
         dispatchOthers(msg);
         break;
   }
}

void 
InviteSession::dispatchReceivedReinviteSentOffer(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   switch (toEvent(msg, offerAnswer.get()))
   {
      case OnInvite:
      case OnInviteReliable:
      case OnInviteOffer:
      case OnInviteReliableOffer:
      case OnUpdate:
      case OnUpdateOffer:
      {
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, msg, 491);
         send(response);
         break;
      }
      case OnAckAnswer:
         transition(Connected);
         setCurrentLocalOfferAnswer(msg);
         mCurrentRemoteOfferAnswer = offerAnswer; 
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         mCurrentRetransmit200 = 0; // stop the 200 retransmit timer

         handler->onAnswer(getSessionHandle(), msg, *mCurrentRemoteOfferAnswer);
         break;         
      case OnAck:
         if (mLastRemoteSessionModification->header(h_CSeq).sequence() > msg.header(h_CSeq).sequence())
         {
            InfoLog(<< "dropped stale ACK");
         }
         else
         {
            InfoLog(<< "Got Ack with no answer");
            transition(Connected);
            mProposedLocalOfferAnswer.reset();
            mProposedEncryptionLevel = DialogUsageManager::None;
            mCurrentRetransmit200 = 0; // stop the 200 retransmit timer
            //!dcm! -- should this be onIllegalNegotiation?
            handler->onOfferRejected(getSessionHandle(), &msg);
         }
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
   if (msg.isRequest() && (method == INVITE || method == UPDATE))
   {
      DebugLog(<< "Re-INVITE or UPDATE received when in SentReinviteGlare or SentUpdateGlare" << endl);
      // Received inbound reinvite or update, when waiting to resend outbound reinvite or update
      handler->onOfferRejected(getSessionHandle(), &msg);
      if(!isTerminated())   // make sure application didn't call end()
      {
         dispatchConnected(msg);  // act as if we received message in Connected state
      }
      else
      {
         dispatchTerminated(msg);
      }
   }
   else
   {
      dispatchOthers(msg);
   }
}

void
InviteSession::dispatchReinviteNoOfferGlare(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   MethodTypes method = msg.header(h_CSeq).method();
   if (msg.isRequest() && (method == INVITE || method == UPDATE))
   {
      // Received inbound reinvite or update, when waiting to resend outbound reinvite or update
      handler->onOfferRequestRejected(getSessionHandle(), msg);
      if(!isTerminated())   // make sure application didn't call end()
      {
         dispatchConnected(msg);  // act as if we received message in Connected state
      }
      else
      {
         dispatchTerminated(msg);
      }
   }
   else
   {
      dispatchOthers(msg);
   }
}

void
InviteSession::dispatchReceivedUpdateOrReinvite(const SipMessage& msg)
{
   // InviteSessionHandler* handler = mDum.mInviteSessionHandler; // unused
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   switch (toEvent(msg, offerAnswer.get()))
   {
      case OnInvite:
      case OnInviteReliable:
      case OnInviteOffer:
      case OnInviteReliableOffer:
      case OnUpdate:
      case OnUpdateOffer:
      {
         // Means that the UAC has sent us a second reINVITE or UPDATE before we
         // responded to the first one. Bastard!
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, msg, 500);
         response->header(h_RetryAfter).value() = Random::getRandom() % 10;
         send(response);
         break;
      }
      case OnBye:
      {
         // BYE received after a reINVITE, terminate the reINVITE transaction.
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, *mLastRemoteSessionModification, 487); // Request Terminated
         handleSessionTimerRequest(*response, *mLastRemoteSessionModification);
         send(response);

         dispatchBye(msg);
         break;
      }
      default:
         dispatchOthers(msg);
         break;
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
InviteSession::dispatchSentReinviteAnswered(const SipMessage& msg)
{
   if (msg.isResponse() &&
       msg.header(h_CSeq).method() == INVITE &&
       msg.header(h_StatusLine).statusCode() / 200 == 1)
   {
      // Receving a 200 retransmission is possible - but we don't have an ACK response yet - we are still waiting for provideAnswer to be
      // called by the app - so just drop the retransmission
      return;
   }
   dispatchOthers(msg);
}

void
InviteSession::dispatchWaitingToOffer(const SipMessage& msg)
{
   if (msg.isRequest() && msg.header(h_RequestLine).method() == ACK)
   {
      resip_assert(mProposedLocalOfferAnswer.get());
      mCurrentRetransmit200 = 0; // stop the 200 retransmit timer
      provideProposedOffer(); 
   }
   else
   {
      dispatchOthers(msg);
   }
}

void
InviteSession::dispatchWaitingToRequestOffer(const SipMessage& msg)
{
   if (msg.isRequest() && msg.header(h_RequestLine).method() == ACK)
   {
      mCurrentRetransmit200 = 0; // stop the 200 retransmit timer
      requestOffer(); 
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
      SharedPtr<SipMessage> msg = sendBye();
      transition(Terminated);
      mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::LocalBye, msg.get()); 
   }
   else if(msg.isRequest())
   {
      if(msg.method() == BYE)
      {
         dispatchBye(msg);
      }
      else
      {
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, msg, 400 /* Bad Request */);
         send(response);
      }
   }
}

void
InviteSession::dispatchWaitingToHangup(const SipMessage& msg)
{
   std::auto_ptr<Contents> offerAnswer = InviteSession::getOfferAnswer(msg);

   switch (toEvent(msg, offerAnswer.get()))
   {
      case OnAck:
      case OnAckAnswer:
      {
         mCurrentRetransmit200 = 0; // stop the 200 retransmit timer

         SharedPtr<SipMessage> msg = sendBye();
         transition(Terminated);
         mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::LocalBye, msg.get());
         break;
      }
      
      default:
         break;
   }
}

void
InviteSession::dispatchTerminated(const SipMessage& msg)
{
   InfoLog (<< "InviteSession::dispatchTerminated " << msg.brief());

   if (msg.isRequest())
   {
      if (BYE == msg.header(h_CSeq).method())
      {
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, msg, 200);
         send(response);
      }
      else
      {
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, msg, 481);
         send(response);
      }

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
      case MESSAGE:
         dispatchMessage(msg);
         break;
	  case ACK:
		  // Ignore duplicate ACKs from 2xx reTransmissions
		  break;
      default:
         // handled in Dialog
         WarningLog (<< "DUM delivered a "
                     << msg.header(h_CSeq).unknownMethodName()
                     << " to the InviteSession in state: " << toData(mState)
                     << endl
                     << msg);
         resip_assert(0);
         break;
   }
}

void
InviteSession::dispatchUnhandledInvite(const SipMessage& msg)
{
   resip_assert(msg.isRequest());
   resip_assert(msg.header(h_CSeq).method() == INVITE);

   // If we get an INVITE request from the wire and we are not in
   // Connected state, reject the request and send a BYE
   SharedPtr<SipMessage> response(new SipMessage);
   mDialog.makeResponse(*response, msg, 400); // !jf! what code to use?
   InfoLog (<< "Sending " << response->brief());
   send(response);

   sendBye();
   transition(Terminated);
   mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg); 
}

void
InviteSession::dispatchPrack(const SipMessage& msg)
{
   resip_assert(msg.header(h_CSeq).method() == PRACK);
   if(msg.isRequest())
   {
      SharedPtr<SipMessage> rsp(new SipMessage);
      mDialog.makeResponse(*rsp, msg, 481);
      send(rsp);

      sendBye();
      // !jf! should we make some other callback here
      transition(Terminated);
      mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
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
   resip_assert(msg.header(h_CSeq).method() == CANCEL);
   if(msg.isRequest())
   {
      SharedPtr<SipMessage> rsp(new SipMessage);
      mDialog.makeResponse(*rsp, msg, 200);
      send(rsp);

      sendBye();
      // !jf! should we make some other callback here
      transition(Terminated);

      handler->onTerminated(getSessionHandle(), InviteSessionHandler::RemoteCancel, &msg);
   }
   else
   {
      WarningLog (<< "DUM let me send a CANCEL at an incorrect state " << endl << msg);
      resip_assert(0);
   }
}

void
InviteSession::dispatchBye(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;

   if (msg.isRequest())
   {
      // Check for any non-invite server transactions (e.g. INFO)
      // that have not been responded yet and terminate them.
      if (mServerNitState == NitProceeding)
      {
         mLastNitResponse->header(h_StatusLine).statusCode() = 487;  
         mLastNitResponse->setContents(0);
         Helper::getResponseCodeReason(487, mLastNitResponse->header(h_StatusLine).reason());
         send(mLastNitResponse);
         mServerNitState = NitComplete;
      }

      SharedPtr<SipMessage> rsp(new SipMessage);
      InfoLog (<< "Received " << msg.brief());
      mDialog.makeResponse(*rsp, msg, 200);
      send(rsp);

      // !jf! should we make some other callback here
      transition(Terminated);

      if (mDum.mDialogEventStateManager)
      {
         mDum.mDialogEventStateManager->onTerminated(mDialog, msg, InviteSessionHandler::RemoteBye);
      }

      handler->onTerminated(getSessionHandle(), InviteSessionHandler::RemoteBye, &msg);
      mDum.destroy(this);
   }
   else
   {
      WarningLog (<< "DUM let me send a BYE at an incorrect state " << endl << msg);
      resip_assert(0);
   }
}

void
InviteSession::dispatchInfo(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   if (msg.isRequest())
   {
      if (mServerNitState == NitProceeding)
      {
         // Means that the UAC has sent us a second INFO before we
         // responded to the first one.
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, msg, 500);
         response->header(h_RetryAfter).value() = Random::getRandom() % 10;
         send(response);
         WarningLog(<<"an INFO message was received before the application called acceptNIT() for the previous INFO message");
      }
      else
      {
         InfoLog (<< "Received " << msg.brief());
         mServerNitState = NitProceeding;
         mDialog.makeResponse(*mLastNitResponse, msg, 200);
         handler->onInfo(getSessionHandle(), msg);
      }
   }
   else
   {
      resip_assert(mNitState == NitProceeding);
      //!dcm! -- toss away 1xx to an info?
      if (msg.header(h_StatusLine).statusCode() >= 300)
      {
         handler->onInfoFailure(getSessionHandle(), msg);
      }
      else if (msg.header(h_StatusLine).statusCode() >= 200)
      {
         handler->onInfoSuccess(getSessionHandle(), msg);
      }
      nitComplete();
   }
}

void
InviteSession::acceptNIT(int statusCode, const Contents * contents)
{
   if (statusCode / 100  != 2)
   {
      throw UsageUseException("Must accept with a 2xx", __FILE__, __LINE__);
   }

   if (mServerNitState != NitProceeding )
   {
      throw UsageUseException("No transaction to accept", __FILE__, __LINE__);
   }

   mLastNitResponse->header(h_StatusLine).statusCode() = statusCode;   
   mLastNitResponse->setContents(contents);
   Helper::getResponseCodeReason(statusCode, mLastNitResponse->header(h_StatusLine).reason());
   send(mLastNitResponse);   
   mServerNitState = NitComplete;
} 

class InviteSessionAcceptNITCommand : public DumCommandAdapter
{
public:
   InviteSessionAcceptNITCommand(const InviteSessionHandle& inviteSessionHandle, int statusCode, const Contents* contents)
      : mInviteSessionHandle(inviteSessionHandle),
        mStatusCode(statusCode),
        mContents(contents?contents->clone():0)
   {

   }

   virtual void executeCommand()
   {
      if(mInviteSessionHandle.isValid())
      {
         mInviteSessionHandle->acceptNIT(mStatusCode, mContents.get());
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "InviteSessionAcceptNITCommand";
   }
private:
   InviteSessionHandle mInviteSessionHandle;
   int mStatusCode;
   std::auto_ptr<Contents> mContents;
};

void
InviteSession::acceptNITCommand(int statusCode, const Contents* contents)
{
   mDum.post(new InviteSessionAcceptNITCommand(getSessionHandle(), statusCode, contents));
} 

void
InviteSession::rejectNIT(int statusCode)
{
   if (statusCode < 400)
   {
      throw UsageUseException("Must reject with a >= 4xx", __FILE__, __LINE__);
   }

   if (mServerNitState != NitProceeding )
   {
      throw UsageUseException("No transaction to reject", __FILE__, __LINE__);
   }

   mLastNitResponse->header(h_StatusLine).statusCode() = statusCode;  
   mLastNitResponse->setContents(0);
   Helper::getResponseCodeReason(statusCode, mLastNitResponse->header(h_StatusLine).reason());
   send(mLastNitResponse);
   mServerNitState = NitComplete;
}

class InviteSessionRejectNITCommand : public DumCommandAdapter
{
public:
   InviteSessionRejectNITCommand(const InviteSessionHandle& inviteSessionHandle, int statusCode)
      : mInviteSessionHandle(inviteSessionHandle),
      mStatusCode(statusCode)
   {
   }

   virtual void executeCommand()
   {
      if(mInviteSessionHandle.isValid())
      {
         mInviteSessionHandle->rejectNIT(mStatusCode);
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "InviteSessionRejectNITCommand";
   }
private:
   InviteSessionHandle mInviteSessionHandle;
   int mStatusCode;
};

void
InviteSession::rejectNITCommand(int statusCode)
{
   mDum.post(new InviteSessionRejectNITCommand(getSessionHandle(), statusCode));
}

void
InviteSession::dispatchMessage(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   if (msg.isRequest())
   {
      if (mServerNitState == NitProceeding)
      {
         // Means that the UAC has sent us a second NIT message before we
         // responded to the first one.
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, msg, 500);
         response->header(h_RetryAfter).value() = Random::getRandom() % 10;
         send(response);
      }
      else
      {
         InfoLog (<< "Received " << msg.brief());
         mServerNitState = NitProceeding;
         mDialog.makeResponse(*mLastNitResponse, msg, 200);
         mLastNitResponse->header(h_Contacts).clear();
         handler->onMessage(getSessionHandle(), msg);
      }
   }
   else
   {
      resip_assert(mNitState == NitProceeding);
      //!dcm! -- toss away 1xx to an message?
      if (msg.header(h_StatusLine).statusCode() >= 300)
      {
         handler->onMessageFailure(getSessionHandle(), msg);
      }
      else if (msg.header(h_StatusLine).statusCode() >= 200)
      {
         handler->onMessageSuccess(getSessionHandle(), msg);
      }
      nitComplete();
   }
}

void
InviteSession::startRetransmit200Timer()
{
   mCurrentRetransmit200 = Timer::T1;
   unsigned int seq = mLastRemoteSessionModification->header(h_CSeq).sequence();
   mDum.addTimerMs(DumTimeout::Retransmit200, mCurrentRetransmit200, getBaseHandle(), seq);
   mDum.addTimerMs(DumTimeout::WaitForAck, Timer::TH, getBaseHandle(), seq);
}

// RFC3261 section 14.1
// If a UAC receives a 491 response to a re-INVITE, it SHOULD start a timer with
// a value T chosen as follows:
//  1. If the UAC is the owner of the Call-ID of the dialog ID, T has a randomly
//  chosen value between 2.1 and 4 seconds in units of 10 ms.
//  2. If the UAC is not the owner of the Call-ID of the dialog ID, T has a
//  randomly chosen value of between 0 and 2 seconds in units of 10 ms.
void
InviteSession::start491Timer()
{
   unsigned int seq = mLastLocalSessionModification->header(h_CSeq).sequence();

   if (dynamic_cast<ClientInviteSession*>(this))
   {
      int timer = Random::getRandom() % (4000 - 2100);
      timer += 2100;
      timer -= timer % 10;
      
      DebugLog(<< "491 timer value: " << timer << "ms" << endl);
      mDum.addTimerMs(DumTimeout::Glare, timer, getBaseHandle(), seq);
   }
   else
   {
      int timer = Random::getRandom() % 2000;
      timer -= timer % 10;
      DebugLog(<< "491 timer value: " << timer << "ms" << endl);
      mDum.addTimerMs(DumTimeout::Glare, timer, getBaseHandle(), seq);
   }
}

void 
InviteSession::startStaleReInviteTimer()
{
   InfoLog (<< toData(mState) << ": startStaleReInviteTimer");
   unsigned long when = mDialog.mDialogSet.getUserProfile()->getDefaultStaleReInviteTime();
   
   mDum.addTimer(DumTimeout::StaleReInvite, 
                 when, 
                 getBaseHandle(), 
                 ++mStaleReInviteTimerSeq);
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
      if(msg.isRequest() || 
         (msg.isResponse() && msg.header(h_StatusLine).responseCode() == 422))
      {
         msg.header(h_MinSE).value() = mMinSE;
      }
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
      mDialog.makeRequest(*mLastLocalSessionModification, UPDATE);
      mLastLocalSessionModification->setContents(0);  // Don't send SDP
   }
   else
   {
      transition(SentReinvite);
      mDialog.makeRequest(*mLastLocalSessionModification, INVITE);
      startStaleReInviteTimer();
      InviteSession::setOfferAnswer(*mLastLocalSessionModification, mCurrentLocalOfferAnswer.get());
      mProposedLocalOfferAnswer = InviteSession::makeOfferAnswer(*mCurrentLocalOfferAnswer.get(), 0);
      mSessionRefreshReInvite = true;      
   }
   setSessionTimerHeaders(*mLastLocalSessionModification);

   InfoLog (<< "sessionRefresh: Sending " << mLastLocalSessionModification->brief());
   DumHelper::setOutgoingEncryptionLevel(*mLastLocalSessionModification, mCurrentEncryptionLevel);
   send(mLastLocalSessionModification);
}

void
InviteSession::setSessionTimerPreferences()
{
   mSessionInterval = mDialog.mDialogSet.getUserProfile()->getDefaultSessionTime();  // Used only if remote doesn't request a time
   if(mSessionInterval != 0)
   {
       // If session timers are not disabled then ensure interval is greater than or equal to MinSE
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
   case Profile::PreferCalleeRefreshes:  
      mSessionRefresher = dynamic_cast<ServerInviteSession*>(this) != NULL; // Default refresher is callee
      break;
   case Profile::PreferCallerRefreshes:
      mSessionRefresher = dynamic_cast<ClientInviteSession*>(this) != NULL; // Default refresher is caller
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
         mDum.addTimer(DumTimeout::SessionExpiration, mSessionInterval - resipMin((UInt32)32,mSessionInterval/3), getBaseHandle(), ++mSessionTimerSeq);
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
   resip_assert(msg.header(h_CSeq).method() == INVITE || msg.header(h_CSeq).method() == UPDATE);

   // Allow Re-Invites and Updates to update the Peer P-Asserted-Identity
   if (msg.exists(h_PAssertedIdentities))
   {
       mPeerPAssertedIdentities = msg.header(h_PAssertedIdentities);
   }

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
   resip_assert(request.header(h_CSeq).method() == INVITE || request.header(h_CSeq).method() == UPDATE);

   // Allow Re-Invites and Updates to update the Peer P-Asserted-Identity
   if (request.exists(h_PAssertedIdentities))
   {
       mPeerPAssertedIdentities = request.header(h_PAssertedIdentities);
   }

   // If session timers are locally supported then add necessary headers to response
   if(mDum.getMasterProfile()->getSupportedOptionTags().find(Token(Symbols::Timer)))
   {
      // Update MinSE if specified and longer than current value
      if(request.exists(h_MinSE))
      {
         mMinSE = resipMax(mMinSE, request.header(h_MinSE).value());
      }
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
      case SentReinviteNoOffer:
         return "InviteSession::SentReinviteNoOffer";
      case SentReinviteAnswered:
         return "InviteSession::SentReinviteAnswered";
      case SentReinviteNoOfferGlare:
         return "InviteSession::SentReinviteNoOfferGlare";
      case ReceivedUpdate:
         return "InviteSession::ReceivedUpdate";
      case ReceivedReinvite:
         return "InviteSession::ReceivedReinvite";
      case ReceivedReinviteNoOffer:
         return "InviteSession::ReceivedReinviteNoOffer";
	  case ReceivedReinviteSentOffer:
		  return "InviteSession::ReceivedReinviteSentOffer";
      case Answered:
         return "InviteSession::Answered";
      case WaitingToOffer:
         return "InviteSession::WaitingToOffer";
      case WaitingToRequestOffer:
         return "InviteSession::WaitingToRequestOffer";
      case WaitingToTerminate:
         return "InviteSession::WaitingToTerminate";
      case WaitingToHangup:
         return "InviteSession::WaitingToHangup";
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
      case UAC_SentUpdateEarlyGlare:
         return "UAC_SentUpdateEarlyGlare";
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
      case UAS_OfferReliableProvidedAnswer:
         return "UAS_OfferReliableProvidedAnswer";
      case UAS_NoOfferReliable:
         return "UAS_NoOfferReliable";
      case UAS_ProvidedOfferReliable:
         return "UAS_ProvidedOfferReliable";
      case UAS_FirstSentOfferReliable:
         return "UAS_FirstSentOfferReliable";
      case UAS_FirstSentAnswerReliable:
         return "UAS_FirstSentAnswerReliable";
      case UAS_NoAnswerReliableWaitingPrack:
         return "UAS_NoAnswerReliableWaitingPrack";
      case UAS_NoAnswerReliable:
         return "UAS_NoAnswerReliable";
      case UAS_NegotiatedReliable:
         return "UAS_NegotiatedReliable";
      case UAS_SentUpdate:
         return "UAS_SentUpdate";
      case UAS_SentUpdateAccepted:
         return "UAS_SentUpdateAccepted";
      case UAS_SentUpdateGlare:
         return "UAS_SentUpdateGlare";
      case UAS_ReceivedUpdate:
         return "UAS_ReceivedUpdate";
      case UAS_ReceivedUpdateWaitingAnswer:
         return "UAS_ReceivedUpdateWaitingAnswer";
      case UAS_WaitingToHangup:
         return "UAS_WaitingToHangup";
      case UAS_WaitingToRequestOffer:
         return "UAS_WaitingToRequestOffer";
   }
   resip_assert(0);
   return "Undefined";
}


void
InviteSession::transition(State target)
{
   InfoLog (<< "Transition " << toData(mState) << " -> " << toData(target));
   mState = target;
}

bool
InviteSession::isReliable(const SipMessage& msg) const
{
   if(msg.method() != INVITE)
   {
      return false;
   }
   if(msg.isRequest())
   {
      return mDum.getMasterProfile()->getUasReliableProvisionalMode() > MasterProfile::Never &&
             ((msg.exists(h_Supporteds) && msg.header(h_Supporteds).find(Token(Symbols::C100rel))) || 
              (msg.exists(h_Requires)   && msg.header(h_Requires).find(Token(Symbols::C100rel))));
   }
   else
   {
      // RFC3262 says reliable provisionals MUST have a Require: 100rel and an RSeq
      return mDum.getMasterProfile()->getUacReliableProvisionalMode() > MasterProfile::Never &&
             msg.exists(h_Requires) && msg.header(h_Requires).find(Token(Symbols::C100rel)) && 
             msg.exists(h_RSeq);
   }
}

//static std::auto_ptr<SdpContents> emptySdp;
std::auto_ptr<Contents>
InviteSession::getOfferAnswer(const SipMessage& msg)
{
	if(mDum.mInviteSessionHandler->isGenericOfferAnswer())   
   {
      if(msg.getContents())
      {
         return std::auto_ptr<Contents>(msg.getContents()->clone());
      }
      else
      {
         return std::auto_ptr<Contents>();
      }
   }
   else
   {
      return std::auto_ptr<Contents>(Helper::getSdp(msg.getContents()));
   }
}

std::auto_ptr<Contents>
InviteSession::makeOfferAnswer(const Contents& offerAnswer)
{
   return std::auto_ptr<Contents>(static_cast<Contents*>(offerAnswer.clone()));
}

auto_ptr<Contents>
InviteSession::makeOfferAnswer(const Contents& offerAnswer,
                               const Contents* alternative)
{
   if (alternative)
   {
      MultipartAlternativeContents* mac = new MultipartAlternativeContents;
      mac->parts().push_back(alternative->clone());
      mac->parts().push_back(offerAnswer.clone());
      return auto_ptr<Contents>(mac);
   }
   else
   {
      return auto_ptr<Contents>(offerAnswer.clone());
   }
}

void
InviteSession::setOfferAnswer(SipMessage& msg, const Contents& offerAnswer, const Contents* alternative)
{
   // !jf! should deal with multipart here

   // This will clone the offerAnswer since the InviteSession also wants to keep its own
   // copy of the offerAnswer around for the application to access
   if (alternative)
   {
      MultipartAlternativeContents* mac = new MultipartAlternativeContents;
      mac->parts().push_back(alternative->clone());
      mac->parts().push_back(offerAnswer.clone());
      msg.setContents(auto_ptr<Contents>(mac));
   }
   else
   {
      msg.setContents(&offerAnswer);
   }
}

void
InviteSession::setOfferAnswer(SipMessage& msg, const Contents* offerAnswer)
{
   resip_assert(offerAnswer);
   msg.setContents(offerAnswer);
}

void 
InviteSession::provideProposedOffer()
{
   MultipartAlternativeContents* mp_ans =
     dynamic_cast<MultipartAlternativeContents*>(mProposedLocalOfferAnswer.get());
   if (mp_ans)
   {
      // .kw. can cast below ever be NULL? Need assert/throw?
      provideOffer( *(dynamic_cast<Contents*>((mp_ans)->parts().back())),
                    mProposedEncryptionLevel,
                    dynamic_cast<Contents*>((mp_ans)->parts().front()));
   }
   else
   {
      // .kw. can cast below ever be NULL? Need assert/throw?
      provideOffer(*(dynamic_cast<Contents*>(mProposedLocalOfferAnswer.get())), mProposedEncryptionLevel, 0);
   }
}

InviteSession::Event
InviteSession::toEvent(const SipMessage& msg, const Contents* offerAnswer)
{
   MethodTypes method = msg.header(h_CSeq).method();
   int code = msg.isResponse() ? msg.header(h_StatusLine).statusCode() : 0;
   
   //.dcm. Treat an invite as reliable if UAS 100rel support is enabled. For
   //responses, reiable provisionals should only be received if the invite was
   //sent reliably.  Spurious reliable provisional respnoses are dropped outside
   //the state machine.
   bool reliable = isReliable(msg);
   bool sentOffer = mProposedLocalOfferAnswer.get();

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
      if (offerAnswer)
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
         if (offerAnswer)
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
         if (offerAnswer)
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
      if (offerAnswer)
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
      if (offerAnswer)
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
      if (offerAnswer)
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
      //assert(0);   // dispatchOthers will throw if the message type is really unknown
      return Unknown;
   }
}

void 
InviteSession::sendAck(const Contents *answer)
{
   SharedPtr<SipMessage> ack(new SipMessage);

   SharedPtr<SipMessage> source;
   
   if (mLastLocalSessionModification->method() == UPDATE)
   {
      //.dcm. scary--we could make a special ClientInviteSession variable/sendAck
      source = mDialog.mDialogSet.getCreator()->getLastRequest();
   }
   else
   {
      source = mLastLocalSessionModification;
   }

   resip_assert(mAcks.count(source->getTransactionId()) == 0);

   mDialog.makeRequest(*ack, ACK);

   // Copy Authorization and Proxy Authorization headers from 
   // mLastLocalSessionModification; regardless of whether this was the original 
   // INVITE or not, this is the correct place to go for auth headers.
   if(mLastLocalSessionModification->exists(h_Authorizations))
   {
      ack->header(h_Authorizations) = mLastLocalSessionModification->header(h_Authorizations);
   }
   if(mLastLocalSessionModification->exists(h_ProxyAuthorizations))
   {
      ack->header(h_ProxyAuthorizations) = mLastLocalSessionModification->header(h_ProxyAuthorizations);
   }

   // Copy CSeq from original INVITE
   ack->header(h_CSeq).sequence() = source->header(h_CSeq).sequence();

   if(answer != 0)
   {
      setOfferAnswer(*ack, *answer);
   }
   mAcks[source->getTransactionId()] = ack;
   mDum.addTimerMs(DumTimeout::CanDiscardAck, Timer::TH, getBaseHandle(), ack->header(h_CSeq).sequence(), 0, source->getTransactionId());

   InfoLog (<< "Sending " << ack->brief());
   send(ack);
}

SharedPtr<SipMessage>
InviteSession::sendBye()
{
   SharedPtr<SipMessage> bye(new SipMessage());
   mDialog.makeRequest(*bye, BYE);
   Data txt;
   if (mEndReason != NotSpecified)
   {
      Token reason("SIP");
      txt = getEndReasonString(mEndReason);
      reason.param(p_text) = txt;
      bye->header(h_Reasons).push_back(reason);      
   }

   if (mDum.mDialogEventStateManager)
   {
      mDum.mDialogEventStateManager->onTerminated(mDialog, *bye, InviteSessionHandler::LocalBye);
   }
   
   InfoLog (<< myAddr() << " Sending BYE " << txt);
   send(bye);
   return bye;
}

DialogUsageManager::EncryptionLevel 
InviteSession::getEncryptionLevel(const SipMessage& msg)
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

void 
InviteSession::setCurrentLocalOfferAnswer(const SipMessage& msg)
{
   resip_assert(mProposedLocalOfferAnswer.get());
   if (dynamic_cast<MultipartAlternativeContents*>(mProposedLocalOfferAnswer.get()))
   {
      if (DialogUsageManager::Encrypt == getEncryptionLevel(msg) || DialogUsageManager::SignAndEncrypt == getEncryptionLevel(msg))
      {
         mCurrentLocalOfferAnswer = auto_ptr<Contents>(static_cast<Contents*>((dynamic_cast<MultipartAlternativeContents*>(mProposedLocalOfferAnswer.get()))->parts().back()->clone()));
      }
      else
      {
         mCurrentLocalOfferAnswer = auto_ptr<Contents>(static_cast<Contents*>((dynamic_cast<MultipartAlternativeContents*>(mProposedLocalOfferAnswer.get()))->parts().front()->clone()));
      }
   }
   else
   {
      mCurrentLocalOfferAnswer = auto_ptr<Contents>(static_cast<Contents*>(mProposedLocalOfferAnswer.get()->clone()));
   }
   mProposedLocalOfferAnswer.reset();   
}

void 
InviteSession::onReadyToSend(SipMessage& msg)
{
   mDum.mInviteSessionHandler->onReadyToSend(getSessionHandle(), msg);
}

void
InviteSession::flowTerminated()
{
   // notify handler
   mDum.mInviteSessionHandler->onFlowTerminated(getSessionHandle());
}

void 
InviteSession::referNoSub(const SipMessage& msg)
{
   resip_assert(msg.isRequest() && msg.header(h_CSeq).method()==REFER);
   mLastReferNoSubRequest = msg;
   mDum.mInviteSessionHandler->onReferNoSub(getSessionHandle(), mLastReferNoSubRequest);
}

void
InviteSession::acceptReferNoSub(int statusCode)
{
   if (statusCode / 100  != 2)
   {
      throw UsageUseException("Must accept with a 2xx", __FILE__, __LINE__);
   }

   SharedPtr<SipMessage> response(new SipMessage);
   mDialog.makeResponse(*response, mLastReferNoSubRequest, statusCode);
   response->header(h_ReferSub).value() = "false";
   //response->header(h_Supporteds).push_back(Token(Symbols::NoReferSub));
   
   send(response);
} 

void
InviteSession::rejectReferNoSub(int responseCode)
{
   if (responseCode < 400)
   {
      throw UsageUseException("Must reject with a >= 4xx", __FILE__, __LINE__);
   }

   SharedPtr<SipMessage> response(new SipMessage);
   mDialog.makeResponse(*response, mLastReferNoSubRequest, responseCode);
   send(response);
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



