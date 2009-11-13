#include "resip/stack/MultipartMixedContents.hxx"
#include "resip/stack/MultipartAlternativeContents.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/DialogEventStateManager.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/DumTimeout.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/ServerInviteSession.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/UsageUseException.hxx"
#include "resip/dum/DumHelper.hxx"
#include "resip/dum/DumCommand.hxx"
#include "rutil/Logger.hxx"
#include "rutil/compat.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

ServerInviteSession::ServerInviteSession(DialogUsageManager& dum, Dialog& dialog, const SipMessage& request)
   : InviteSession(dum, dialog),
     mFirstRequest(request),
     m1xx(new SipMessage),
     mCurrentRetransmit1xx(0),
     mAnswerSentReliably(false)
{
   assert(request.isRequest());
   mState = UAS_Start;
}

ServerInviteSessionHandle 
ServerInviteSession::getHandle()
{
   return ServerInviteSessionHandle(mDum, getBaseHandle().getId());
}

void 
ServerInviteSession::redirect(const NameAddrs& contacts, int code)
{
   InfoLog (<< toData(mState) << ": redirect(" << code << ")"); // -> " << contacts);

   switch (mState)
   {
      case UAS_EarlyNoOffer:
      case UAS_EarlyOffer:
      case UAS_EarlyProvidedAnswer:
      case UAS_EarlyProvidedOffer:
      case UAS_NegotiatedReliable:
      case UAS_FirstSentAnswerReliable:
      case UAS_FirstSentOfferReliable:
      case UAS_NoOffer:
      case UAS_NoOfferReliable:
      case UAS_ProvidedOfferReliable:
      case UAS_Offer:
      case UAS_OfferProvidedAnswer:
      case UAS_ReceivedOfferReliable: 
      case UAS_ProvidedOffer:
      case UAS_ReceivedUpdate:
      case UAS_ReceivedUpdateWaitingAnswer:
      case UAS_SentUpdate:
      {
         // !jf! the cleanup for 3xx may be a bit strange if we are in the middle of
         // an offer/answer exchange with PRACK. 
         // e.g. we sent 183 reliably and then 302 before PRACK was received. Ideally,
         // we should send 200PRACK
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, mFirstRequest, code);
         response->header(h_Contacts) = contacts;
         send(response);

         if (mDum.mDialogEventStateManager)
         {
            mDum.mDialogEventStateManager->onTerminated(mDialog, *response, InviteSessionHandler::Rejected);
         }

         transition(Terminated);

         mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::Referred); 
         mDum.destroy(this);
         break;
      }

      case UAS_Accepted:
      case UAS_WaitingToOffer:
      case UAS_WaitingToRequestOffer:
      case UAS_WaitingToHangup:
      case UAS_WaitingToTerminate:
      case UAS_SentUpdateAccepted:
      case UAS_Start:
      default:
         assert(0);
         throw UsageUseException("Can't redirect after accepted", __FILE__, __LINE__);
         break;
   }
}

class ServerInviteSessionRedirectCommand : public DumCommandAdapter
{
public:
   ServerInviteSessionRedirectCommand(ServerInviteSession& serverInviteSession, const NameAddrs& contacts, int code)
      : mServerInviteSession(serverInviteSession),
      mContacts(contacts),
      mCode(code)
   {

   }

   virtual void executeCommand()
   {
      mServerInviteSession.redirect(mContacts, mCode);
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "ServerInviteSessionRedirectCommand";
   }
private:
   ServerInviteSession& mServerInviteSession;
   NameAddrs mContacts;
   int mCode;
};

void 
ServerInviteSession::redirectCommand(const NameAddrs& contacts, int code)
{
   mDum.post(new ServerInviteSessionRedirectCommand(*this, contacts, code));
}

void 
ServerInviteSession::provisional(int code, bool earlyFlag)
{
   InfoLog (<< toData(mState) << ": provisional(" << code << ")");

   switch (mState)
   {
      case UAS_Offer:
         transition(UAS_EarlyOffer);
         sendProvisional(code, earlyFlag);
         break;

      case UAS_OfferProvidedAnswer:
      case UAS_EarlyProvidedAnswer:
         transition(UAS_EarlyProvidedAnswer);
         sendProvisional(code, earlyFlag);
         break;

      case UAS_ProvidedOffer:
      case UAS_EarlyProvidedOffer:
         transition(UAS_EarlyProvidedOffer);
         sendProvisional(code, earlyFlag);
         break;
         
      case UAS_EarlyOffer:
         transition(UAS_EarlyOffer);
         sendProvisional(code, earlyFlag);
         break;
         
      case UAS_NoOffer:
      case UAS_EarlyNoOffer:
         transition(UAS_EarlyNoOffer);
         sendProvisional(code, earlyFlag);
         break;
         

      case UAS_NoOfferReliable:
      case UAS_NegotiatedReliable:
      case UAS_FirstSentAnswerReliable:
      case UAS_FirstNoAnswerReliable: 
      case UAS_NoAnswerReliable:
      case UAS_FirstSentOfferReliable:
         if(mUnacknowledgedProvisionals.size()>0)
         {
            queueReliableProvisional(code, earlyFlag);
         }
         else
         {
            sendProvisional(code, earlyFlag);
         }
         break;
         
      case UAS_ReceivedOfferReliable: 
         if(code!=100)
         {
            transition(UAS_FirstNoAnswerReliable);
         }
         sendProvisional(code, earlyFlag);
         break;

      case UAS_ProvidedOfferReliable:
         if(code!=100)
         {
            transition(UAS_FirstSentOfferReliable);
         }
         sendProvisional(code, earlyFlag);
         break;

      case UAS_ReceivedOfferReliableProvidedAnswer: 
         if(code!=100)
         {
            transition(UAS_FirstSentAnswerReliable);
         }
         sendProvisional(code, earlyFlag);
         break;

      case UAS_Accepted:
      case UAS_AcceptedWaitingAnswer:
      case UAS_WaitingToOffer:
      case UAS_WaitingToRequestOffer:
      case UAS_ReceivedUpdate:
      case UAS_ReceivedUpdateWaitingAnswer:
      case UAS_SentUpdate:
      case UAS_SentUpdateAccepted:
      case UAS_Start:
      case UAS_WaitingToHangup:
      case UAS_WaitingToTerminate:
      default:
         assert(0);
         break;
   }
}

