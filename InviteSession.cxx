#include "resip/stack/MultipartMixedContents.hxx"
#include "resip/stack/MultipartAlternativeContents.hxx"
#include "resip/stack/SdpContents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/dum/Dialog.hxx"
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
#include "rutil/MD5Stream.hxx"
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
   "Session Timer Expired"
};

const Data& getEndReasonString(InviteSession::EndReason reason)
{
   assert(reason >= InviteSession::NotSpecified && reason < InviteSession::ENDREASON_MAX); //!dcm! -- necessary?
   return EndReasons[reason];
}

InviteSession::InviteSession(DialogUsageManager& dum, Dialog& dialog)
   : DialogUsage(dum, dialog),
     mState(Undefined),
     mNitState(NitComplete),
     mLastLocalSessionModification(new SipMessage),
     mLastRemoteSessionModification(new SipMessage),
     mInvite200(new SipMessage),
     mLastNitResponse(new SipMessage),
     mCurrentRetransmit200(0),
     mSessionInterval(0),
     mMinSE(90), 
     mSessionRefresher(false),
     mSessionTimerSeq(0),
     mSessionRefreshReInvite(false),
     mSentRefer(false),
     mReferSub(true),
     mCurrentEncryptionLevel(DialogUsageManager::None),
     mProposedEncryptionLevel(DialogUsageManager::None),
     mEndReason(NotSpecified)
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
   if(mCurrentLocalSdp.get())
   {
      return *mCurrentLocalSdp;
   }
   else
   {
      return SdpContents::Empty;
   }
}

