#include "resiprocate/SdpContents.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/DumTimeout.hxx"
#include "resiprocate/dum/MasterProfile.hxx"
#include "resiprocate/dum/ServerInviteSession.hxx"
#include "resiprocate/dum/ServerSubscription.hxx"
#include "resiprocate/dum/UsageUseException.hxx"
#include "resiprocate/SipFrag.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

ClientInviteSession::ClientInviteSession(DialogUsageManager& dum,
                                         Dialog& dialog,
                                         const SipMessage& request,
                                         const SdpContents* initialOffer,
                                         ServerSubscriptionHandle serverSub) :
   InviteSession(dum, dialog),
   mLastReceivedRSeq(-1),
   mStaleCallTimerSeq(1),
   mCancelledTimerSeq(1),
   mServerSub(serverSub)
{
   assert(request.isRequest());
   mProposedLocalSdp = InviteSession::makeSdp(*initialOffer);
   mInvite = request;

   mState=UAC_Start;
}

ClientInviteSessionHandle
ClientInviteSession::getHandle()
{
   return ClientInviteSessionHandle(mDum, getBaseHandle().getId());
}

const SdpContents&
ClientInviteSession::getEarlyMedia() const
{
   return *mEarlyMedia;
}

void
ClientInviteSession::provideOffer (const SdpContents& offer)
{
   InfoLog (<< toData(mState) << ": provideOffer");

   switch(mState)
   {
      case UAC_EarlyWithAnswer:
      {
         transition(UAC_SentUpdateEarly);

         //  Creates an UPDATE request with application supplied offer.
         SipMessage req;
         mDialog.makeRequest(req, UPDATE);
         InviteSession::setSdp(req, offer);

         //  Remember last seesion modification.
         mLastSessionModification = req;

         //  Remember proposed local SDP.
         mProposedLocalSdp = InviteSession::makeSdp(offer);

         //  Send the req and do state transition.
         mDialog.send(req);
         break;
      }

      case UAC_SentAnswer:
         // just queue it for later
         transition(UAC_QueuedUpdate);
         mProposedLocalSdp = InviteSession::makeSdp(offer);
         break;

      case UAC_Start:
      case UAC_Early:
      case UAC_EarlyWithOffer:
      case UAC_Answered:
      case UAC_SentUpdateEarly:
      case UAC_ReceivedUpdateEarly:
      case UAC_Cancelled:
         assert(0);
         break;

      default:
         InviteSession::provideOffer(offer);
         break;
   }
}

void
ClientInviteSession::provideAnswer (const SdpContents& answer)
{
   InfoLog (<< toData(mState) << ": provideAnswer");

   switch(mState)
   {
      case UAC_EarlyWithOffer:
      {
         transition(UAC_SentAnswer);

         //  Remember proposed local SDP.
         mCurrentRemoteSdp = mProposedRemoteSdp;
         mCurrentLocalSdp = InviteSession::makeSdp(answer);

         //  Creates an PRACK request with application supplied offer.
         sendPrack(answer);
         break;
      }

      case UAC_Answered:
      {
         transition(Connected);

         mCurrentRemoteSdp = mProposedRemoteSdp;
         mCurrentLocalSdp = InviteSession::makeSdp(answer);

         SipMessage ack;
         mDialog.makeRequest(ack, ACK);
         InviteSession::setSdp(ack, answer);

         mLastSessionModification = ack;
         mDialog.send(ack);
         break;
      }


      case UAC_Start:
      case UAC_Early:
      case UAC_EarlyWithAnswer:
      case UAC_SentUpdateEarly:
      case UAC_ReceivedUpdateEarly:
      case UAC_SentAnswer:
      case UAC_Cancelled:
         assert(0);
         break;

      default:
         InviteSession::provideAnswer(answer);
         break;
   }
}