class ServerInviteSessionProvisionalCommand : public DumCommandAdapter
{
public:
   ServerInviteSessionProvisionalCommand(ServerInviteSession& serverInviteSession, int statusCode)
      : mServerInviteSession(serverInviteSession),
        mStatusCode(statusCode)
   {
   }

   virtual void executeCommand()
   {
      mServerInviteSession.provisional(mStatusCode);
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "ServerInviteSessionProvisionalCommand";
   }
private:
   ServerInviteSession& mServerInviteSession;
   int mStatusCode;
};

void 
ServerInviteSession::provisionalCommand(int statusCode)
{
   mDum.post(new ServerInviteSessionProvisionalCommand(*this, statusCode));
}

void
ServerInviteSession::provideOffer(const SdpContents& offer,
                                  DialogUsageManager::EncryptionLevel level, 
                                  const SdpContents* alternative)
{
   InfoLog (<< toData(mState) << ": provideOffer");
   mAnswerSentReliably = false;
   switch (mState)
   {
      case UAS_NoOffer:
         transition(UAS_ProvidedOffer);
         mProposedLocalSdp = InviteSession::makeSdp(offer, alternative);
         mProposedEncryptionLevel = level;
         break;

      case UAS_EarlyNoOffer:
         transition(UAS_EarlyProvidedOffer);
         mProposedLocalSdp = InviteSession::makeSdp(offer, alternative);
         mProposedEncryptionLevel = level;
         break;
         
      case UAS_NoOfferReliable:
         transition(UAS_ProvidedOfferReliable);
         mProposedLocalSdp = InviteSession::makeSdp(offer, alternative);
         mProposedEncryptionLevel = level;
         break;

      case UAS_NegotiatedReliable:
         // queue offer
         transition(UAS_SentUpdate);
         mProposedLocalSdp = InviteSession::makeSdp(offer, alternative);
         mProposedEncryptionLevel = level;
         sendUpdate(offer);
         break;
         
      case UAS_Accepted:
         // queue the offer to be sent after the ACK is received
         transition(UAS_WaitingToOffer);
         mProposedLocalSdp = InviteSession::makeSdp(offer);
         mProposedEncryptionLevel = level;
         break;

      case UAS_WaitingToOffer:
         InviteSession::provideOffer(offer, level, alternative);
         break;

      case UAS_EarlyProvidedAnswer:
      case UAS_EarlyProvidedOffer:
      case UAS_FirstSentAnswerReliable:
      case UAS_FirstSentOfferReliable:
      case UAS_Offer:
      case UAS_EarlyOffer:
      case UAS_ReceivedOfferReliable: 
      case UAS_OfferProvidedAnswer:
      case UAS_ProvidedOffer:
      case UAS_ProvidedOfferReliable:
      case UAS_ReceivedUpdate:
      case UAS_ReceivedUpdateWaitingAnswer:
      case UAS_SentUpdate:
      case UAS_SentUpdateAccepted:
      case UAS_Start:
      case UAS_WaitingToHangup:
      case UAS_WaitingToTerminate:
      case UAS_WaitingToRequestOffer:
      case UAS_AcceptedWaitingAnswer:
         assert(0);
         break;
      default:
         InviteSession::provideOffer(offer, level, alternative);
         break;
   }
}

void 
ServerInviteSession::provideOffer(const SdpContents& offer)
{
   this->provideOffer(offer, mCurrentEncryptionLevel, 0);
}


void
ServerInviteSession::requestOffer()
{
   InfoLog (<< toData(mState) << ": requestOffer");
   switch (mState)
   {
      case UAS_Accepted:
         // queue the request to be sent after the ACK is received
         transition(UAS_WaitingToRequestOffer);
         break;

      case UAS_WaitingToRequestOffer:
         InviteSession::requestOffer();
         break;

      default:
         InviteSession::requestOffer();
         break;
   }
}

void 
ServerInviteSession::provideAnswer(const SdpContents& answer)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   InfoLog (<< toData(mState) << ": provideAnswer");
   mAnswerSentReliably = false;
   switch (mState)
   {
      case UAS_Offer:
         transition(UAS_OfferProvidedAnswer);
         mCurrentRemoteSdp = mProposedRemoteSdp;
         mCurrentLocalSdp = InviteSession::makeSdp(answer);
         break;

      case UAS_EarlyOffer:
         transition(UAS_EarlyProvidedAnswer);
         mCurrentRemoteSdp = mProposedRemoteSdp;
         mCurrentLocalSdp = InviteSession::makeSdp(answer);
         break;
         
      case UAS_ReceivedOfferReliable: 
         mCurrentRemoteSdp = mProposedRemoteSdp;
         mCurrentLocalSdp = InviteSession::makeSdp(answer);
         transition(UAS_ReceivedOfferReliableProvidedAnswer);
         break;

      case UAS_FirstNoAnswerReliable: 
         mCurrentRemoteSdp = mProposedRemoteSdp;
         mCurrentLocalSdp = InviteSession::makeSdp(answer);
         break;

      case UAS_ReceivedUpdate:
         // send::200U-answer - TODO
         transition(UAS_NegotiatedReliable);
         break;
         
      case UAS_ReceivedUpdateWaitingAnswer:
         // send::2XXU-answer - TODO
         // send::2XXI - TODO
         transition(UAS_Accepted);
         handler->onConnected(getSessionHandle(), *mInvite200);
         break;

      case UAS_NoAnswerReliable:
         mCurrentRemoteSdp = mProposedRemoteSdp;
         mCurrentLocalSdp = InviteSession::makeSdp(answer);
         break;

      case UAS_NegotiatedReliable:
         mCurrentRemoteSdp = mProposedRemoteSdp;
         mCurrentLocalSdp = InviteSession::makeSdp(answer);
         if(mPrackWithOffer.get())
         {
            SharedPtr<SipMessage> p200(new SipMessage);
            mDialog.makeResponse(*p200, *mPrackWithOffer, 200);
            setSdp(*p200, mCurrentLocalSdp.get());
            mAnswerSentReliably = true;
            mPrackWithOffer.reset();
            send(p200);
         }
         break;

      case UAS_Accepted:
      case UAS_WaitingToOffer:
      case UAS_WaitingToRequestOffer:
      case UAS_EarlyNoOffer:
      case UAS_EarlyProvidedAnswer:
      case UAS_EarlyProvidedOffer:
      case UAS_FirstSentAnswerReliable:
      case UAS_FirstSentOfferReliable:
      case UAS_NoOffer:
      case UAS_NoOfferReliable:
      case UAS_OfferProvidedAnswer:
      case UAS_ProvidedOffer:
      case UAS_ProvidedOfferReliable:
      case UAS_SentUpdate:
      case UAS_SentUpdateAccepted:
      case UAS_Start:
      case UAS_WaitingToHangup:
      case UAS_WaitingToTerminate:
      case UAS_AcceptedWaitingAnswer:
         assert(0);
         break;
      default:
         InviteSession::provideAnswer(answer);
         break;
   }
}