const SdpContents&
InviteSession::getRemoteSdp() const
{
   if(mCurrentRemoteSdp.get())
   {
      return *mCurrentRemoteSdp;
   }
   else
   {
      return SdpContents::Empty;
   }
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
      case UAS_NoOffer:
      case UAS_NoOfferReliable:
      case UAS_ProvidedOffer:
      case UAS_OfferProvidedAnswer:
      case UAS_EarlyOffer:
      case UAS_EarlyProvidedOffer:
      case UAS_EarlyProvidedAnswer:
      case UAS_EarlyNoOffer:
      case UAS_FirstSentAnswerReliable:
      case UAS_FirstSentOfferReliable:
      case UAS_NegotiatedReliable:
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
InviteSession::requestOffer()
{
   switch (mState)
   {
      case Connected:
      case WaitingToRequestOffer:
      case UAS_WaitingToRequestOffer:
         transition(SentReinviteNoOffer);
         mDialog.makeRequest(*mLastLocalSessionModification, INVITE);
         mLastLocalSessionModification->setContents(0);		// Clear the SDP contents from the INVITE
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
            mDialog.makeRequest(*mLastLocalSessionModification, UPDATE);
         }
         else
         {
            transition(SentReinvite);
            mDialog.makeRequest(*mLastLocalSessionModification, INVITE);
         }
         setSessionTimerHeaders(*mLastLocalSessionModification);

         InfoLog (<< "Sending " << mLastLocalSessionModification->brief());
         InviteSession::setSdp(*mLastLocalSessionModification, offer, alternative);
         mProposedLocalSdp = InviteSession::makeSdp(offer, alternative);
         mProposedEncryptionLevel = level;
         DumHelper::setOutgoingEncryptionLevel(*mLastLocalSessionModification, mProposedEncryptionLevel);

         // call send to give app an chance to adorn the message.
         send(mLastLocalSessionModification);
         break;

      case Answered:
         // queue the offer to be sent after the ACK is received
         transition(WaitingToOffer);
         mProposedEncryptionLevel = level;
         mProposedLocalSdp = InviteSession::makeSdp(offer, alternative);
         break;

      case ReceivedReinviteNoOffer:
         assert(!mProposedRemoteSdp.get());
         transition(ReceivedReinviteSentOffer);
         mDialog.makeResponse(*mInvite200, *mLastRemoteSessionModification, 200);
         handleSessionTimerRequest(*mInvite200, *mLastRemoteSessionModification);
         InviteSession::setSdp(*mInvite200, offer, 0);
         mProposedLocalSdp  = InviteSession::makeSdp(offer);

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
   InviteSessionProvideOfferExCommand(InviteSession& inviteSession, 
      const SdpContents& offer, 
      DialogUsageManager::EncryptionLevel level, 
      const SdpContents* alternative)
      : mInviteSession(inviteSession),
      mOffer(offer)
   {
   }

   virtual void executeCommand()
   {
      mInviteSession.provideOffer(mOffer, mLevel, mAlternative.get());
   }

   virtual std::ostream& encodeBrief(std::ostream& strm) const
   {
      return strm << "InviteSessionProvideOfferExCommand";
   }
private:
   InviteSession& mInviteSession;
   SdpContents mOffer;
   DialogUsageManager::EncryptionLevel mLevel;
   std::auto_ptr<const SdpContents> mAlternative;
};

void
InviteSession::provideOfferCommand(const SdpContents& offer, DialogUsageManager::EncryptionLevel level, const SdpContents* alternative)
{
   mDum.post(new InviteSessionProvideOfferExCommand(*this, offer, level, alternative));
}

void
InviteSession::provideOffer(const SdpContents& offer)
{
   return provideOffer(offer, mCurrentEncryptionLevel, 0);
}

class InviteSessionProvideOfferCommand : public DumCommandAdapter
{
public:
   InviteSessionProvideOfferCommand(InviteSession& inviteSession, const SdpContents& offer)
      : mInviteSession(inviteSession),
      mOffer(offer)
   {
   }

   virtual void executeCommand()
   {
      mInviteSession.provideOffer(mOffer);
   }

   virtual std::ostream& encodeBrief(std::ostream& strm) const
   {
      return strm << "InviteSessionProvideOfferCommand";
   }
private:
   InviteSession& mInviteSession;
   SdpContents mOffer;
};

void
InviteSession::provideOfferCommand(const SdpContents& offer)
{
   mDum.post(new InviteSessionProvideOfferCommand(*this, offer));
}

void
InviteSession::provideAnswer(const SdpContents& answer)
{
   switch (mState)
   {
      case ReceivedReinvite:
         transition(Connected);
         mDialog.makeResponse(*mInvite200, *mLastRemoteSessionModification, 200);
         handleSessionTimerRequest(*mInvite200, *mLastRemoteSessionModification);
         InviteSession::setSdp(*mInvite200, answer, 0);
         mCurrentLocalSdp = InviteSession::makeSdp(answer);
         mCurrentRemoteSdp = mProposedRemoteSdp;
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
         InviteSession::setSdp(*response, answer, 0);
         mCurrentLocalSdp = InviteSession::makeSdp(answer);
         mCurrentRemoteSdp = mProposedRemoteSdp;
         InfoLog (<< "Sending " << response->brief());
         DumHelper::setOutgoingEncryptionLevel(*response, mCurrentEncryptionLevel);
         send(response);
         break;
      }

      case SentReinviteAnswered:
         transition(Connected);
         sendAck(&answer);

         mCurrentRemoteSdp = mProposedRemoteSdp;
         mCurrentLocalSdp = InviteSession::makeSdp(answer);
         break;

      default:
         WarningLog (<< "Incorrect state to provideAnswer: " << toData(mState));
         throw DialogUsage::Exception("Can't provide an answer", __FILE__,__LINE__);
   }
}

class InviteSessionProvideAnswerCommand : public DumCommandAdapter
{
public:
   InviteSessionProvideAnswerCommand(InviteSession& inviteSession, const SdpContents& answer)
      : mInviteSession(inviteSession),
        mAnswer(answer)
   {
   }

   virtual void executeCommand()
   {
      mInviteSession.provideOffer(mAnswer);
   }

   virtual std::ostream& encodeBrief(std::ostream& strm) const
   {
      return strm << "InviteSessionProvideAnswerCommand";
   }
private:
   InviteSession& mInviteSession;
   SdpContents mAnswer;
};

void
InviteSession::provideAnswerCommand(const SdpContents& answer)
{
   mDum.post(new InviteSessionProvideAnswerCommand(*this, answer));
}

void
InviteSession::end()
{
   end(NotSpecified);
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
         sendBye();
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Ended); 
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
             sendBye();
             transition(Terminated);
             mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::Ended);
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

         sendBye();
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Ended); 
         break;
      }

      case WaitingToTerminate:  // ?slg?  Why is this here?
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