void
ClientInviteSession::end()
{
   InfoLog (<< toData(mState) << ": end");

   switch(mState)
   {
      case UAC_Early:
      case UAC_EarlyWithOffer:
      case UAC_EarlyWithAnswer:
      case UAC_Answered:
      case UAC_SentUpdateEarly:
      case UAC_ReceivedUpdateEarly:
      case UAC_SentAnswer:
      case UAC_Cancelled: // !jf! possibly incorrect to always BYE in UAC_Cancelled
      {
         transition(Terminated);
         SipMessage bye;
         mDialog.makeRequest(bye, BYE);
         mDialog.send(bye);
         break;
      }

      case UAC_Start:
         WarningLog (<< "Try to end when in state=" << toData(mState));
         assert(0);
         break;

      case Terminated:
         assert(0);
         break;

      default:
         InviteSession::end();
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
         //  Creates an PRACK request with application supplied status code.
         //  !kh! hopefully 488....
         SipMessage req;
         mDialog.makeRequest(req, PRACK);
         req.header(h_StatusLine).statusCode() = statusCode;
         if(warning)
         {
            req.header(h_Warnings).push_back(*warning);
         }

         //  Send the req and do state transition.
         mDialog.send(req);
         transition(UAC_EarlyWithAnswer);
         break;
      }

      case UAC_Start:
      case UAC_Early:
      case UAC_EarlyWithOffer:
      case UAC_EarlyWithAnswer:
      case UAC_Answered:
      case UAC_SentUpdateEarly:
      case UAC_SentAnswer:
      case UAC_Cancelled:
         WarningLog (<< "Try to reject when in state=" << toData(mState));
         assert(0);
         break;

      default:
         InviteSession::reject(statusCode);
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
         // !jf!
         // assert(0);
         break;

      default:
         assert(0);
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
            if (code < 200)
            {
               mServerSub->send(mServerSub->update(&contents));
            }
            else
            {
               contents.message().header(h_StatusLine) = msg.header(h_StatusLine);
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
	case UAC_SentUpdateConnected:
	   dispatchSentUpdateConnected(msg);
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
     end();
     InviteSessionHandler* handler = mDum.mInviteSessionHandler;
     handler->onTerminated(getSessionHandle(),
			   InviteSessionHandler::GeneralFailure);
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
            mDialog.makeResponse(response, mInvite, 487);
            sendSipFrag(response);
         }
         InviteSessionHandler* handler = mDum.mInviteSessionHandler;
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Cancelled);
         mDum.destroy(this);
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
   handler->onRedirected(getHandle(), msg);
   mDum.destroy(this);
}

void
ClientInviteSession::handleProvisional(const SipMessage& msg)
{
   assert(msg.isResponse());
   assert(msg.header(h_StatusLine).statusCode() < 200);
   assert(msg.header(h_StatusLine).statusCode() > 100);

   InviteSessionHandler* handler = mDum.mInviteSessionHandler;

   // must match
   if (msg.header(h_CSeq).sequence() != mInvite.header(h_CSeq).sequence())
   {
      InfoLog (<< "CSeq doesn't match invite" << msg.brief());
      handler->onFailure(getHandle(), msg);
      end();
   }
   else if (isReliable(msg))
   {
      if (!msg.exists(h_RSeq))
      {
         InfoLog (<< "No RSeq in 1xx " << msg.brief());
         handler->onFailure(getHandle(), msg);
         end();
      }
      else
      {
         // store state about the provisional if reliable, so we can detect retransmissions
         int rseq = msg.header(h_RSeq).value();
         if ( (mLastReceivedRSeq == -1) || (rseq == mLastReceivedRSeq+1))
         {
            mLastReceivedRSeq = rseq;
            InfoLog (<< "Got a reliable 1xx with rseq = " << rseq);
            handler->onProvisional(getHandle(), msg);
         }
         else
         {
            InfoLog (<< "Got an out of order reliable 1xx with rseq = " << rseq << " dropping");
         }
      }
   }
   else
   {
      handler->onProvisional(getHandle(), msg);
   }
}

void
ClientInviteSession::handleOffer (const SipMessage& msg, const SdpContents& sdp)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;

   handleProvisional(msg);
   mProposedRemoteSdp = InviteSession::makeSdp(sdp);
   handler->onOffer(getSessionHandle(), msg, sdp);
}

void
ClientInviteSession::handleAnswer (const SipMessage& msg, const SdpContents& sdp)
{
   mCurrentLocalSdp = mProposedLocalSdp;
   mCurrentRemoteSdp = InviteSession::makeSdp(sdp);

   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   handleProvisional(msg);
   handler->onAnswer(getSessionHandle(), msg, sdp);

   sendPrackIfNeeded(msg);
}