void
ServerInviteSession::end()
{
   end(NotSpecified);
}

void 
ServerInviteSession::end(EndReason reason)
{
   InfoLog (<< toData(mState) << ": end");
   if (mEndReason == NotSpecified)
   {
      mEndReason = reason;   
   }
   
   switch (mState)
   {
      case UAS_EarlyNoOffer:
      case UAS_EarlyOffer:
      case UAS_EarlyProvidedAnswer:
      case UAS_EarlyProvidedOffer:
      case UAS_NoOffer:
      case UAS_Offer:
      case UAS_OfferProvidedAnswer:
      case UAS_ProvidedOffer:
      case UAS_ProvidedOfferReliable:
         reject(480);
         break;         
         
      case UAS_ReceivedOfferReliable: 
      case UAS_ReceivedOfferReliableProvidedAnswer:
      case UAS_NegotiatedReliable:
      case UAS_FirstSentOfferReliable:
      case UAS_FirstSentAnswerReliable:
      case UAS_NoOfferReliable:
      case UAS_ReceivedUpdate:   // !slg! todo: send 488U
      case UAS_ReceivedUpdateWaitingAnswer: // !slg! todo: send 488U
      case UAS_SentUpdate:
      case UAS_WaitingToTerminate:
         reject(480);
         break;
         
      case UAS_Start:
         assert(0);
         break;

      case UAS_Accepted:
      case UAS_WaitingToOffer:
      case UAS_WaitingToRequestOffer:
      case UAS_SentUpdateAccepted:
      case UAS_AcceptedWaitingAnswer:
         if(mCurrentRetransmit200)  // If retransmit200 timer is active then ACK is not received yet - wait for it
         {
            transition(UAS_WaitingToHangup);
         }
         else
         {
             // ACK has likely timedout - hangup immediately
             sendBye();
             transition(Terminated);
             mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::LocalBye);
         }
         break;

      case UAS_WaitingToHangup:     // This can happen if we are waiting for an ACK to hangup and the ACK timesout
          break;
         
      default:
         InviteSession::end(reason);
         break;
   }
}

void 
ServerInviteSession::reject(int code, WarningCategory *warning)
{
   InfoLog (<< toData(mState) << ": reject(" << code << ")");

   switch (mState)
   {
      case UAS_EarlyNoOffer:
      case UAS_EarlyOffer:
      case UAS_EarlyProvidedAnswer:
      case UAS_EarlyProvidedOffer:
      case UAS_NoOffer:
      case UAS_Offer:
      case UAS_OfferProvidedAnswer:
      case UAS_ProvidedOffer:
      case UAS_ProvidedOfferReliable:

      case UAS_NegotiatedReliable:
      case UAS_FirstSentAnswerReliable:
      case UAS_FirstNoAnswerReliable:
      case UAS_NoAnswerReliable:
      case UAS_FirstSentOfferReliable:
      case UAS_NoOfferReliable:
      case UAS_ReceivedOfferReliable: 
      case UAS_ReceivedOfferReliableProvidedAnswer:
      case UAS_ReceivedUpdate:
      case UAS_SentUpdate:
      {
         // !jf! the cleanup for 3xx may be a bit strange if we are in the middle of
         // an offer/answer exchange with PRACK. 
         // e.g. we sent 183 reliably and then 302 before PRACK was received. Ideally,
         // we should send 200PRACK
         SharedPtr<SipMessage> response(new SipMessage);
         mDialog.makeResponse(*response, mFirstRequest, code);
         if(warning)
         {
            response->header(h_Warnings).push_back(*warning);
         }
         send(response);

         transition(Terminated);
         mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::Rejected); 
         mDum.destroy(this);
         break;
      }

      case UAS_Accepted:
      case UAS_WaitingToOffer:
      case UAS_WaitingToRequestOffer:
      case UAS_ReceivedUpdateWaitingAnswer:
      case UAS_SentUpdateAccepted:
      case UAS_Start:
      case UAS_WaitingToHangup:
      case UAS_WaitingToTerminate:
         assert(0);
         break;

      default:
         InviteSession::reject(code);
         break;
   }
}