class InviteSessionEndCommand : public DumCommandAdapter
{
public:
   InviteSessionEndCommand(InviteSession& inviteSession, InviteSession::EndReason reason)
      : mInviteSession(inviteSession),
        mReason(reason)
   {
   }

   virtual void executeCommand()
   {
      mInviteSession.end(mReason);
   }

   virtual std::ostream& encodeBrief(std::ostream& strm) const
   {
      return strm << "InviteSessionEndCommand";
   }
private:
   InviteSession& mInviteSession;
   InviteSession::EndReason mReason;
};

void
InviteSession::endCommand(EndReason reason)
{
   mDum.post(new InviteSessionEndCommand(*this, reason));
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

      default:
         assert(0);
         break;
   }
}

class InviteSessionRejectCommand : public DumCommandAdapter
{
public:
   InviteSessionRejectCommand(InviteSession& inviteSession, int code, WarningCategory* warning)
      : mInviteSession(inviteSession),
        mCode(code),
        mWarning(warning?new WarningCategory(*warning):0)
   {
   }

   virtual void executeCommand()
   {
      mInviteSession.reject(mCode, mWarning.get());
   }

   virtual std::ostream& encodeBrief(std::ostream& strm) const
   {
      return strm << "InviteSessionRejectCommand";
   }
private:
   InviteSession& mInviteSession;
   int mCode;
   std::auto_ptr<WarningCategory> mWarning;
};

void
InviteSession::rejectCommand(int code, WarningCategory *warning)
{
   mDum.post(new InviteSessionRejectCommand(*this, code, warning));
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
   if (mSentRefer)
   {
      throw UsageUseException("Attempted to send overlapping refer", __FILE__, __LINE__);
   }

   if (isConnected()) // ?slg? likely not safe in any state except Connected - what should behaviour be if state is ReceivedReinvite?
   {
      mSentRefer = true;
      mReferSub = referSub;
      SharedPtr<SipMessage> refer(new SipMessage());
      mDialog.makeRequest(*refer, REFER);
      refer->header(h_ReferTo) = referTo;
      refer->header(h_ReferredBy) = mDialog.mLocalContact; // 
                                                           // !slg! is it ok to do this - should it be an option?
      if (!referSub)
      {
         refer->header(h_ReferSub).value() = "false";
         refer->header(h_Supporteds).push_back(Token(Symbols::NoReferSub));
      }

      send(refer);
   }
   else
   {
      WarningLog (<< "Can't refer before Connected");
      assert(0);
      throw UsageUseException("REFER not allowed in this context", __FILE__, __LINE__);
   }
}

class InviteSessionReferCommand : public DumCommandAdapter
{
public:
   InviteSessionReferCommand(InviteSession& inviteSession, const NameAddr& referTo, bool referSub)
      : mInviteSession(inviteSession),
      mReferTo(referTo),
      mReferSub(referSub)
   {

   }

   virtual void executeCommand()
   {
      mInviteSession.referCommand(mReferTo, mReferSub);
   }

   virtual std::ostream& encodeBrief(std::ostream& strm) const
   {
      return strm << "InviteSessionReferCommand";
   }

private:
   InviteSession& mInviteSession;
   NameAddr mReferTo;
   bool mReferSub;
};

void
InviteSession::referCommand(const NameAddr& referTo, bool referSub)
{
   mDum.post(new InviteSessionReferCommand(*this, referTo, referSub));
}

