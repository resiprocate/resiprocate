#include "B2BSession.hxx"
#include "Server.hxx"
#include "AppSubsystem.hxx"

#include <resip/stack/PlainContents.hxx>
#include <resip/stack/SdpContents.hxx>
#include <resip/stack/SipFrag.hxx>
#include <resip/dum/ServerInviteSession.hxx>
#include <resip/dum/ClientInviteSession.hxx>
#include <resip/dum/ClientSubscription.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace clicktocall;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::CLICKTOCALL

#ifdef BRIEF_MSG_LOGGING
#define LOG_MSG msg.brief()
#define LOG_MSG_WITH_SDP msg.brief() << ", sdp=" << sdp
#define LOG_MSGP msg->brief()
#else
#define LOG_MSG endl << msg
#define LOG_MSG_WITH_SDP endl << msg
#define LOG_MSGP endl << *msg
#endif
#define B2BLOG_PREFIX << "B2BSession[" << mHandle << "] "

namespace clicktocall 
{

B2BSession::B2BSession(Server& server) : 
   AppDialogSet(server.getDialogUsageManager()), 
   mServer(server), 
   mDum(server.getDialogUsageManager()),
   mPeer(0),
   mUACConnectedDialogId(Data::Empty, Data::Empty, Data::Empty),
   mWaitingOfferFromPeer(false),
   mWaitingAnswerFromPeer(false),
   mWaitingNitAnswerFromPeer(false),
   mClickToCallState(Undefined),
   mClickToCallAnchorEnabled(false),
   mClickToCallInitiator(false)
{
   mHandle = mServer.registerB2BSession(this);
}

B2BSession::~B2BSession()
{
   endPeer();
   mServer.unregisterB2BSession(mHandle);
}

void 
B2BSession::setPeer(B2BSession* peer)
{
   mPeer = peer;
}

B2BSession* 
B2BSession::getPeer()
{
   return mPeer;
}

void 
B2BSession::stealPeer(B2BSession* victimSession)
{
   // Assume Peer mapping of victim - and copy some settings
   setPeer(victimSession->getPeer());
   if(mPeer)
   {
      mPeer->setPeer(this);
   }

   // Clear peer mapping in victim session
   victimSession->setPeer(0);
}

void 
B2BSession::startCall(const Uri& destinationUri, const NameAddr& from, const SdpContents *sdp)
{
   // Create a UserProfile for new call
   SharedPtr<UserProfile> userProfile(new UserProfile(mServer.getMasterProfile()));

   // Set the From address
   userProfile->setDefaultFrom(from);  
   userProfile->getDefaultFrom().remove(p_tag);  // Remove tag (if it exists)

   // Create the invite message
   SharedPtr<SipMessage> invitemsg = mDum.makeInviteSession(
      NameAddr(destinationUri), 
      userProfile,
      sdp, 
      this);

   // Send the invite message
   mDum.send(invitemsg);
}

void
B2BSession::clickToCall(const resip::Uri& initiator, const resip::Uri& destination, bool anchorCall, const XmlRpcInfo& xmlRpcInfo)
{
   resip_assert(mClickToCallState == Undefined);

   // Store Click-To-Call request info
   mClickToCallInitiator = true;
   mClickToCallDestination = destination;
   mClickToCallAnchorEnabled = anchorCall;
   mXmlRpcInfo = xmlRpcInfo;
   transitionClickToCallState(Setup);

   // Create local sdp offer
   SdpContents sdp;
   if(!buildLocalOffer(sdp))
   {
      ErrLog(<< "B2BSession::clickToCall: unable to build local offer.");
      return;
   }

   NameAddr from = mServer.getMasterProfile()->getDefaultFrom();
   from.displayName() = "Click-To-Call: " + destination.user();
   startCall(initiator, from, &sdp);
}

void 
B2BSession::startClickToCallAnchorLeg(const NameAddr& initiator, const Uri& destination, const XmlRpcInfo& xmlRpcInfo)
{
   // Create B2BCall
   InfoLog(B2BLOG_PREFIX << "B2BSession::startClickToCallAnchorLeg: Starting second leg of click to call to " << destination);

   mXmlRpcInfo = xmlRpcInfo;
   transitionClickToCallState(Setup);
   startCall(destination, initiator, 0);
}

bool 
B2BSession::buildLocalOffer(SdpContents& offer)
{
   // Build s=, o=, t=, and c= lines
   UInt64 currentTime = Timer::getTimeMicroSec();

   unsigned int port=8000;  // Placeholder port

   // Note:  The outbound decorator will take care of filling in the correct IP address before the message is sent 
   //        to the wire.
   SdpContents::Session::Origin origin("clicktocall", currentTime /* sessionId */, currentTime /* version */, SdpContents::IP4, "0.0.0.0");   // o=   
   SdpContents::Session session(0, origin, "-" /* s= */);
   session.connection() = SdpContents::Session::Connection(SdpContents::IP4, "0.0.0.0");  // c=
   session.addTime(SdpContents::Session::Time(0, 0));

   // Build Codecs and media offering
   SdpContents::Session::Medium medium("audio", port, 1, "RTP/AVP");
   SdpContents::Session::Codec g711ucodec("PCMU", 8000);
   g711ucodec.payloadType() = 0;  /* RFC3551 */ ;
   medium.addCodec(g711ucodec);
   medium.addAttribute("inactive");
   session.addMedium(medium);

   offer.session() = session;
   return true;
}

void 
B2BSession::endPeer()
{
   if(mPeer)
   {
      mPeer->setPeer(0);
      mPeer->end();  // send cancel or bye appropriately
      mPeer = 0;
   }
}

bool 
B2BSession::isUACConnected()
{
   return !mUACConnectedDialogId.getCallId().empty();
}

bool 
B2BSession::isStaleFork(const DialogId& dialogId)
{
   return (!mUACConnectedDialogId.getCallId().empty() && dialogId != mUACConnectedDialogId);
}

void
B2BSession::processReferNotify(const SipMessage& notify)
{
   unsigned int code = 400;  // Bad Request - default if for some reason a valid sipfrag is not present

   SipFrag* frag  = dynamic_cast<SipFrag*>(notify.getContents());
   if (frag)
   {
      // Get StatusCode from SipFrag
      if (frag->message().isResponse())
      {
         code = frag->message().header(h_StatusLine).statusCode();
      }
   }

   // If subscription is terminated, due to timeout and we don't have a final response in the SipFrag - treat as error
   if(notify.exists(h_SubscriptionState) && 
      isEqualNoCase(notify.header(h_SubscriptionState).value(), Symbols::Terminated) &&
      notify.header(h_SubscriptionState).exists(p_reason) &&
      isEqualNoCase(notify.header(h_SubscriptionState).param(p_reason), getTerminateReasonString(Timeout)) &&
      code < 200)
   {
      // failure
      transitionClickToCallState(ReferToDestinationFailed, 408);
      end();
   }
   // Check if success or failure response code was in SipFrag
   else if(code >= 180 && code < 200)
   {
      // proceeding
      transitionClickToCallState(ReferToDestinationProceeding);
   }
   else if(code >= 200 && code < 300)
   {
      // success
      transitionClickToCallState(ReferToDestinationConnected);
      end();
   }
   else if(code >= 300)
   {
      // failure
      transitionClickToCallState(ReferToDestinationFailed, code);
      end();
   }
}

void 
B2BSession::transitionClickToCallState(ClickToCallState newState, unsigned int statusCode)
{
   if(newState != mClickToCallState)
   {
      unsigned int resultCode = 0;
      Data resultText;
      Data leg = mClickToCallInitiator ? "Initiator" : "Destination";
      switch(newState)
      {
      case Setup:
         InfoLog(B2BLOG_PREFIX << "transitionClickToCallState: newState=Setup, initiator=" << mClickToCallInitiator);
         break;
      case Proceeding:
         InfoLog(B2BLOG_PREFIX << "transitionClickToCallState: newState=Proceeding, initiator=" << mClickToCallInitiator);
         resultCode = 180;
         resultText = mClickToCallInitiator ? "Ringing initiator" : "Ringing destination";
         break;
      case Connected:
         InfoLog(B2BLOG_PREFIX << "transitionClickToCallState: newState=Connected, initiator=" << mClickToCallInitiator);
         resultCode = 200;
         resultText = mClickToCallInitiator ? "Connected to initiator" : "Connected to destination";
         break;
      case Failed:
         InfoLog(B2BLOG_PREFIX << "transitionClickToCallState: newState=Failed, initiator=" << mClickToCallInitiator);
         resultCode = statusCode;
         resultText = mClickToCallInitiator ? "Failed to connect to initiator" : "Failed to connect to destination";
         break;
      case ReferToDestinationProceeding:
         InfoLog(B2BLOG_PREFIX << "transitionClickToCallState: newState=ReferToDestinationProceeding, initiator=" << mClickToCallInitiator);
         leg = "Destination";
         resultCode = 180;
         resultText = "Ringing destination";
         break;
      case ReferToDestinationConnected:
         InfoLog(B2BLOG_PREFIX << "transitionClickToCallState: newState=ReferToDestinationConnected, initiator=" << mClickToCallInitiator);
         leg = "Destination";
         resultCode = 200;
         resultText = "Connected to destination";
         break;
      case ReferToDestinationFailed:
         InfoLog(B2BLOG_PREFIX << "transitionClickToCallState: newState=ReferToDestinationFailed, initiator=" << mClickToCallInitiator);
         leg = "Destination";
         resultCode = statusCode;
         resultText = "Failed to connect to destination";
         break;
      default:
         resip_assert(false);
      }
      mClickToCallState = newState;

      if(mXmlRpcInfo.mXmlRpcServer && resultCode != 0)
      {
         mXmlRpcInfo.mXmlRpcServer->sendResponse(mXmlRpcInfo.mConnectionId, mXmlRpcInfo.mRequestId, Data::Empty, resultCode, resultText, leg);
      }
   }
}


////////////////////////////////////////////////////////////////////////////////
// InviteSessionHandler      ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void
B2BSession::onNewSession(ClientInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onNewSession(ClientInviteSessionHandle): msg=" << LOG_MSG);
   mInviteSessionHandle = h->getSessionHandle();
}

void
B2BSession::onNewSession(ServerInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   WarningLog(B2BLOG_PREFIX << "onNewSession(ServerInviteSessionHandle):  ClickToCall server does not accept inbound calls - rejecting.");

   mInviteSessionHandle = h->getSessionHandle();

   h->reject(403 /* Forbidden */);
}

void
B2BSession::onFailure(ClientInviteSessionHandle h, const SipMessage& msg)
{
   // Note:  Teardown of peer is handled in destructor
   InfoLog(B2BLOG_PREFIX << "onFailure: msg=" << LOG_MSG);
}
      
void
B2BSession::onEarlyMedia(ClientInviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   InfoLog(B2BLOG_PREFIX << "onEarlyMedia: msg=" << LOG_MSG_WITH_SDP);
}

void
B2BSession::onProvisional(ClientInviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onProvisional: msg=" << LOG_MSG);

   if(isStaleFork(h->getDialogId())) return;

   if(msg.header(h_StatusLine).responseCode() >= 180 && mClickToCallState == Setup)
   {
      transitionClickToCallState(Proceeding);
   }

   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid())
      {
         ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mPeer->mInviteSessionHandle.get());
         if(sis && !sis->isAccepted())
         {
            sis->provisional(msg.header(h_StatusLine).responseCode());
         }
      }
   }
}