void 
ServerInviteSession::accept(int code)
{
   InfoLog (<< toData(mState) << ": accept(" << code << ")");
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   switch (mState)
   {
      case UAS_Offer:
      case UAS_EarlyOffer:
         assert(0);
         break;

      case UAS_OfferProvidedAnswer:
      case UAS_EarlyProvidedAnswer:
         transition(UAS_Accepted);
         sendAccept(code, mCurrentLocalSdp.get());
         handler->onConnected(getSessionHandle(), *mInvite200);
         break;
         
      case UAS_NoOffer:
      case UAS_EarlyNoOffer:
         assert(0);

      case UAS_ProvidedOffer:
      case UAS_ProvidedOfferReliable:
      case UAS_EarlyProvidedOffer:
         transition(UAS_AcceptedWaitingAnswer);
         sendAccept(code, mProposedLocalSdp.get());
         break;
         
      case UAS_Accepted:
      case UAS_WaitingToOffer:
      case UAS_WaitingToRequestOffer:
         assert(0);  // Already Accepted
         break;
         
      case UAS_FirstSentAnswerReliable:
      case UAS_FirstSentOfferReliable:
      case UAS_FirstNoAnswerReliable:
         // queue 2xx
         // waiting for PRACK
         InfoLog (<< "Waiting for PRACK. queued 200 OK" );
         mQueuedProvisionals.push_back( std::make_pair(code,false) );
         transition(UAS_Accepted);
         mDialog.makeResponse(*mInvite200, mFirstRequest, code);
         handleSessionTimerRequest(*mInvite200, mFirstRequest);
         // this is stange - TODO review me
         break;
         
      case UAS_NegotiatedReliable:
         transition(UAS_Accepted);
         sendAccept(code, 0);
         handler->onConnected(getSessionHandle(), *mInvite200);
         break;

      case UAS_ReceivedOfferReliableProvidedAnswer: 
         transition(UAS_Accepted);
         sendAccept(code, mCurrentLocalSdp.get());
         handler->onConnected(getSessionHandle(), *mInvite200);
         break;

      case UAS_NoAnswerReliable:
         if (mAnswerSentReliably)
         {
            InfoLog (<< "Waiting for PRACK. queued 200 OK" );
            mQueuedProvisionals.push_back( std::make_pair(code,false) );
         }
         else
         {
            transition(UAS_Accepted);
            sendAccept(code, mCurrentLocalSdp.get());
            handler->onConnected(getSessionHandle(), *mInvite200);
         }
         break;

      case UAS_SentUpdate:
         transition(UAS_SentUpdateAccepted);
         sendAccept(code, 0);
         break;

      case UAS_ReceivedUpdate:
         transition(UAS_ReceivedUpdateWaitingAnswer);
         mDialog.makeResponse(*mInvite200, mFirstRequest, code);// queue 2xx
         handleSessionTimerRequest(*mInvite200, mFirstRequest);
         break;
         
      case UAS_NoOfferReliable:
      case UAS_ReceivedOfferReliable: 
      case UAS_ReceivedUpdateWaitingAnswer:
      case UAS_SentUpdateAccepted:
      case UAS_Start:
      case UAS_WaitingToHangup:
      case UAS_WaitingToTerminate:
      default:
         assert(0);
         break;
   }
}

class ServerInviteSessionAcceptCommand : public DumCommandAdapter
{
public:
   ServerInviteSessionAcceptCommand(ServerInviteSession& serverInviteSession, int statusCode)
      : mServerInviteSession(serverInviteSession),
        mStatusCode(statusCode)
   {
   }

   virtual void executeCommand()
   {
      mServerInviteSession.accept(mStatusCode);
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "ServerInviteSessionAcceptCommand";
   }
private:
   ServerInviteSession& mServerInviteSession;
   int mStatusCode;
};

void 
ServerInviteSession::acceptCommand(int statusCode)
{
   mDum.post(new ServerInviteSessionAcceptCommand(*this, statusCode));
}

void 
ServerInviteSession::dispatch(const SipMessage& msg)
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

   switch (mState)
   {
      case UAS_Start:
         dispatchStart(msg);
         break;

      case UAS_Offer:
      case UAS_EarlyOffer:
      case UAS_EarlyProvidedAnswer:
      case UAS_NoOffer:
      case UAS_ProvidedOffer:
      case UAS_ProvidedOfferReliable:
      case UAS_EarlyNoOffer:
      case UAS_EarlyProvidedOffer:
      case UAS_OfferProvidedAnswer:
         dispatchOfferOrEarly(msg);
         break;       
      case UAS_Accepted:
         dispatchAccepted(msg);
         break;
      case UAS_WaitingToOffer:
         dispatchWaitingToOffer(msg);
         break;
      case UAS_WaitingToRequestOffer:
         dispatchWaitingToRequestOffer(msg);
         break;
      case UAS_AcceptedWaitingAnswer:
         dispatchAcceptedWaitingAnswer(msg);
         break;         
      case UAS_NoAnswerReliable:
      case UAS_NegotiatedReliable:
         dispatchEarlyReliable(msg);
         break;
      case UAS_FirstSentAnswerReliable:
         dispatchFirstSentAnswerReliable(msg);
         break;
      case UAS_FirstNoAnswerReliable:
         dispatchFirstNoAnswerReliable(msg);
         break;
      case UAS_FirstSentOfferReliable:
         dispatchFirstSentOfferReliable(msg);
         break;
      case UAS_NoOfferReliable:
         dispatchNoOfferReliable(msg);
         break;
      case UAS_ReceivedOfferReliable:
         dispatchOfferReliable(msg);
         break;
      case UAS_ReceivedUpdate:
         dispatchReceivedUpdate(msg);
         break;
      case UAS_ReceivedUpdateWaitingAnswer:
         dispatchReceivedUpdateWaitingAnswer(msg);
         break;
      case UAS_SentUpdate:
         dispatchSentUpdate(msg);
         break;
      case UAS_WaitingToHangup:
         dispatchWaitingToHangup(msg);
         break;
      case UAS_WaitingToTerminate:
         dispatchWaitingToTerminate(msg);
         break;
      case UAS_SentUpdateAccepted:
         dispatchSentUpdateAccepted(msg);
         break;
      default:
         InviteSession::dispatch(msg);
         break;
   }
}

void 
ServerInviteSession::dispatch(const DumTimeout& timeout)
{
   if (timeout.type() == DumTimeout::Retransmit1xx)
   {
      if (mCurrentRetransmit1xx && m1xx->header(h_CSeq).sequence() == timeout.seq())  // If timer isn't stopped and this timer is for last 1xx sent, then resend
      {
         send(m1xx);
         startRetransmit1xxTimer();
      }
   }
   else if (timeout.type() == DumTimeout::Retransmit1xxRel)
   {
      if (mUnacknowledgedProvisionals.size() 
          && mUnacknowledgedProvisionals.front()->header(h_RSeq).value() == timeout.seq())
      {
         unsigned int duration = 2*timeout.secondarySeq();
         if(duration>=64*Timer::T1)
         {
            InfoLog (<< "Reliable provisional timeout" );
            SharedPtr<SipMessage> i504(new SipMessage);
            mDialog.makeResponse(*i504, mFirstRequest, 504);
            send(i504);
 
            transition(Terminated);
 
            if (mDum.mDialogEventStateManager)
            {
               SipMessage msg;  // should onTerminated require this msg?
               mDum.mDialogEventStateManager->onTerminated(mDialog, msg, InviteSessionHandler::Timeout);
            }
 
            mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::Timeout);
            mDum.destroy(this);
            return;
          
         }
         else
         {
            InfoLog (<< "Reliable provisional retransmit" );
            send(mUnacknowledgedProvisionals.front());
            mDum.addTimerMs(DumTimeout::Retransmit1xxRel, duration, getBaseHandle(), timeout.seq(), duration);
         }
      }
   }
   else
   {
      InviteSession::dispatch(timeout);
   }
}