void
InviteSession::refer(const NameAddr& referTo, InviteSessionHandle sessionToReplace, bool referSub)
{
   if (!sessionToReplace.isValid())
   {
      throw UsageUseException("Attempted to make a refer w/ and invalid replacement target", __FILE__, __LINE__);
   }

   if (mSentRefer)
   {
      throw UsageUseException("Attempted to send overlapping refer", __FILE__, __LINE__);
   }

   if (isConnected())  // ?slg? likely not safe in any state except Connected - what should behaviour be if state is ReceivedReinvite?
   {
      mSentRefer = true;
      mReferSub = referSub;
      SharedPtr<SipMessage> refer(new SipMessage());      
      mDialog.makeRequest(*refer, REFER);

      refer->header(h_ReferTo) = referTo;
      refer->header(h_ReferredBy) = mDialog.mLocalContact; // ?slg? is it ok to do this - should it be an option?
      CallId replaces;
      DialogId id = sessionToReplace->mDialog.getId();
      replaces.value() = id.getCallId();
      replaces.param(p_toTag) = id.getRemoteTag();
      replaces.param(p_fromTag) = id.getLocalTag();

      refer->header(h_ReferTo).uri().embedded().header(h_Replaces) = replaces;
      
      if (!referSub)
      {
         refer->header(h_ReferSub).value() = "false";
         refer->header(h_Supporteds).push_back(Token(Symbols::NoReferSub));
      }

      send(refer);
   }
   else
   {
      WarningLog (<< "Can't refer before Connected");
      assert(0);
      throw UsageUseException("REFER not allowed in this context", __FILE__, __LINE__);
   }
}

class InviteSessionReferExCommand : public DumCommandAdapter
{
public:
   InviteSessionReferExCommand(InviteSession& inviteSession, const NameAddr& referTo, InviteSessionHandle sessionToReplace, bool referSub)
      : mInviteSession(inviteSession),
      mReferTo(referTo),
      mSessionToReplace(sessionToReplace),
      mReferSub(referSub)
   {

   }

   virtual void executeCommand()
   {
      mInviteSession.referCommand(mReferTo, mSessionToReplace, mReferSub);
   }

   virtual std::ostream& encodeBrief(std::ostream& strm) const
   {
      return strm << "InviteSessionReferExCommand";
   }

private:
   InviteSession& mInviteSession;
   InviteSessionHandle mSessionToReplace;
   NameAddr mReferTo;
   bool mReferSub;
};

void
InviteSession::referCommand(const NameAddr& referTo, InviteSessionHandle sessionToReplace, bool referSub)
{
   mDum.post(new InviteSessionReferExCommand(*this, referTo, sessionToReplace, referSub));
}