// will not include SDP (this is a subsequent 1xx)
void
ClientInviteSession::sendPrackIfNeeded(const SipMessage& msg)
{
   if ( isReliable(msg) &&
        (mLastReceivedRSeq == -1 || msg.header(h_RSeq).value() == mLastReceivedRSeq+1))
   {
      SipMessage prack;
      mDialog.makeRequest(prack, PRACK);
      prack.header(h_RSeq) = msg.header(h_RSeq);
      mDialog.send(prack);
   }
}

// This version is used to send an answer to the UAS in PRACK
// from EarlyWithOffer state. Assumes that it is the first PRACK. Subsequent
// PRACK will not have SDP
void
ClientInviteSession::sendPrack(const SdpContents& sdp)
{
   SipMessage prack;
   mDialog.makeRequest(prack, PRACK);
   prack.header(h_RSeq).value() = mLastReceivedRSeq;
   InviteSession::setSdp(prack, sdp);

   //  Remember last session modification.
   mLastSessionModification = prack;

   mDialog.send(prack);
}


/*
bool
ClientInviteSession::isNextProvisional(const SipMessage& msg)
{
}

bool
ClientInviteSession::isRetransmission(const SipMessage& msg)
{
   if ( mLastReceivedRSeq == -1 ||
        msg.header(h_RSeq).value() <= mLastReceivedRSeq)
   {
      return false;
   }
   else
   {
      return true;
   }
}
*/

void
ClientInviteSession::dispatchStart (const SipMessage& msg)
{
   assert(msg.isResponse());
   assert(msg.header(h_StatusLine).statusCode() > 100);
   assert(msg.header(h_CSeq).method() == INVITE);

   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
   {
      case OnRedirect:
         handleRedirect(msg);
         break;

      case On1xx:
         transition(UAC_Early);
         handler->onNewSession(getHandle(), None, msg);
         handler->onProvisional(getHandle(), msg);
         break;

      case On1xxEarly:
         // !jf! Assumed that if UAS supports 100rel, the first 1xx must contain an
         // offer or an answer.
         transition(UAC_Early);
         mEarlyMedia = InviteSession::makeSdp(*sdp);
         handler->onNewSession(getHandle(), None, msg);
         handler->onProvisional(getHandle(), msg);
         handler->onEarlyMedia(getHandle(), msg, *sdp);
         break;

      case On1xxOffer:
         transition(UAC_EarlyWithOffer);
         handler->onNewSession(getHandle(), Offer, msg);
         handleOffer(msg, *sdp);
         break;

      case On1xxAnswer:
         transition(UAC_EarlyWithAnswer);
         handler->onNewSession(getHandle(), Answer, msg);
         handleAnswer(msg, *sdp);
         break;

      case On2xxOffer:
         transition(UAC_Answered);
         handleSessionTimerResponse(msg);
         storePeerCapabilities(msg);
         mProposedRemoteSdp = InviteSession::makeSdp(*sdp);
         handler->onNewSession(getHandle(), Offer, msg);
         assert(mProposedLocalSdp.get() == 0);
         handler->onOffer(getSessionHandle(), msg, *sdp);
         handler->onConnected(getHandle(), msg);
         break;

      case On2xxAnswer:
         transition(Connected);
         {
            SipMessage ack;
            mDialog.makeRequest(ack, ACK);
            mDialog.send(ack);
         }
         handleSessionTimerResponse(msg);
         storePeerCapabilities(msg);
         mCurrentLocalSdp = mProposedLocalSdp;
         mCurrentRemoteSdp = InviteSession::makeSdp(*sdp);
         handler->onNewSession(getHandle(), Answer, msg);
         handler->onAnswer(getSessionHandle(), msg, *sdp);
         handler->onConnected(getHandle(), msg);
         break;

      case On2xx:
      {
         transition(Terminated);

         SipMessage ack;
         mDialog.makeRequest(ack, ACK);
         mDialog.send(ack);

         SipMessage bye;
         mDialog.makeRequest(bye, BYE);
         mDialog.send(bye);

         handler->onFailure(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure);
         break;
      }

      case OnInviteFailure:
      case OnGeneralFailure:
         transition(Terminated);
         handler->onFailure(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure);
         mDum.destroy(this);
         break;

      default:
         assert(0);
         break;
   }
}