void
B2BSession::onConnected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onConnected: msg=" << LOG_MSG);
   if(!isUACConnected())
   {
      // It is possible in forking scenarios to get multiple 200 responses, if this is 
      // our first 200 response, then this is the leg we accept, store the connected DialogId
      mUACConnectedDialogId = h->getDialogId();

      // Note:  each forked leg will update mInviteSessionHandle (in onNewSession call) - need to set mInviteSessionHandle for final answering leg on 200
      mInviteSessionHandle = h->getSessionHandle();  

      if(mClickToCallState == Setup || mClickToCallState == Proceeding)
      {
         transitionClickToCallState(Connected);
      }
   }
   else
   {
      // We already have a connected leg - end this one with a BYE
      h->end();
   }
}

void
B2BSession::onConnected(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onConnected: msg=" << LOG_MSG);
   if(mClickToCallState == Setup || mClickToCallState == Proceeding)
   {
      transitionClickToCallState(Connected);
   }
}

void
B2BSession::onStaleCallTimeout(ClientInviteSessionHandle h)
{
   InfoLog(B2BLOG_PREFIX << "onStaleCallTimeout:");
   endPeer();
}

void
B2BSession::onTerminated(InviteSessionHandle h, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg)
{
   Data reasonData;
   switch(reason)
   {
   case InviteSessionHandler::RemoteBye:
      reasonData = "received a BYE from peer";
      break;
   case InviteSessionHandler::RemoteCancel:
      reasonData = "received a CANCEL from peer";
      break;
   case InviteSessionHandler::Rejected:
      reasonData = "received a rejection from peer";
      break;
   case InviteSessionHandler::LocalBye:
      reasonData = "ended locally via BYE";
      break;
   case InviteSessionHandler::LocalCancel:
      reasonData = "ended locally via CANCEL";
      break;
   case InviteSessionHandler::Replaced:
      reasonData = "ended due to being replaced";
      break;
   case InviteSessionHandler::Referred:
      reasonData = "ended due to being reffered";
      break;
   case InviteSessionHandler::Error:
      reasonData = "ended due to an error";
      break;
   case InviteSessionHandler::Timeout:
      reasonData = "ended due to a timeout";
      break;
   default:
      resip_assert(false);
      break;
   }

   if(msg)
   {
      InfoLog(B2BLOG_PREFIX << "onTerminated: reason=" << reasonData << ", msg=" << LOG_MSGP);
   }
   else
   {
      InfoLog(B2BLOG_PREFIX << "onTerminated: reason=" << reasonData);
   }

   unsigned int statusCode = 603;
   if(msg)
   {
      if(msg->isResponse())
      {
         statusCode = msg->header(h_StatusLine).responseCode();
      }
   }

   // If this is a referred call and the refer is still around - then switch back to referrer (ie. failed transfer recovery)
   if(mReferringAppDialogSet.isValid() && mPeer)
   {
      B2BSession* session = (B2BSession*)mReferringAppDialogSet.get();
      session->stealPeer(this);
   }

   if(isStaleFork(h->getDialogId())) return;

   // If we have a peer that hasn't been accepted yet - then pass back terminated code, by calling reject now
   if(mPeer && mPeer->mInviteSessionHandle.isValid())
   {
      ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mPeer->mInviteSessionHandle.get());
      if(sis && !sis->isAccepted())
      {
         sis->reject(statusCode);
      }
   }

   if(mClickToCallState == Setup || mClickToCallState == Proceeding)
   {
      transitionClickToCallState(Failed, statusCode);
   }

   endPeer();
}