void
ServerInviteSession::dispatchStart(const SipMessage& msg)
{
   assert(msg.isRequest());
   assert(msg.header(h_CSeq).method() == INVITE);

   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);
   storePeerCapabilities(msg);

   if (mDum.mDialogEventStateManager)
   {
      mDum.mDialogEventStateManager->onTryingUas(mDialog, msg);
   }

   switch (toEvent(msg, sdp.get()))
   {
      case OnInviteOffer:
         *mLastRemoteSessionModification = msg;
         transition(UAS_Offer);
         mProposedRemoteSdp = InviteSession::makeSdp(*sdp);
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         handler->onNewSession(getHandle(), InviteSession::Offer, msg);
         if(!isTerminated())  
         {
            handler->onOffer(getSessionHandle(), msg, *sdp);
         }
         break;
      case OnInvite:
         *mLastRemoteSessionModification = msg;
         transition(UAS_NoOffer);
         handler->onNewSession(getHandle(), InviteSession::None, msg);
         if(!isTerminated())  
         {
            handler->onOfferRequired(getSessionHandle(), msg);
         }
         break;
      case OnInviteReliableOffer:
         *mLastRemoteSessionModification = msg;
         transition(UAS_ReceivedOfferReliable);
         mProposedRemoteSdp = InviteSession::makeSdp(*sdp);
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         handler->onNewSession(getHandle(), InviteSession::Offer, msg);
         if(!isTerminated())  
         {
            handler->onOffer(getSessionHandle(), msg, *sdp);
         }
         break;
      case OnInviteReliable:
         *mLastRemoteSessionModification = msg;
         transition(UAS_NoOfferReliable);
         handler->onNewSession(getHandle(), InviteSession::None, msg);
         if(!isTerminated())  
         {
            handler->onOfferRequired(getSessionHandle(), msg);
         }
         break;
      default:
         assert(0);
         break;
   }
}

// Offer, Early, EarlyNoOffer, NoOffer
void
ServerInviteSession::dispatchOfferOrEarly(const SipMessage& msg)
{
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);
   switch (toEvent(msg, sdp.get()))
   {
      case OnCancel:
         dispatchCancel(msg);
         break;
      case OnBye:
         dispatchBye(msg);
         break;
      default:
         assert(msg.isRequest());
         dispatchUnknown(msg);
         break;
   }
}

void
ServerInviteSession::dispatchAccepted(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);
   InfoLog (<< "dispatchAccepted: " << msg.brief());
   
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

      case OnAck:
      case OnAckAnswer:  // illegal but accept anyway for improved interop
      {
         mCurrentRetransmit200 = 0; // stop the 200 retransmit timer
         transition(Connected);
         handler->onConnectedConfirmed(getSessionHandle(), msg);  
         break;
      }
      
      case OnCancel:
      {
         // Cancel and 200 crossed
         SharedPtr<SipMessage> c200(new SipMessage);
         mDialog.makeResponse(*c200, msg, 200);
         send(c200);
         break;
      }

      case OnBye:
      {
         SharedPtr<SipMessage> b200(new SipMessage);
         mDialog.makeResponse(*b200, msg, 200);
         send(b200);

         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::RemoteBye, &msg);
         mDum.destroy(this);
         break;
      }
        
      case OnPrack:
      {
         if(!prackCheckProvisionals(msg))
         {
            return; // prack does not correspond to an unacknowedged provisional
         }
         
         if(sdp.get())  // New offer
         {
            if(!mAnswerSentReliably)    // new offer before previous answer sent 
            {
               // TODO - reject PRACK, teardown call?
               ErrLog (<< "PRACK with new offer when in state=" << toData(mState));
               return;
               assert(0);
            }
            else
            {
               // dispatch offer here and respond with 200OK in provideAnswer
               transition(UAS_NegotiatedReliable);  // TODO - this doesn't look right - needs review
               mPrackWithOffer = resip::SharedPtr<SipMessage>(new SipMessage(msg));
               mProposedRemoteSdp = InviteSession::makeSdp(*sdp);
               mCurrentEncryptionLevel = getEncryptionLevel(msg);
               if(!isTerminated())  
               {
                  handler->onOffer(getSessionHandle(), msg, *sdp);
               }
            }
         }
         else
         {
             SharedPtr<SipMessage> p200(new SipMessage);
             mDialog.makeResponse(*p200, msg, 200);
             send(p200);
             prackCheckQueue();
         }
         break;
      }
      
      default:
         if(msg.isRequest())
         {
            dispatchUnknown(msg);
         }
         break;
   }
}

void
ServerInviteSession::dispatchWaitingToOffer(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);
   InfoLog (<< "dispatchWaitingToOffer: " << msg.brief());
   
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

      case OnAck:
      case OnAckAnswer:   // illegal but accept anyway for improved interop
      {
         assert(mProposedLocalSdp.get());
         mCurrentRetransmit200 = 0; // stop the 200 retransmit timer
         provideProposedOffer(); 
         break;
      }
      
      case OnCancel:
      {
         // Cancel and 200 crossed
         SharedPtr<SipMessage> c200(new SipMessage);
         mDialog.makeResponse(*c200, msg, 200);
         send(c200);
         break;
      }

      case OnBye:
      {
         SharedPtr<SipMessage> b200(new SipMessage);
         mDialog.makeResponse(*b200, msg, 200);
         send(b200);

         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::RemoteBye, &msg);
         mDum.destroy(this);
         break;
      }
              
      default:
         if(msg.isRequest())
         {
            dispatchUnknown(msg);
         }
         break;
   }
}