void
InviteSession::info(const Contents& contents)
{
   if (mNitState == NitComplete)
   {
      if (isConnected())  // ?slg? likely not safe in any state except Connected - what should behaviour be if state is ReceivedReinvite?
      {
         mNitState = NitProceeding;
         SharedPtr<SipMessage> info(new SipMessage());
         mDialog.makeRequest(*info, INFO);
         // !jf! handle multipart here
         info->setContents(&contents);
         DumHelper::setOutgoingEncryptionLevel(*info, mCurrentEncryptionLevel);
         send(info);
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

class InviteSessionInfoCommand : public DumCommandAdapter
{
public:
   InviteSessionInfoCommand(InviteSession& inviteSession, const Contents& contents)
      : mInviteSession(inviteSession),
        mContents(contents.clone())
   {
   }

   virtual void executeCommand()
   {
      mInviteSession.info(*mContents);
   }

   virtual std::ostream& encodeBrief(std::ostream& strm) const
   {
      return strm << "InviteSessionInfoCommand";
   }
private:
   InviteSession& mInviteSession;
   std::auto_ptr<Contents> mContents;
};

void
InviteSession::infoCommand(const Contents& contents)
{
   mDum.post(new InviteSessionInfoCommand(*this, contents));
}

void
InviteSession::message(const Contents& contents)
{
   if (mNitState == NitComplete)
   {
      if (isConnected())  // ?slg? likely not safe in any state except Connected - what should behaviour be if state is ReceivedReinvite?
      {
         mNitState = NitProceeding;
         SharedPtr<SipMessage> message(new SipMessage());
         mDialog.makeRequest(*message, MESSAGE);
         // !jf! handle multipart here
         message->setContents(&contents);
         DumHelper::setOutgoingEncryptionLevel(*message, mCurrentEncryptionLevel);
         send(message);
         InfoLog (<< "Trying to send MESSAGE: " << message);
      }
      else
      {
         WarningLog (<< "Can't send MESSAGE before Connected");
         assert(0);
         throw UsageUseException("Can't send MESSAGE before Connected", __FILE__, __LINE__);
      }
   }
   else
   {
      throw UsageUseException("Cannot start a non-invite transaction until the previous one has completed",
                              __FILE__, __LINE__);
   }
}

class InviteSessionMessageCommand : public DumCommandAdapter
{
public:
   InviteSessionMessageCommand(InviteSession& inviteSession, const Contents& contents)
      : mInviteSession(inviteSession),
        mContents(contents.clone())
   {
   }

   virtual void executeCommand()
   {
      mInviteSession.message(*mContents);
   }

   virtual std::ostream& encodeBrief(std::ostream& strm) const
   {
      return strm << "InviteSessionMessageCommand";
   }
private:
   InviteSession& mInviteSession;
   std::auto_ptr<Contents> mContents;
};


void
InviteSession::messageCommand(const Contents& contents)
{
   mDum.post(new InviteSessionMessageCommand(*this, contents));
}

void
InviteSession::dispatch(const SipMessage& msg)
{
   // Look for 2xx retransmissions - resend ACK and filter out of state machine
   if(msg.header(h_CSeq).method() == INVITE && msg.isResponse() && msg.header(h_StatusLine).statusCode() / 200 == 1)
   {
      AckMap::iterator i = mAcks.find(msg.header(h_CSeq).sequence());
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
         InfoLog (<< "Retransmitting: " << endl << *mInvite200);
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
               sendBye();
               transition(Terminated);
               mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::Ended); 
            }
            else if(mState == ReceivedReinviteSentOffer)
            {
               transition(Connected);
               mProposedLocalSdp.reset();
               mProposedEncryptionLevel = DialogUsageManager::None;
               //!dcm! -- should this be onIllegalNegotiation?
               mDum.mInviteSessionHandler->onOfferRejected(getSessionHandle(), 0);
            }
            else if(mState == WaitingToOffer || 
                    mState == UAS_WaitingToOffer)
            {
               assert(mProposedLocalSdp.get());
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
      AckMap::iterator i = mAcks.find(timeout.seq());
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
         send(mLastLocalSessionModification);
      }
      else if (mState == SentReinviteNoOfferGlare)
      {
         transition(SentReinviteNoOffer);

         InfoLog (<< "Retransmitting the reINVITE-nooffer (glare condition timer)");
         mDialog.makeRequest(*mLastLocalSessionModification, INVITE);  // increments CSeq
         send(mLastLocalSessionModification);
      }
   }
   else if (timeout.type() == DumTimeout::SessionExpiration)
   {
      if(timeout.seq() == mSessionTimerSeq)
      {
         // this is so the app can decided to ignore this. default implementation
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
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
   {
      case OnInvite:
      case OnInviteReliable:
         *mLastRemoteSessionModification = msg;
         transition(ReceivedReinviteNoOffer);
         //handler->onDialogModified(getSessionHandle(), None, msg);
         handler->onOfferRequired(getSessionHandle(), msg);
         break;

      case OnInviteOffer:
      case OnInviteReliableOffer:
         *mLastRemoteSessionModification = msg;
         transition(ReceivedReinvite);
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         mProposedRemoteSdp = InviteSession::makeSdp(*sdp);

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
         *mLastRemoteSessionModification = msg;
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         mProposedRemoteSdp = InviteSession::makeSdp(*sdp);
         handler->onOffer(getSessionHandle(), msg, *sdp);
         break;

      case OnUpdate:
      {
         // ?slg? no sdp in update - just respond immediately (likely session timer) - do we need a callback?
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
         assert(0);
         break;

      case OnAck:
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
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, msg, 491);
         send(response);
         break;
      }

      case On200Update:
         transition(Connected);
         handleSessionTimerResponse(msg);
         if (sdp.get() && mProposedLocalSdp.get())
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
            mProposedLocalSdp.reset();
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
            mProposedLocalSdp.reset();
            mProposedEncryptionLevel = DialogUsageManager::None;
         }
         break;

      case OnUpdateRejected:
         // !jf! - callback?
         mProposedLocalSdp.reset();
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
      case On2xxOffer:  // !slg! doesn't really make sense
      {
         transition(Connected);
         handleSessionTimerResponse(msg);
         setCurrentLocalSdp(msg);

         // !jf! I need to potentially include an answer in the ACK here
         sendAck();
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         
         if (mSessionRefreshReInvite)
         {
            mSessionRefreshReInvite = false;
         
            MD5Stream currentRemote;
            currentRemote<< *mCurrentRemoteSdp;
            MD5Stream newRemote;
            newRemote << *sdp;
            bool changed = currentRemote.getHex() != newRemote.getHex();

            if (changed)
            {
               mCurrentRemoteSdp = InviteSession::makeSdp(*sdp);
               handler->onRemoteSdpChanged(getSessionHandle(), msg, *sdp);
            }
         }
         else
         {
            handler->onAnswer(getSessionHandle(), msg, *sdp);
         }
         
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
         mProposedLocalSdp.reset();
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
            // ?slg? callback?
            transition(Connected);
            mProposedLocalSdp.reset();
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
      case On487Invite:
      case On489Invite:
         transition(Connected);
         mProposedLocalSdp.reset();
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
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, msg, 491);
         send(response);
         break;
      }

      case On1xx:
      case On1xxEarly:
         // Some UA's send a 100 response to a ReInvite - just ignore it
         break;

      case On2xxAnswer:  // !slg! doesn't really make sense
      case On2xxOffer:
      {
         transition(SentReinviteAnswered);
         handleSessionTimerResponse(msg);
         // mLastSessionModification = msg;   // ?slg? why are we storing 200's?
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         mProposedRemoteSdp = InviteSession::makeSdp(*sdp);
         handler->onOffer(getSessionHandle(), msg, *sdp);

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
         mProposedLocalSdp.reset();
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
            // ?slg? callback?
            transition(Connected);
            mProposedLocalSdp.reset();
            mProposedEncryptionLevel = DialogUsageManager::None;
         }
         break;

      case On491Invite:
         transition(SentReinviteNoOfferGlare);
         start491Timer();
         break;

      case OnGeneralFailure:
         sendBye();
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure, &msg);
         break;

      case OnInviteFailure:
      case On487Invite:
      case On489Invite:
         transition(Connected);
         mProposedLocalSdp.reset();
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
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, msg, 491);
         send(response);
         break;
      }
      case OnAckAnswer:
         transition(Connected);
         setCurrentLocalSdp(msg);
         mCurrentRemoteSdp = InviteSession::makeSdp(*sdp);
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         mCurrentRetransmit200 = 0; // stop the 200 retransmit timer
         handler->onAnswer(getSessionHandle(), msg, *sdp);		 
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
            mProposedLocalSdp.reset();
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
      dispatchConnected(msg);  // act as if we received message in Connected state
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
      dispatchConnected(msg);  // act as if we received message in Connected state
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
      SharedPtr<SipMessage> response(new SipMessage);
      mDialog.makeResponse(*response, msg, 500);
      response->header(h_RetryAfter).value() = Random::getRandom() % 10;
      send(response);
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
      assert(mProposedLocalSdp.get());
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
      sendBye();
      transition(Terminated);
      mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::Ended); 
   }
}