void
B2BSession::onRedirected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   // We will recurse on redirect requests, so nothing to do here
   InfoLog(B2BLOG_PREFIX << "onRedirected: msg=" << LOG_MSG);
}

void
B2BSession::onAnswer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   InfoLog(B2BLOG_PREFIX << "onAnswer: msg=" << LOG_MSG_WITH_SDP);

   if(isStaleFork(h->getDialogId())) return;

   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid())
      {
         if(mPeer->mWaitingAnswerFromPeer)
         {
            mPeer->mInviteSessionHandle->provideAnswer(sdp);
            mPeer->mWaitingAnswerFromPeer = false;
         }
         else if(mPeer->mWaitingOfferFromPeer)
         {
            mPeer->mInviteSessionHandle->provideOffer(sdp);
            mPeer->mWaitingOfferFromPeer = false;
         }

         ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mPeer->mInviteSessionHandle.get());
         if(sis && !sis->isAccepted())
         { 
            sis->accept();
         }
      }
   }
   else if(mClickToCallInitiator && (mClickToCallState == Setup || mClickToCallState == Proceeding))
   {
      if(mClickToCallAnchorEnabled)
      {
         // We are anchoring (B2B'ing) the call - time to start second leg
         mPeer = new B2BSession(mServer);
         mPeer->setPeer(this);
         mPeer->startClickToCallAnchorLeg(msg.header(h_To), mClickToCallDestination, mXmlRpcInfo);
         mWaitingOfferFromPeer = true;
      }
      else
      {
         // ClickToCall, but do not anchor call - send REFER here instead
         h->refer(NameAddr(mClickToCallDestination)); 
      }
   }
}