void
ServerInviteSession::dispatchWaitingToRequestOffer(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);
   InfoLog (<< "dispatchWaitingToRequestOffer: " << msg.brief());
   
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

      case OnAck:
      case OnAckAnswer:   // illegal but accept anyway for improved interop
      {
         mCurrentRetransmit200 = 0; // stop the 200 retransmit timer
         requestOffer(); 
         break;
      }
      
      case OnCancel:
      {
         // Cancel and 200 crossed
         SharedPtr<SipMessage> c200(new SipMessage);
         mDialog.makeResponse(*c200, msg, 200);
         send(c200);
         break;
      }

      case OnBye:
      {
         SharedPtr<SipMessage> b200(new SipMessage);
         mDialog.makeResponse(*b200, msg, 200);
         send(b200);

         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::RemoteBye, &msg);
         mDum.destroy(this);
         break;
      }
              
      default:
         if(msg.isRequest())
         {
            dispatchUnknown(msg);
         }
         break;
   }
}

void
ServerInviteSession::dispatchAcceptedWaitingAnswer(const SipMessage& msg)
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
      {
         mCurrentRetransmit200 = 0; // stop the 200 retransmit timer
         transition(Connected);
         setCurrentLocalSdp(msg);
         mCurrentEncryptionLevel = getEncryptionLevel(msg);
         mCurrentRemoteSdp = InviteSession::makeSdp(*sdp);
         handler->onAnswer(getSessionHandle(), msg, *sdp);
         if(!isTerminated())  // onAnswer callback may call end() or reject()
         {
            handler->onConnected(getSessionHandle(), msg);
         }
         break;
      }

      case OnAck:
      {
         mCurrentRetransmit200 = 0; // stop the 200 retransmit timer
         mEndReason = IllegalNegotiation;
         sendBye();
         transition(Terminated);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
         break;
      }
         
      case OnCancel:
      {
         // no transition

         SharedPtr<SipMessage> c200(new SipMessage);
         mDialog.makeResponse(*c200, msg, 200);
         send(c200);
         break;
      }

      case OnPrack:
      {
         if(!prackCheckProvisionals(msg))
         {
            return; // prack does not correspond to an unacknowedged provisional
         }
         
         if(sdp.get())
         {
            // TODO - reject PRACK, teardown call?
            ErrLog (<< "PRACK with new offer when in state=" << toData(mState));
            return;

            assert(0);
         }
         else
         {
             SharedPtr<SipMessage> p200(new SipMessage);
             mDialog.makeResponse(*p200, msg, 200);
             send(p200);
             transition(UAS_NoAnswerReliable);
             prackCheckQueue();
         }
         break;
      }

      default:
         if(msg.isRequest())
         {
            dispatchUnknown(msg);
         }
         break;
   }
}

void
ServerInviteSession::dispatchOfferReliable(const SipMessage& msg)
{
}

void
ServerInviteSession::dispatchNoOfferReliable(const SipMessage& msg)
{
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);
   switch (toEvent(msg, sdp.get()))
   {
      case OnCancel:
      {
         dispatchCancel(msg);
         break;
      }

      default:
         if(msg.isRequest())
         {
            dispatchUnknown(msg);
         }
         break;
   }
}

void
ServerInviteSession::dispatchFirstSentOfferReliable(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
   {
      case OnCancel:
      {
         dispatchCancel(msg);
         break;
      }

      case OnPrack:
      {
         if(!prackCheckProvisionals(msg))
         {
            return; // prack does not correspond to an unacknowedged provisional
         }
         
         if(sdp.get())  // Answer
         {
            SharedPtr<SipMessage> p200(new SipMessage);
            mDialog.makeResponse(*p200, msg, 200);
            send(p200);

            setCurrentLocalSdp(msg);
            mCurrentRemoteSdp = InviteSession::makeSdp(*sdp);
            mCurrentEncryptionLevel = getEncryptionLevel(msg);
            handler->onAnswer(getSessionHandle(), msg, *sdp);

            if( mState != UAS_Accepted)    // queued 200 OK was sent in prackCheckProvisionals
            {                              // and state was changed there
               transition(UAS_NegotiatedReliable);
            }
           
         }
         else
         {
            mEndReason = IllegalNegotiation;
            transition(Terminated);
            handler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
            SharedPtr<SipMessage> p406(new SipMessage);
            mDialog.makeResponse(*p406, msg, 406);
            send(p406);
            // TODO - something is missing here - we are transitioning to terminated, but we are not destroying ourself
         }
         break;
      }

      default:
         if(msg.isRequest())
         {
            dispatchUnknown(msg);
         }
         break;
   }
}

bool
ServerInviteSession::prackCheckProvisionals(const SipMessage& msg)
{
   std::deque< SharedPtr<SipMessage> >::iterator msgIt = mUnacknowledgedProvisionals.begin();

   InfoLog (<< "prackCheckProvisionals: " << mUnacknowledgedProvisionals.size() );
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;

   while(msgIt!=mUnacknowledgedProvisionals.end())
   {
      if((*msgIt)->header(h_RSeq).value() == msg.header(h_RAck).rSequence()
         && (*msgIt)->header(h_CSeq).sequence() == msg.header(h_RAck).cSequence()
         && (*msgIt)->header(h_CSeq).method() == msg.header(h_RAck).method())
      {
          mUnacknowledgedProvisionals.erase(msgIt);
          handler->onPrack(getHandle(),msg);
          InfoLog (<< "Found matching provisional. Outstanding: " << mUnacknowledgedProvisionals.size() );
          return true;
      }
      msgIt++;
   }

   ErrLog (<< "spurious PRACK in state=" << toData(mState));
   SharedPtr<SipMessage> p481(new SipMessage);
   mDialog.makeResponse(*p481, msg, 481);
   send(p481);
   return false;

}