void
InviteSession::dispatchWaitingToHangup(const SipMessage& msg)
{
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
   {
      case OnAck:
      case OnAckAnswer:
      {
         mCurrentRetransmit200 = 0; // stop the 200 retransmit timer

         sendBye();
         transition(Terminated);
         mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::Ended);
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
   SharedPtr<SipMessage> response(new SipMessage);
   mDialog.makeResponse(*response, msg, 400); // !jf! what code to use?
   InfoLog (<< "Sending " << response->brief());
   send(response);

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
      SharedPtr<SipMessage> rsp(new SipMessage);
      mDialog.makeResponse(*rsp, msg, 481);
      send(rsp);

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
      SharedPtr<SipMessage> rsp(new SipMessage);
      mDialog.makeResponse(*rsp, msg, 200);
      send(rsp);

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

      SharedPtr<SipMessage> rsp(new SipMessage);
      InfoLog (<< "Received " << msg.brief());
      mDialog.makeResponse(*rsp, msg, 200);
      send(rsp);

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
      mDialog.makeResponse(*mLastNitResponse, msg, 200);
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
InviteSession::acceptNIT(int statusCode, const Contents * contents)
{
   if (statusCode / 100  != 2)
   {
      throw UsageUseException("Must accept with a 2xx", __FILE__, __LINE__);
   }

   mLastNitResponse->header(h_StatusLine).statusCode() = statusCode;   
   mLastNitResponse->setContents(contents);
   Helper::getResponseCodeReason(statusCode, mLastNitResponse->header(h_StatusLine).reason());
   send(mLastNitResponse);   
} 

class InviteSessionAcceptNITCommand : public DumCommandAdapter
{
public:
   InviteSessionAcceptNITCommand(InviteSession& inviteSession, int statusCode, const Contents* contents)
      : mInviteSession(inviteSession),
        mStatusCode(statusCode),
        mContents(contents?contents->clone():0)
   {

   }

   virtual void executeCommand()
   {
      mInviteSession.acceptNITCommand(mStatusCode, mContents.get());
   }

   virtual std::ostream& encodeBrief(std::ostream& strm) const
   {
      return strm << "InviteSessionAcceptNITCommand";
   }
private:
   InviteSession& mInviteSession;
   int mStatusCode;
   std::auto_ptr<Contents> mContents;
};

void
InviteSession::acceptNITCommand(int statusCode, const Contents* contents)
{
   mDum.post(new InviteSessionAcceptNITCommand(*this, statusCode, contents));
} 

void
InviteSession::rejectNIT(int statusCode)
{
   if (statusCode < 400)
   {
      throw UsageUseException("Must reject with a >= 4xx", __FILE__, __LINE__);
   }
   mLastNitResponse->header(h_StatusLine).statusCode() = statusCode;  
   mLastNitResponse->releaseContents();
   Helper::getResponseCodeReason(statusCode, mLastNitResponse->header(h_StatusLine).reason());
   send(mLastNitResponse);
}

class InviteSessionRejectNITCommand : public DumCommandAdapter
{
public:
   InviteSessionRejectNITCommand(InviteSession& inviteSession, int statusCode)
      : mInviteSession(inviteSession),
      mStatusCode(statusCode)
   {
   }

   virtual void executeCommand()
   {
      mInviteSession.rejectNITCommand(mStatusCode);
   }

   virtual std::ostream& encodeBrief(std::ostream& strm) const
   {
      return strm << "InviteSessionRejectNITCommand";
   }
private:
   InviteSession& mInviteSession;
   int mStatusCode;
};

void
InviteSession::rejectNITCommand(int statusCode)
{
   mDum.post(new InviteSessionRejectNITCommand(*this, statusCode));
}

void
InviteSession::dispatchMessage(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   if (msg.isRequest())
   {
      InfoLog (<< "Received " << msg.brief());
      mDialog.makeResponse(*mLastNitResponse, msg, 200);
      mLastNitResponse->header(h_Contacts).clear();
      handler->onMessage(getSessionHandle(), msg);
   }
   else
   {
      assert(mNitState == NitProceeding);
      mNitState = NitComplete;
      //!dcm! -- toss away 1xx to an message?
      if (msg.header(h_StatusLine).statusCode() >= 300)
      {
         handler->onMessageFailure(getSessionHandle(), msg);
      }
      else if (msg.header(h_StatusLine).statusCode() >= 200)
      {
         handler->onMessageSuccess(getSessionHandle(), msg);
      }
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
      mDialog.makeRequest(*mLastLocalSessionModification, UPDATE);
      mLastLocalSessionModification->releaseContents();  // Don't send SDP
   }
   else
   {
      transition(SentReinvite);
      mDialog.makeRequest(*mLastLocalSessionModification, INVITE);
      InviteSession::setSdp(*mLastLocalSessionModification, mCurrentLocalSdp.get());
      mProposedLocalSdp = InviteSession::makeSdp(*mCurrentLocalSdp.get(), 0);
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
   assert(msg.header(h_CSeq).method() == INVITE || msg.header(h_CSeq).method() == UPDATE);

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
   assert(request.header(h_CSeq).method() == INVITE || request.header(h_CSeq).method() == UPDATE);

   // Allow Re-Invites and Updates to update the Peer P-Asserted-Identity
   if (request.exists(h_PAssertedIdentities))
   {
       mPeerPAssertedIdentities = request.header(h_PAssertedIdentities);
   }

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
      case UAS_ReceivedOfferReliable:
         return "UAS_ReceivedOfferReliable";
      case UAS_NoOfferReliable:
         return "UAS_NoOfferReliable";
      case UAS_FirstSentOfferReliable:
         return "UAS_FirstSentOfferReliable";
      case UAS_FirstSentAnswerReliable:
         return "UAS_FirstSentAnswerReliable";
      case UAS_NegotiatedReliable:
         return "UAS_NegotiatedReliable";
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
      case UAS_WaitingToRequestOffer:
         return "UAS_WaitingToRequestOffer";
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
   if(msg.method() != INVITE)
   {
      return false;
   }
   if(msg.isRequest())
   {
      return mDum.getMasterProfile()->getUasReliableProvisionalMode() > MasterProfile::Never
         && (msg.exists(h_Supporteds) && msg.header(h_Supporteds).find(Token(Symbols::C100rel)) 
             || msg.exists(h_Requires)   && msg.header(h_Requires).find(Token(Symbols::C100rel)));
   }
   else
   {
      return (msg.exists(h_Supporteds) && msg.header(h_Supporteds).find(Token(Symbols::C100rel))) 
         || (msg.exists(h_Requires) && msg.header(h_Requires).find(Token(Symbols::C100rel)));
   }
}

std::auto_ptr<SdpContents>
InviteSession::getSdp(const SipMessage& msg)
{
   return Helper::getSdp(msg.getContents());
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

void 
InviteSession::provideProposedOffer()
{
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

InviteSession::Event
InviteSession::toEvent(const SipMessage& msg, const SdpContents* sdp)
{
   MethodTypes method = msg.header(h_CSeq).method();
   int code = msg.isResponse() ? msg.header(h_StatusLine).statusCode() : 0;
   
   //.dcm. Treat an invite as reliable if UAS 100rel support is enabled. For
   //responses, reiable provisionals should only be received if the invite was
   //sent reliably.  Spurious reliable provisional respnoses are dropped outside
   //the state machine.
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
   else if (method == UPDATE && code == 488)
   {
      return On488Update;
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

void InviteSession::sendAck(const SdpContents *sdp)
{
   SharedPtr<SipMessage> ack(new SipMessage);

   assert(mAcks.count(mLastLocalSessionModification->header(h_CSeq).sequence()) == 0);

   mDialog.makeRequest(*ack, ACK);

   // Copy Authorization, Proxy Authorization headers and CSeq from original Invite
   if(mLastLocalSessionModification->exists(h_Authorizations))
   {
      ack->header(h_Authorizations) = mLastLocalSessionModification->header(h_Authorizations);
   }
   if(mLastLocalSessionModification->exists(h_ProxyAuthorizations))
   {
      ack->header(h_ProxyAuthorizations) = mLastLocalSessionModification->header(h_ProxyAuthorizations);
   }
   ack->header(h_CSeq).sequence() = mLastLocalSessionModification->header(h_CSeq).sequence();

   if(sdp != 0)
   {
      setSdp(*ack, *sdp);
   }
   mAcks[ack->header(h_CSeq).sequence()] = ack;
   mDum.addTimerMs(DumTimeout::CanDiscardAck, Timer::TH, getBaseHandle(), ack->header(h_CSeq).sequence());

   InfoLog (<< "Sending " << ack->brief());
   send(ack);
}

void InviteSession::sendBye()
{
   SharedPtr<SipMessage> bye(new SipMessage());
   mDialog.makeRequest(*bye, BYE);
   Data txt;
   if (mEndReason != NotSpecified)
   {
      Token reason("SIP");
      txt = getEndReasonString(mEndReason);
      reason.param(p_description) = txt;
      bye->header(h_Reasons).push_back(reason);      
   }
   
   InfoLog (<< myAddr() << " Sending BYE " << txt);
   send(bye);
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
   mProposedLocalSdp.reset();   
}

void InviteSession::onReadyToSend(SipMessage& msg)
{
   mDum.mInviteSessionHandler->onReadyToSend(getSessionHandle(), msg);
}

void InviteSession::referNoSub(const SipMessage& msg)
{
   assert(msg.isRequest() && msg.header(h_CSeq).method()==REFER);
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