void
B2BSession::onOffer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{         
   InfoLog(B2BLOG_PREFIX << "onOffer: msg=" << LOG_MSG_WITH_SDP);

   if(isStaleFork(h->getDialogId())) return;

   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid())
      {
         mPeer->mInviteSessionHandle->provideOffer(sdp);
         mPeer->mWaitingOfferFromPeer = false;
         mWaitingAnswerFromPeer = true;
      }
      else
      {
         resip_assert(false);
      }
   }
   else
   {
      ErrLog(B2BLOG_PREFIX << "onOffer: No peer, rejecting with 501.");
      h->reject(501 /* Not Implemented */);
   }
}

void
B2BSession::onOfferRequired(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onOfferRequired: msg=" << LOG_MSG);

   if(isStaleFork(h->getDialogId())) return;

   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid())
      {
         mPeer->mInviteSessionHandle->requestOffer();
         mWaitingOfferFromPeer = true;
      }
      else
      {
         resip_assert(false);
      }
   }
   else
   {
      ErrLog(B2BLOG_PREFIX << "onOfferRequired: No peer, rejecting with 501.");
      h->reject(501 /* Not Implemented */);
   }
}

void
B2BSession::onOfferRejected(InviteSessionHandle h, const SipMessage* msg)
{
   int statusCode = 488;
   WarningCategory* warning=0;

   if(isStaleFork(h->getDialogId())) return;

   if(msg)
   {
      InfoLog(B2BLOG_PREFIX << "onOfferRejected: msg=" << LOG_MSGP);
      if(msg->exists(h_Warnings))
      {
         warning = (WarningCategory*)&msg->header(h_Warnings).back();
      }
      if(msg->isResponse())
      {
         statusCode = msg->header(h_StatusLine).responseCode();
      }            
   }
   else
   {
      InfoLog(B2BLOG_PREFIX << "onOfferRejected:");
   }

   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid() && mPeer->mWaitingAnswerFromPeer)
      {
         mPeer->mWaitingAnswerFromPeer = false;
         mPeer->mInviteSessionHandle->reject(statusCode, warning);
      }
   }
}