void
ServerInviteSession::prackCheckQueue()
{
   InfoLog (<< "prackCheckQueue: " << mQueuedProvisionals.size() );
   if(mQueuedProvisionals.size() > 0 && mQueuedProvisionals.front().first < 200)
   {
      InfoLog (<< "Sending queued provisional" );
      sendProvisional(mQueuedProvisionals.front().first, mQueuedProvisionals.front().second);
      mQueuedProvisionals.pop_front();
   }
   else if(mQueuedProvisionals.size() > 0 && mQueuedProvisionals.front().first < 300)
   {
      InfoLog (<< "Sending queued 200 OK" );
      InviteSessionHandler* handler = mDum.mInviteSessionHandler;
      transition(UAS_Accepted);
      Contents* sdp = 0;
      if( !mAnswerSentReliably && mCurrentLocalSdp.get())
      {
         sdp = mCurrentLocalSdp.get();
      }
      sendAccept(mQueuedProvisionals.front().first, sdp);
      handler->onConnected(getSessionHandle(), *mInvite200);
      mQueuedProvisionals.pop_front();
   }

}

void 
ServerInviteSession::dispatchFirstSentAnswerReliable(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
   {
         
      case OnCancel:
      {
         dispatchCancel(msg);
         break;
      }

      case OnPrack:
      {
         if(!prackCheckProvisionals(msg))
         {
            return; // prack does not correspond to an unacknowedged provisional
         }
         
         if(sdp.get())  // New offer
         {
            if(!mAnswerSentReliably)    // new offer before previous answer sent 
            {
               // TODO - reject PRACK, teardown call?
               ErrLog (<< "PRACK with new offer when in state=" << toData(mState));
               return;
               assert(0);
            }
            else
            {
               // dispatch offer here and respond with 200OK in provideAnswer
               transition(UAS_NegotiatedReliable);
               mPrackWithOffer = resip::SharedPtr<SipMessage>(new SipMessage(msg));
               mProposedRemoteSdp = InviteSession::makeSdp(*sdp);
               mCurrentEncryptionLevel = getEncryptionLevel(msg);
               if(!isTerminated())  
               {
                  handler->onOffer(getSessionHandle(), msg, *sdp);
               }
            }
         }
         else
         {
             SharedPtr<SipMessage> p200(new SipMessage);
             mDialog.makeResponse(*p200, msg, 200);
             send(p200);
             if( mState != UAS_Accepted)    // queued 200 OK was sent in prackCheckProvisionals
             {                              // and state was changed there
                transition(UAS_NegotiatedReliable);
             }
             prackCheckQueue();
         }
         break;
      }

      default:
         if(msg.isRequest())
         {
            dispatchUnknown(msg);
         }
         break;
   }
}

void 
ServerInviteSession::dispatchFirstNoAnswerReliable(const SipMessage& msg)
{
//   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
   {
         
      case OnCancel:
      {
         dispatchCancel(msg);
         break;
      }

      case OnPrack:
      {
         if(!prackCheckProvisionals(msg))
         {
            return; // prack does not correspond to an unacknowedged provisional
         }
         
         if(sdp.get())
         {
            // TODO - reject PRACK, teardown call?
            ErrLog (<< "PRACK with new offer when in state=" << toData(mState));
            return;

            assert(0);
         }
         else
         {
             SharedPtr<SipMessage> p200(new SipMessage);
             mDialog.makeResponse(*p200, msg, 200);
             send(p200);
             transition(UAS_NoAnswerReliable);
             prackCheckQueue();
         }
         break;
      }

      default:
         if(msg.isRequest())
         {
            dispatchUnknown(msg);
         }
         break;
   }


}

void
ServerInviteSession::dispatchFirstEarlyReliable(const SipMessage& msg)
{
}

void
ServerInviteSession::dispatchEarlyReliable(const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
   {
         
      case OnCancel:
      {
         dispatchCancel(msg);
         break;
      }
      case OnPrack:
      {
         if(!prackCheckProvisionals(msg))
         {
            return; // prack does not correspond to an unacknowedged provisional
         }

         if(mAnswerSentReliably)
         {
            transition(UAS_NegotiatedReliable);
         }

         if(sdp.get())
         {
            if(!mAnswerSentReliably)    // new offer before previous answer sent 
            {
                // TODO - reject PRACK, teardown call?
                ErrLog (<< "PRACK with new offer when in state=" << toData(mState));
                return;
                assert(0);
            }
            else
            {
               // dispatch offer here and respond with 200OK in provideAnswer
               mPrackWithOffer = resip::SharedPtr<SipMessage>(new SipMessage(msg));
               mProposedRemoteSdp = InviteSession::makeSdp(*sdp);
               mCurrentEncryptionLevel = getEncryptionLevel(msg);
               if(!isTerminated())  
               {
                  handler->onOffer(getSessionHandle(), msg, *sdp);
               }
            }
         }
         else
         {
             SharedPtr<SipMessage> p200(new SipMessage);
             mDialog.makeResponse(*p200, msg, 200);
             send(p200);
             prackCheckQueue();
         }
         break;
      }

      default:
         if(msg.isRequest())
         {
            dispatchUnknown(msg);
         }
         break;
   }
}

void
ServerInviteSession::dispatchSentUpdate(const SipMessage& msg)
{
   // TODO
   // on2xxU -> NegotiatedReliable
}

void
ServerInviteSession::dispatchSentUpdateAccepted(const SipMessage& msg)
{
   // TODO
   // on2xxU/app::onAnswer
   // on2xxU/app::onOfferRejected
}

void
ServerInviteSession::dispatchReceivedUpdate(const SipMessage& msg)
{
}

void
ServerInviteSession::dispatchReceivedUpdateWaitingAnswer(const SipMessage& msg)
{
}

void
ServerInviteSession::dispatchWaitingToTerminate(const SipMessage& msg)
{
}

void
ServerInviteSession::dispatchWaitingToHangup(const SipMessage& msg)
{
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
   {
      case OnPrack:
      case OnAck:
      case OnAckAnswer:
      {
         mCurrentRetransmit200 = 0; // stop the 200 retransmit timer

         sendBye();
         transition(Terminated);
         mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::LocalBye);
         break;
      }
      
      default:
         break;
   }
}