void
ClientInviteSession::dispatchEarly (const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
   {
      case On1xx:
         transition(UAC_Early);
         handler->onProvisional(getHandle(), msg);
         break;

      case On1xxEarly: // only unreliable
         transition(UAC_Early);
         handler->onProvisional(getHandle(), msg);
         mEarlyMedia = InviteSession::makeSdp(*sdp);
         handler->onEarlyMedia(getHandle(), msg, *sdp);
         break;

      case On1xxOffer:
         transition(UAC_EarlyWithOffer);
         handler->onNewSession(getHandle(), Offer, msg);
         handleOffer(msg, *sdp);
         break;

      case On1xxAnswer:
         transition(UAC_EarlyWithAnswer);
         handleAnswer(msg, *sdp);
         break;

      case On2xxOffer:
         transition(UAC_Answered);
         handleSessionTimerResponse(msg);
         storePeerCapabilities(msg);

         assert(mProposedLocalSdp.get() == 0);
         mProposedRemoteSdp = InviteSession::makeSdp(*sdp);

         handler->onOffer(getSessionHandle(), msg, *sdp);
         handler->onConnected(getHandle(), msg);
         break;

      case On2xxAnswer:
         transition(Connected);
         {
            SipMessage ack;
            mDialog.makeRequest(ack, ACK);
            mDialog.send(ack);
         }
         handleSessionTimerResponse(msg);
         storePeerCapabilities(msg);
         mCurrentLocalSdp = mProposedLocalSdp;
         mCurrentRemoteSdp = InviteSession::makeSdp(*sdp);
         handler->onAnswer(getSessionHandle(), msg, *sdp);
         handler->onConnected(getHandle(), msg);
         break;

      case On2xx:
      {
         transition(Terminated);

         SipMessage ack;
         mDialog.makeRequest(ack, ACK);
         mDialog.send(ack);

         SipMessage bye;
         mDialog.makeRequest(bye, BYE);
         mDialog.send(bye);

         handler->onFailure(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure);
         break;
      }

      case OnRedirect:
         handleRedirect(msg);
         break;

      case OnInviteFailure:
      case OnGeneralFailure:
         handler->onFailure(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure);
         mDum.destroy(this);
         break;

      default:
         assert(0);
         break;
   }
}

void
ClientInviteSession::dispatchAnswered (const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
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

      case OnGeneralFailure:
      {
         SipMessage bye;
         mDialog.makeRequest(bye, BYE);
         mDialog.send(bye);

         handler->onFailure(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure);
         mDum.destroy(this);
         break;
      }

      default:
         assert(0);
         break;
   }

}

void
ClientInviteSession::dispatchEarlyWithOffer (const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
   {
      case On1xx: // must be reliable
         handleProvisional(msg);
         sendPrackIfNeeded(msg);
         break;

      case On2xx:
      case On2xxAnswer:
         transition(Terminated);
         handler->onFailure(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure);
         {
            SipMessage ack;
            mDialog.makeRequest(ack, ACK);
            mDialog.send(ack);

            SipMessage bye;
            mDialog.makeRequest(bye, BYE);
            mDialog.send(bye);
         }
         break;

      case OnRedirect:
         handleRedirect(msg);
         break;

      case OnInviteFailure:
      case OnGeneralFailure:
         handler->onFailure(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure);
         mDum.destroy(this);
         break;

      default:
         assert(0);
         break;
   }
}

void
ClientInviteSession::dispatchSentAnswer (const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
   {
      case On200Prack:
         transition(UAC_EarlyWithAnswer);
         break;

      case On2xx:
         transition(Connected);
         {
            SipMessage ack;
            mDialog.makeRequest(ack, ACK);
            mDialog.send(ack);
         }
         handleSessionTimerResponse(msg);
         storePeerCapabilities(msg);
         handler->onConnected(getHandle(), msg);
         break;

      case On2xxAnswer:
      case On2xxOffer:
      case On1xxAnswer:
      case On1xxOffer:
         transition(Terminated);
         {
            SipMessage ack;
            mDialog.makeRequest(ack, ACK);
            mDialog.send(ack);

            SipMessage bye;
            mDialog.makeRequest(bye, BYE);
            mDialog.send(bye);
         }
         handler->onFailure(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure);
         break;

      case On1xx:
         handleProvisional(msg);
         sendPrackIfNeeded(msg);
         break;

      case OnRedirect:
         handleRedirect(msg);
         break;

      case OnInviteFailure:
      case OnGeneralFailure:
         handler->onFailure(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure);
         mDum.destroy(this);
         break;

      default:
         assert(0);
         break;
   }
}