void
B2BSession::onOfferRequestRejected(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onOfferRequestRejected: msg=" << LOG_MSG);
   // This is called when we are waiting to resend a INVITE with no sdp after a glare condition, and we 
   // instead receive an inbound INVITE or UPDATE
   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid() && mPeer->mWaitingOfferFromPeer)
      {
         // Return glare to peer
         mPeer->mInviteSessionHandle->reject(491);
         mPeer->mWaitingOfferFromPeer = false;
      }
   }
}

void
B2BSession::onRemoteSdpChanged(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   /// called when a modified SDP is received in a 2xx response to a
   /// session-timer reINVITE. Under normal circumstances where the response
   /// SDP is unchanged from current remote SDP no handler is called
   /// There is not much we can do about this.  If session timers are used then they are managed seperately per leg
   /// and we have no real mechanism to notify the other peer of new SDP without starting a new offer/answer negotiation
   InfoLog(B2BLOG_PREFIX << "onRemoteSdpChanged: msg=" << LOG_MSG_WITH_SDP);
}

void
B2BSession::onInfo(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onInfo: msg=" << LOG_MSG);
   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid() && msg.getContents())
      {
         mPeer->mInviteSessionHandle->info(*msg.getContents());
         mWaitingNitAnswerFromPeer = true;
      }
   }
   else
   {
      h->acceptNIT();
   }
}