void
ServerInviteSession::dispatchCancel(const SipMessage& msg)
{
   SharedPtr<SipMessage> c200(new SipMessage);
   mDialog.makeResponse(*c200, msg, 200);
   send(c200);

   SharedPtr<SipMessage> i487(new SipMessage);
   mDialog.makeResponse(*i487, mFirstRequest, 487);
   send(i487);

   transition(Terminated);

   if (mDum.mDialogEventStateManager)
   {
      mDum.mDialogEventStateManager->onTerminated(mDialog, msg, InviteSessionHandler::RemoteCancel);
   }

   mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::RemoteCancel, &msg);
   mDum.destroy(this);
}

void
ServerInviteSession::dispatchBye(const SipMessage& msg)
{
   SharedPtr<SipMessage> b200(new SipMessage);
   mDialog.makeResponse(*b200, msg, 200);
   send(b200);
// !dcm! -- pretty sure we shouldn't 487 after the BYE/200
   SharedPtr<SipMessage> i487(new SipMessage);
   mDialog.makeResponse(*i487, mFirstRequest, 487);
   send(i487);

   transition(Terminated);

   mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::RemoteBye, &msg);
   mDum.destroy(this);
}

void
ServerInviteSession::dispatchUnknown(const SipMessage& msg)
{
   SharedPtr<SipMessage> r481(new SipMessage); // !jf! what should we send here? 
   mDialog.makeResponse(*r481, msg, 481);
   send(r481);
   
   SharedPtr<SipMessage> i400(new SipMessage);
   mDialog.makeResponse(*i400, mFirstRequest, 400);
   send(i400);

   transition(Terminated);
   mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), InviteSessionHandler::Error, &msg);
   mDum.destroy(this);
}

void 
ServerInviteSession::startRetransmit1xxTimer()
{
   // RFC3261 13.3.1 says the UAS must send a non-100 provisional response every minute, to handle the possiblity of lost provisional responses
   mCurrentRetransmit1xx = mDialog.mDialogSet.getUserProfile()->get1xxRetransmissionTime();  
   if(mCurrentRetransmit1xx > 0)
   {	
      unsigned int seq = m1xx->header(h_CSeq).sequence();
      mDum.addTimer(DumTimeout::Retransmit1xx, mCurrentRetransmit1xx, getBaseHandle(), seq);
   }
}

void 
ServerInviteSession::startRetransmit1xxRelTimer()
{
   unsigned int seq = m1xx->header(h_RSeq).value();
   mDum.addTimerMs(DumTimeout::Retransmit1xxRel, Timer::T1, getBaseHandle(), seq, Timer::T1);
}

void
ServerInviteSession::sendProvisional(int code, bool earlyFlag)
{
   m1xx->releaseContents();
   mDialog.makeResponse(*m1xx, mFirstRequest, code);
   switch (mState)
   {
      case UAS_OfferProvidedAnswer:
      case UAS_EarlyProvidedAnswer:
      case UAS_FirstSentAnswerReliable:
      case UAS_NoAnswerReliable:
         if (earlyFlag && !mAnswerSentReliably && mCurrentLocalSdp.get()) // early media
         {
            setSdp(*m1xx, mCurrentLocalSdp.get());
            if (provisionalWillBeSentReliable())
            {
                mAnswerSentReliably = true;
            }
         }
         break;

      case UAS_NoOfferReliable:
         if (code>100 && earlyFlag && !mAnswerSentReliably && mProposedLocalSdp.get()) // early media
         {
            setSdp(*m1xx, mProposedLocalSdp.get());
            mAnswerSentReliably = true;
            transition(UAS_FirstSentOfferReliable);
         }
         break;

      case UAS_ProvidedOffer:
      case UAS_ProvidedOfferReliable:
      case UAS_EarlyProvidedOffer:
         if (earlyFlag && mProposedLocalSdp.get()) 
         {
            setSdp(*m1xx, mProposedLocalSdp.get());
         }
         break;

      default:
         break;
   }
   startRetransmit1xxTimer();
   DumHelper::setOutgoingEncryptionLevel(*m1xx, mProposedEncryptionLevel);

   if (mDum.mDialogEventStateManager)
   {
      mDum.mDialogEventStateManager->onEarly(mDialog, getSessionHandle());
   }

   if(m1xx->exists(h_Requires) && m1xx->header(h_Requires).find(Token(Symbols::C100rel)))
   {
      mUnacknowledgedProvisionals.push_back(m1xx);
      startRetransmit1xxRelTimer();
   }

   send(m1xx);
}

void
ServerInviteSession::queueReliableProvisional(int code, bool earlyFlag)
{
   InfoLog (<< "Reliable provisional queued" );
   mQueuedProvisionals.push_back( std::make_pair(code,earlyFlag) );
    
}

void
ServerInviteSession::sendAccept(int code, Contents* sdp)
{
   mDialog.makeResponse(*mInvite200, mFirstRequest, code);
   handleSessionTimerRequest(*mInvite200, mFirstRequest);
   if (sdp && !mAnswerSentReliably )
   {
      setSdp(*mInvite200, sdp);
      mAnswerSentReliably = true;
   }
   mCurrentRetransmit1xx = 0; // Stop the 1xx timer
   startRetransmit200Timer(); // 2xx timer
   DumHelper::setOutgoingEncryptionLevel(*mInvite200, mCurrentEncryptionLevel);

   if (mDum.mDialogEventStateManager)
   {
      mDum.mDialogEventStateManager->onConfirmed(mDialog, getSessionHandle());
   }

   send(mInvite200);
}

void
ServerInviteSession::sendUpdate(const SdpContents& sdp)
{
   if (updateMethodSupported())
   {
      mDialog.makeRequest(*mLastLocalSessionModification, UPDATE);
      InviteSession::setSdp(*mLastLocalSessionModification, sdp);
      DumHelper::setOutgoingEncryptionLevel(*mLastLocalSessionModification, mProposedEncryptionLevel);
      send(mLastLocalSessionModification);
   }
   else
   {
      throw UsageUseException("Can't send UPDATE to peer", __FILE__, __LINE__);
   }
}

bool 
ServerInviteSession::provisionalWillBeSentReliable()
{
   return isReliable(mFirstRequest);
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