void
ClientInviteSession::dispatchQueuedUpdate (const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
   {
      case On200Prack:
         transition(UAC_SentUpdateEarly);
         {
            SipMessage update;
            mDialog.makeRequest(update, UPDATE);
            InviteSession::setSdp(update, *mProposedLocalSdp);

            //  Remember last seesion modification.
            mLastSessionModification = update;

            mDialog.send(update);
         }
         break;

      case On2xx:
         transition(SentUpdate);
         {
            SipMessage ack;
            mDialog.makeRequest(ack, ACK);
            mDialog.send(ack);

            SipMessage update;
            mDialog.makeRequest(update, UPDATE);
            InviteSession::setSdp(update, *mProposedLocalSdp);
            mDialog.send(update);
         }
         handleSessionTimerResponse(msg);
         storePeerCapabilities(msg);
         handler->onConnected(getHandle(), msg);
         break;

      case On2xxAnswer:
      case On2xxOffer:
      case On1xxAnswer:
      case On1xxOffer:
         transition(Terminated);
         {
            SipMessage ack;
            mDialog.makeRequest(ack, ACK);
            mDialog.send(ack);

            SipMessage bye;
            mDialog.makeRequest(bye, BYE);
            mDialog.send(bye);
         }
         handler->onFailure(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure);
         break;

      case On1xx:
         handleProvisional(msg);
         sendPrackIfNeeded(msg);
         break;

      case OnRedirect:
         handleRedirect(msg);
         break;

      case OnInviteFailure:
      case OnGeneralFailure:
         handler->onFailure(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure);
         mDum.destroy(this);
         break;

      default:
         assert(0);
         break;
   }
}



void
ClientInviteSession::dispatchEarlyWithAnswer (const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
   {
      case On1xx:
         handler->onProvisional(getHandle(), msg);
         sendPrackIfNeeded(msg);
         break;

      case On2xx:
         transition(Connected);
         {
            SipMessage ack;
            mDialog.makeRequest(ack, ACK);
            mDialog.send(ack);
         }
         handleSessionTimerResponse(msg);
         storePeerCapabilities(msg);
         handler->onConnected(getHandle(), msg);
         break;

      case On2xxAnswer:
      case On2xxOffer:
         transition(Terminated);
         {
            SipMessage ack;
            mDialog.makeRequest(ack, ACK);
            mDialog.send(ack);

            SipMessage bye;
            mDialog.makeRequest(bye, BYE);
            mDialog.send(bye);
         }
         handler->onFailure(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure);
         break;

      case OnUpdate:
         transition(UAC_ReceivedUpdateEarly);
         handler->onOffer(getSessionHandle(), msg, *sdp);
         break;

      case OnRedirect:
         handleRedirect(msg);
         break;

      case OnInviteFailure:
      case OnGeneralFailure:
         handler->onFailure(getHandle(), msg);
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::GeneralFailure);
         mDum.destroy(this);
         break;

      default:
         assert(0);
         break;
   }
}

void
ClientInviteSession::dispatchSentUpdateEarly (const SipMessage& msg)
{
   assert(0);
}

void
ClientInviteSession::dispatchSentUpdateConnected (const SipMessage& msg)
{
   assert(0);
}

void
ClientInviteSession::dispatchReceivedUpdateEarly (const SipMessage& msg)
{
   assert(0);
}

void
ClientInviteSession::dispatchCancelled (const SipMessage& msg)
{
   InviteSessionHandler* handler = mDum.mInviteSessionHandler;
   std::auto_ptr<SdpContents> sdp = InviteSession::getSdp(msg);

   switch (toEvent(msg, sdp.get()))
   {
      case OnGeneralFailure:
      case OnCancelFailure:
      case On487Invite:
      case OnRedirect:
         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Cancelled, &msg);
         mDum.destroy(this);
         break;

      case On2xx:
      case On2xxOffer:
      case On2xxAnswer:
      {
         // this is the 2xx crossing the CANCEL case
         SipMessage ack;
         mDialog.makeRequest(ack, ACK);
         mDialog.send(ack);

         handler->onTerminated(getSessionHandle(), InviteSessionHandler::Cancelled, &msg);
         mCancelledTimerSeq++;
         end();
         break;
      }

      default:
         break;
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