void
B2BSession::onInfoSuccess(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onInfoSuccess: msg=" << LOG_MSG);
   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid() && mPeer->mWaitingNitAnswerFromPeer)
      {
         mPeer->mInviteSessionHandle->acceptNIT(msg.header(h_StatusLine).responseCode(), msg.getContents());
      }
   }
}

void
B2BSession::onInfoFailure(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onInfoFailure: msg=" << LOG_MSG);
   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid() && mPeer->mWaitingNitAnswerFromPeer)
      {
         mPeer->mInviteSessionHandle->rejectNIT(msg.header(h_StatusLine).responseCode());
      }
   }
}

void
B2BSession::onRefer(InviteSessionHandle h, ServerSubscriptionHandle ss, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onRefer: msg=" << LOG_MSG);

   // If no peer (for some reason), then reject request
   if(!mPeer)
   {
      ss->send(ss->reject(403));
      return;
   }

   // Recurse on the REFER - do not pass to other leg
   try
   {
      // Accept the Refer
      ss->send(ss->accept(202 /* Refer Accepted */));

      B2BSession* newPeer = new B2BSession(mServer);

      mPeer->mWaitingOfferFromPeer = true;

      // Map other leg to this new one
      newPeer->stealPeer(this);
      newPeer->mReferringAppDialogSet = getHandle();

      SharedPtr<SipMessage> invitemsg = mDum.makeInviteSessionFromRefer(msg, ss->getHandle(), 0 /* SdpOffer */, newPeer);
      mDum.send(invitemsg);
   }
   catch(BaseException &e)
   {
      WarningLog(B2BLOG_PREFIX << "onRefer exception: " << e);
   }
   catch(...)
   {
      WarningLog(B2BLOG_PREFIX << "onRefer unknown exception");
   }
}

void
B2BSession::onReferAccepted(InviteSessionHandle h, ClientSubscriptionHandle csh, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onReferAccepted: msg=" << LOG_MSG);
}

void
B2BSession::onReferRejected(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onReferRejected: msg=" << LOG_MSG);

   // Endpoint doesn't support REFER - Fallback to anchoring Click-To-Call
   mClickToCallAnchorEnabled = true;
   mPeer = new B2BSession(mServer);
   mPeer->setPeer(this);
   mPeer->startClickToCallAnchorLeg(msg.header(h_From), mClickToCallDestination, mXmlRpcInfo);
   mWaitingOfferFromPeer = true;
}

bool 
B2BSession::doReferNoSub(const SipMessage& msg)
{
   // If no peer (for some reason), then return false
   if(!mPeer)
   {
      return false;
   }

   B2BSession* newPeer = new B2BSession(mServer);


   mPeer->mWaitingOfferFromPeer = true;

   // Map other leg to this new one
   newPeer->stealPeer(this);
   newPeer->mReferringAppDialogSet = getHandle();

   // Build the Invite
   SharedPtr<SipMessage> invitemsg = mDum.makeInviteSessionFromRefer(msg, getUserProfile(), 0 /* Sdp Offer */, newPeer);
   mDum.send(invitemsg);
   return true;
}

void
B2BSession::onReferNoSub(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onReferNoSub: msg=" << LOG_MSG);

   // If no peer (for some reason), then reject request
   if(!mPeer)
   {
      h->rejectReferNoSub(403);
      return;
   }

   // Accept the Refer
   h->acceptReferNoSub(202 /* Refer Accepted */);

   doReferNoSub(msg);
}

void
B2BSession::onMessage(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onMessage: msg=" << LOG_MSG);
   if(mPeer)  
   {
      if(mPeer->mInviteSessionHandle.isValid() && msg.getContents())
      {
         mPeer->mInviteSessionHandle->message(*msg.getContents());
         mWaitingNitAnswerFromPeer = true;
      }
   }
   else
   {
      h->acceptNIT();
   }
}

void
B2BSession::onMessageSuccess(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onMessageSuccess: msg=" << LOG_MSG);
   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid() && mPeer->mWaitingNitAnswerFromPeer)
      {
         mPeer->mInviteSessionHandle->acceptNIT(msg.header(h_StatusLine).responseCode(), msg.getContents());
      }
   }
}

void
B2BSession::onMessageFailure(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onMessageFailure: msg=" << LOG_MSG);
   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid() && mPeer->mWaitingNitAnswerFromPeer)
      {
         mPeer->mInviteSessionHandle->rejectNIT(msg.header(h_StatusLine).responseCode());
      }
   }
}

void
B2BSession::onForkDestroyed(ClientInviteSessionHandle h)
{
   InfoLog(B2BLOG_PREFIX << "onForkDestroyed:");
}

////////////////////////////////////////////////////////////////////////////////
// DialogSetHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
B2BSession::onTrying(AppDialogSetHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onTrying: msg=" << LOG_MSG);
}

void 
B2BSession::onNonDialogCreatingProvisional(AppDialogSetHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onNonDialogCreatingProvisional: msg=" << LOG_MSG);

   if(isUACConnected()) return;

   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid())
      {
         ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mPeer->mInviteSessionHandle.get());
         if(sis && !sis->isAccepted())
         {
            sis->provisional(msg.header(h_StatusLine).responseCode());
         }
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
// ClientSubscriptionHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
B2BSession::onUpdatePending(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   InfoLog(B2BLOG_PREFIX << "onUpdatePending(ClientSubscriptionHandle): " << LOG_MSG);
   if (msg.exists(h_Event) && msg.header(h_Event).value() == "refer")
   {
      h->acceptUpdate();
      processReferNotify(msg);
   }
   else
   {
      h->rejectUpdate(400, Data("Only notifies for refers are allowed."));
   }
}

void
B2BSession::onUpdateActive(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   InfoLog(B2BLOG_PREFIX << "onUpdateActive(ClientSubscriptionHandle): " << LOG_MSG);
   if (msg.exists(h_Event) && msg.header(h_Event).value() == "refer")
   {
      h->acceptUpdate();
      processReferNotify(msg);
   }
   else
   {
      h->rejectUpdate(400, Data("Only notifies for refers are allowed."));
   }
}

void
B2BSession::onUpdateExtension(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   InfoLog(B2BLOG_PREFIX << "onUpdateExtension(ClientSubscriptionHandle): " << LOG_MSG);
   if (msg.exists(h_Event) && msg.header(h_Event).value() == "refer")
   {
      h->acceptUpdate();
      processReferNotify(msg);
   }
   else
   {
      h->rejectUpdate(400, Data("Only notifies for refers are allowed."));
   }
}

void 
B2BSession::onNotifyNotReceived(resip::ClientSubscriptionHandle h)
{
   InfoLog(B2BLOG_PREFIX << "onNotifyNotReceived(ClientSubscriptionHandle)");
   transitionClickToCallState(ReferToDestinationFailed, 408);
   end();
}

void
B2BSession::onTerminated(ClientSubscriptionHandle h, const SipMessage* msg)
{
   if(msg)
   {
      InfoLog(B2BLOG_PREFIX << "onTerminated(ClientSubscriptionHandle): " << LOG_MSGP);
      //Note:  Final notify is sometimes only passed in the onTerminated callback
      if (msg->isRequest() && msg->exists(h_Event) && msg->header(h_Event).value() == "refer")
      {
         processReferNotify(*msg);
      }
   }
   else
   {
      InfoLog(B2BLOG_PREFIX << "onTerminated(ClientSubscriptionHandle)");
   }
}

void
B2BSession::onNewSubscription(ClientSubscriptionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onNewSubscription(ClientSubscriptionHandle): " << LOG_MSG);
}

int 
B2BSession::onRequestRetry(ClientSubscriptionHandle h, int retryMinimum, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onRequestRetry(ClientSubscriptionHandle): " << LOG_MSG);
   return -1;
}

}

/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */

