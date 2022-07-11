#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "RemoteParticipant.hxx"
#include "IMParticipantBase.hxx"
#include "RemoteIMSessionParticipant.hxx"
#include "Conversation.hxx"
#include "UserAgent.hxx"
#include "DtmfEvent.hxx"
#include "ReconSubsystem.hxx"
#include "MediaStackAdapter.hxx"

#include <rutil/ResipAssert.h>
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/Random.hxx>
#include <resip/stack/DtmfPayloadContents.hxx>
#include <resip/stack/TrickleIceContents.hxx>
#include <resip/stack/SipFrag.hxx>
#include <resip/stack/ExtensionHeader.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/ClientInviteSession.hxx>
#include <resip/dum/ServerInviteSession.hxx>
#include <resip/dum/ClientSubscription.hxx>
#include <resip/dum/ServerOutOfDialogReq.hxx>
#include <resip/dum/ServerSubscription.hxx>

#include <rutil/WinLeakCheck.hxx>

#include <utility>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

/* Technically, there are a range of features that need to be implemented
   to be fully (S)AVPF compliant.
   However, it is speculated that (S)AVPF peers will communicate with legacy
   systems that just fudge the RTP/SAVPF protocol in their SDP.  Enabling
   this define allows such behavior to be tested. 

   http://www.ietf.org/mail-archive/web/rtcweb/current/msg01145.html
   "1) RTCWEB end-point will always signal AVPF or SAVPF. I signalling
   gateway to legacy will change that by removing the F to AVP or SAVP."

   http://www.ietf.org/mail-archive/web/rtcweb/current/msg04380.html
*/
//#define RTP_SAVPF_FUDGE

// UAC / Outbound
RemoteParticipant::RemoteParticipant(ParticipantHandle partHandle,
                                     ConversationManager& conversationManager, 
                                     DialogUsageManager& dum,
                                     RemoteParticipantDialogSet& remoteParticipantDialogSet) :
   Participant(partHandle, ConversationManager::ParticipantType_Remote, conversationManager),
   AppDialog(dum),
   IMParticipantBase(true /* prependSenderInfoToIMs? */),
   mDum(dum),
   mDialogSet(remoteParticipantDialogSet),
   mDialogId(Data::Empty, Data::Empty, Data::Empty),
   mState(Connecting),
   mOfferRequired(false),
   mLocalHold(true),
   mRemoteHold(false),
   mTrickleIce(false),
   mRedirectSuccessCondition(ConversationManager::RedirectSuccessOnConnected),
   mLocalSdp(0),
   mRemoteSdp(0)
{
   InfoLog(<< "RemoteParticipant created (UAC), handle=" << mHandle);
}

// UAS / Inbound - or forked leg
RemoteParticipant::RemoteParticipant(ConversationManager& conversationManager, 
                                     DialogUsageManager& dum, 
                                     RemoteParticipantDialogSet& remoteParticipantDialogSet) :
   Participant(ConversationManager::ParticipantType_Remote, conversationManager),
   AppDialog(dum),
   IMParticipantBase(false /* prependSenderInfoToIMs? */),
   mDum(dum),
   mDialogSet(remoteParticipantDialogSet),
   mDialogId(Data::Empty, Data::Empty, Data::Empty),
   mState(Connecting),
   mOfferRequired(false),
   mLocalHold(true),
   mRemoteHold(false),
   mTrickleIce(false),
   mRedirectSuccessCondition(ConversationManager::RedirectSuccessOnConnected),
   mLocalSdp(0),
   mRemoteSdp(0)
{
   InfoLog(<< "RemoteParticipant created (UAS or forked leg), handle=" << mHandle);
}

RemoteParticipant::~RemoteParticipant()
{
   if(!mDialogId.getCallId().empty())  
   {
      mDialogSet.removeDialog(mDialogId);
   }

   InfoLog(<< "RemoteParticipant destroyed, handle=" << mHandle);
}

//static const resip::ExtensionHeader h_AlertInfo("Alert-Info");

void 
RemoteParticipant::initiateRemoteCall(const NameAddr& destination)
{
   initiateRemoteCall(destination, nullptr, std::multimap<resip::Data,resip::Data>());
}

void
RemoteParticipant::initiateRemoteCall(const NameAddr& destination, const std::shared_ptr<ConversationProfile>& callingProfile, const std::multimap<resip::Data,resip::Data>& extraHeaders)
{
   ParticipantHandle handleId = mHandle;
   ConversationManager& cm = mConversationManager;
   buildSdpOffer(mLocalHold, [this, handleId, &cm, destination, callingProfile, extraHeaders](bool success, std::unique_ptr<SdpContents> _offer){
      if(!cm.getParticipant(handleId))
      {
         WarningLog(<<"handle no longer valid");
         return;
      }
      if(!success)
      {
         // FIXME - can/should we let the application know this failed?
         ErrLog(<<"failed to create offer, aborting");
         mConversationManager.onParticipantTerminated(mHandle, 500);
         delete this;
         return;
      }
      SdpContents& offer = *_offer;
      auto profile = callingProfile;
      if (!profile)
      {
         DebugLog(<<"initiateRemoteCall: no callingProfile supplied, calling mDialogSet.getConversationProfile()");
         profile = mDialogSet.getConversationProfile();
         resip_assert(profile);
      }
      auto invitemsg = mDum.makeInviteSession(
               destination,
               std::move(profile),
               &offer,
               &mDialogSet);

      std::multimap<resip::Data,resip::Data>::const_iterator it = extraHeaders.begin();
      for (; it != extraHeaders.end(); it++)
      {
         resip::Data headerName(it->first);
         resip::Data value(it->second);
         try
         {
            if (resip::isEqualNoCase(headerName, "Replaces"))
            {
               HeaderFieldValue hfv(value.data(), value.size());
               CallId callid(hfv, Headers::UNKNOWN);
               invitemsg->header(h_Replaces) = callid;
            }
            else if (resip::isEqualNoCase(headerName, "Remote-Party-ID"))
            {
               invitemsg->header(h_RemotePartyIds).push_back(NameAddr(value));
            }
            else if (resip::isEqualNoCase(headerName, "Privacy"))
            {
               invitemsg->header(h_Privacies).push_back(PrivacyCategory(value));
            }
            else if (resip::isEqualNoCase(headerName, "P-Asserted-Identity"))
            {
               invitemsg->header(h_PAssertedIdentities).push_back(NameAddr(value));
            }
            else
            {
               StackLog(<< "processing an extension header: " << headerName << ": " << value);
               resip::Headers::Type hType = resip::Headers::getType(headerName.data(), (int)headerName.size());
               if (hType == resip::Headers::UNKNOWN)
               {
                  resip::ExtensionHeader h_Tmp(headerName.c_str());
                  resip::ParserContainer<resip::StringCategory>& pc = invitemsg->header(h_Tmp);
                  resip::StringCategory sc(value);
                  pc.push_back(sc);
               }
               else
               {
                  WarningLog(<< "Discarding header '" << headerName << "', only extension headers and select standard headers permitted");
               }
            }
         }
         catch (resip::BaseException& ex)
         {
            WarningLog(<< "Discarding header '" << headerName << "', invalid value format '" << value << "': " << ex);
         }
      }

      mDialogSet.sendInvite(std::move(invitemsg));

      // Clear any pending hold/unhold requests since our offer/answer here will handle it
      if(mPendingRequest.mType == Hold ||
               mPendingRequest.mType == Unhold)
      {
         mPendingRequest.mType = None;
      }

      // Adjust RTP streams
      adjustRTPStreams(true);

      // Special case of this call - since call in addToConversation will not work, since we didn't know our bridge port at that time
      applyBridgeMixWeights();

   });
}

void 
RemoteParticipant::destroyParticipant()
{
   try
   {
      if(mState != Terminating)
      {
         stateTransition(Terminating);
         if (!mDialogSet.isUACConnected() && mDialogSet.getForkSelectMode() == ConversationManager::ForkSelectAutomaticEx)
         {
            // This will send a CANCEL if unanswered, in ForkSelectAutomaticEx mode we don't expect to
            // manage destruction of forked endpoints manually or individually.  This will also end
            // all the other related conversations.
            mDialogSet.endIncludeRelated(mHandle);
         }
         else
         {
            if (mInviteSessionHandle.isValid())
            {
               // Make sure any active Refer subscriptions are ended
               if (mReferSubscriptionHandle.isValid())
               {
                  mReferSubscriptionHandle->end();
               }
               // This will send a BYE.
               mInviteSessionHandle->end();
            }
            else
            {
               mDialogSet.end();
            }
         }
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteParticipant::destroyParticipant exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteParticipant::destroyParticipant unknown exception");
   }
}

void 
RemoteParticipant::addToConversation(Conversation* conversation, unsigned int inputGain, unsigned int outputGain)
{
   Participant::addToConversation(conversation, inputGain, outputGain);
   if(mLocalHold && !conversation->shouldHold())  // If we are on hold and we now shouldn't be, then unhold
   {
      unhold();
   }
}

void 
RemoteParticipant::removeFromConversation(Conversation *conversation)
{
   Participant::removeFromConversation(conversation);
   checkHoldCondition();
}

void
RemoteParticipant::checkHoldCondition()
{
   // Return to Offer a hold sdp if we are not in any conversations, or all the conversations we are in have conditions such that a hold is required
   bool shouldHold = true;
   ConversationMap::iterator it;
   for(it = mConversations.begin(); it != mConversations.end(); it++)
   {
      if(!it->second->shouldHold())
      {
         shouldHold = false;
         break;
      }
   }
   setLocalHold(shouldHold);
}

void
RemoteParticipant::setLocalHold(bool _hold)
{
   if(mLocalHold != _hold)
   {
      if(_hold)
      {
         hold();
      }
      else
      {
         unhold();
      }
   }
}

void 
RemoteParticipant::sendInstantMessage(std::unique_ptr<resip::Contents> contents)
{
   try
   {
      if (mState != Terminating)
      {
         if (mInviteSessionHandle.isValid())
         {
            mInviteSessionHandle->message(*contents.get());
         }
      }
   }
   catch (BaseException& e)
   {
      WarningLog(<< "RemoteParticipant::sendInstantMessage exception: " << e);
   }
   catch (...)
   {
      WarningLog(<< "RemoteParticipant::sendInstantMessage unknown exception");
   }
}

void 
RemoteParticipant::stateTransition(State state)
{
   Data stateName;

   switch(state)
   {
   case Connecting:
      stateName = "Connecting"; break;
   case Accepted:
      stateName = "Accepted"; break;
   case Connected:
      stateName = "Connected"; break;
   case Redirecting:
      stateName = "Redirecting"; break;
   case Holding:
      stateName = "Holding"; break;
   case Unholding:
      stateName = "Unholding"; break;
   case Replacing:
      stateName = "Replacing"; break;
   case PendingOODRefer:
      stateName = "PendingOODRefer"; break;
   case Terminating:
      stateName = "Terminating"; break;
   default:
      stateName = "Unknown: " + Data(state); break;
   }
   InfoLog( << "RemoteParticipant::stateTransition of handle=" << mHandle << " to state=" << stateName );
   mState = state;

   if(mState == Connected && mPendingRequest.mType != None)
   {
      PendingRequestType type = mPendingRequest.mType;
      mPendingRequest.mType = None;
      switch(type)
      {
      case Hold:
         hold();
         break;
      case Unhold:
         unhold();
         break;
      case Redirect:
         redirect(mPendingRequest.mDestination, mPendingRequest.mRedirectSuccessCondition, mPendingRequest.mRedirectSuccessCondition);
         break;
      case RedirectTo:
         redirectToParticipant(mPendingRequest.mDestInviteSessionHandle, mPendingRequest.mRedirectSuccessCondition);
         break;
      case None:
         break;
      }
   }
}

void
RemoteParticipant::enableTrickleIce()
{
   mTrickleIce = true;
}

void
RemoteParticipant::accept()
{
   try
   {
      // Accept SIP call if required
      if(mState == Connecting && mInviteSessionHandle.isValid())
      {
         ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mInviteSessionHandle.get());
         if(sis && !sis->isAccepted())
         { 
            if(!mediaStackPortAvailable())
            {
               WarningLog(<< "RemoteParticipant::accept cannot accept call, since no free RTP ports, rejecting instead.");
               sis->reject(480);  // Temporarily Unavailable - no free RTP ports
               return;
            }
            // Clear any pending hold/unhold requests since our offer/answer here will handle it
            if(mPendingRequest.mType == Hold ||
               mPendingRequest.mType == Unhold)
            {
               mPendingRequest.mType = None;
            }
            if(mOfferRequired)
            {
               provideOffer(true /* postOfferAccept */);
            }
            else if(mPendingOffer)
            {
               provideAnswer(*mPendingOffer, true /* postAnswerAccept */, false /* postAnswerAlert */);
            }
            else  
            {
               // It is possible to get here if the app calls alert with early true.  There is special logic in
               // RemoteParticipantDialogSet::accept to handle the case then an alert call followed immediately by 
               // accept.  In this case the answer from the alert will be queued waiting on the flow to be ready, and 
               // we need to ensure the accept call is also delayed until the answer completes.
               mDialogSet.accept(mInviteSessionHandle);
               stateTransition(Accepted);
            }
         }
      }
      // Accept Pending OOD Refer if required
      else if(mState == PendingOODRefer)
      {
         acceptPendingOODRefer();
      }
      else
      {
         WarningLog(<< "RemoteParticipant::accept called in invalid state: " << mState);
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteParticipant::accept exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteParticipant::accept unknown exception");
   }
}

void 
RemoteParticipant::alert(bool earlyFlag)
{
   try
   {
      if(mState == Connecting && mInviteSessionHandle.isValid())
      {
         ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mInviteSessionHandle.get());
         if(sis && !sis->isAccepted())
         {
            if(earlyFlag && mPendingOffer)
            {
               if(!mediaStackPortAvailable())
               {
                  WarningLog(<< "RemoteParticipant::alert cannot alert call with early media, since no free RTP ports, rejecting instead.");
                  sis->reject(480);  // Temporarily Unavailable - no free RTP ports
                  return;
               }

               provideAnswer(*mPendingOffer, false /* postAnswerAccept */, true /* postAnswerAlert */);
            }
            else
            {
               sis->provisional(180, earlyFlag);
            }
         }
      }
      else
      {
         WarningLog(<< "RemoteParticipant::alert called in invalid state: " << mState);
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteParticipant::alert exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteParticipant::alert unknown exception");
   }
}

void 
RemoteParticipant::reject(unsigned int rejectCode)
{
   try
   {
      // Reject UAS Invite Session if required
      if(mState == Connecting && mInviteSessionHandle.isValid())
      {
         ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mInviteSessionHandle.get());
         if(sis && !sis->isAccepted())
         {
            sis->reject(rejectCode);
         }
      }
      // Reject Pending OOD Refer request if required
      else if(mState == PendingOODRefer)
      {
         rejectPendingOODRefer(rejectCode);
      }
      else
      {
         WarningLog(<< "RemoteParticipant::reject called in invalid state: " << mState);
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteParticipant::reject exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteParticipant::reject unknown exception");
   }
}

void
RemoteParticipant::redirect(NameAddr& destination, unsigned int redirectCode, ConversationManager::RedirectSuccessCondition successCondition)
{
   try
   {
      // First look for redirect conditions, ie: UAS call that isn't answered yet
      // In this case we don't care about any pending requests, since we are rejecting the call
      if (mState == Connecting && mInviteSessionHandle.isValid())
      {
         ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mInviteSessionHandle.get());
         // If this is a UAS session and we haven't sent a final response yet - then redirect via 302 response
         if (sis && !sis->isAccepted())
         {
            NameAddrs destinations;
            destinations.push_back(destination);
            mConversationManager.onParticipantRedirectSuccess(mHandle);
            // If provided redirect code is not valid, then set it to the commonly used 302 code
            if (redirectCode < 300 || redirectCode > 399)
            {
               WarningLog(<< "RemoteParticipant::redirect: invalid redirect code of " << redirectCode << " provided, using 302 instead");
               redirectCode = 302;
            }
            sis->redirect(destinations, redirectCode);
            return;
         }
      }
      // Next look for Refer conditions
      if(mPendingRequest.mType == None)
      {
         if((mState == Connecting || mState == Accepted || mState == Connected) && mInviteSessionHandle.isValid())
         {
            if(mInviteSessionHandle->isConnected()) // redirect via blind transfer 
            {
               mRedirectSuccessCondition = successCondition;
               mInviteSessionHandle->refer(NameAddr(destination.uri()) /* remove tags */, true /* refersub */);
               stateTransition(Redirecting);
            }
            else
            {
               mPendingRequest.mType = Redirect;
               mPendingRequest.mDestination = destination;
               mPendingRequest.mRedirectCode = redirectCode;
               mPendingRequest.mRedirectSuccessCondition = successCondition;
            }
         }
         else if(mState == PendingOODRefer)
         {
            redirectPendingOODRefer(destination);
         }
         else
         {
            mPendingRequest.mType = Redirect;
            mPendingRequest.mDestination = destination;
            mPendingRequest.mRedirectCode = redirectCode;
            mPendingRequest.mRedirectSuccessCondition = successCondition;
         }
      }
      else
      {
         WarningLog(<< "RemoteParticipant::redirect error: request pending");
         mConversationManager.onParticipantRedirectFailure(mHandle, 406 /* Not Acceptable */);
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteParticipant::redirect exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteParticipant::redirect unknown exception");
   }
}

void
RemoteParticipant::redirectToParticipant(InviteSessionHandle& destParticipantInviteSessionHandle, ConversationManager::RedirectSuccessCondition successCondition)
{
   try
   {
      if(destParticipantInviteSessionHandle.isValid())
      {
         if(mPendingRequest.mType == None)
         {
            if((mState == Connecting || mState == Accepted || mState == Connected) && mInviteSessionHandle.isValid())
            {
               ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mInviteSessionHandle.get());
               // If this is a UAS session and we haven't sent a final response yet - then redirect via 302 response
               if(sis && !sis->isAccepted() && mState == Connecting)
               {
                  NameAddrs destinations;
                  destinations.push_back(NameAddr(destParticipantInviteSessionHandle->peerAddr().uri()));  // ensure we don't get to or from tag by only using the inner uri()
                  mConversationManager.onParticipantRedirectSuccess(mHandle);
                  sis->redirect(destinations);
               }
               else if(mInviteSessionHandle->isConnected()) // redirect via attended transfer (with replaces)
               {
                  mRedirectSuccessCondition = successCondition;
                  mInviteSessionHandle->refer(NameAddr(destParticipantInviteSessionHandle->peerAddr().uri()) /* remove tags */, destParticipantInviteSessionHandle /* session to replace)  */, true /* refersub */);
                  stateTransition(Redirecting);
               }
               else
               {
                  mPendingRequest.mType = RedirectTo;
                  mPendingRequest.mDestInviteSessionHandle = destParticipantInviteSessionHandle;
                  mPendingRequest.mRedirectSuccessCondition = successCondition;
               }
            }
            else
            {
               mPendingRequest.mType = RedirectTo;
               mPendingRequest.mDestInviteSessionHandle = destParticipantInviteSessionHandle;
               mPendingRequest.mRedirectSuccessCondition = successCondition;
            }
         }
         else
         {
            WarningLog(<< "RemoteParticipant::redirectToParticipant error: request pending");
            mConversationManager.onParticipantRedirectFailure(mHandle, 406 /* Not Acceptable */);
         }
      }
      else
      {
         WarningLog(<< "RemoteParticipant::redirectToParticipant error: destParticipant has no valid InviteSession");
         mConversationManager.onParticipantRedirectFailure(mHandle, 406 /* Not Acceptable */);
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteParticipant::redirectToParticipant exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteParticipant::redirectToParticipant unknown exception");
   }
}

void
RemoteParticipant::info(const Contents& contents)
{
   try
   {
      if(mPendingRequest.mType == None)
      {
         bool readyForInfo = (mState == Connected);
         if((mState == Connecting || mState == Accepted) && contents.getType() == TrickleIceContents::getStaticType())
         {
            // we can send INFO in the early media stage, subject to
            // the conditions in RFC 8840 s4.1
            readyForInfo = isTrickleIce();
         }
         if(readyForInfo)
         {
            if(mInviteSessionHandle.isValid())
            {
               DebugLog(<<"sending an INFO message");
               mInviteSessionHandle->info(contents);
            }
            else
            {
               WarningLog(<< "RemoteParticipant::info error: mInviteSessionHandle not valid");
            }
         }
         else
         {
            WarningLog(<< "RemoteParticipant::info error: mState not connected");
         }
      }
      else
      {
         WarningLog(<< "RemoteParticipant::info error: request pending");
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteParticipant::info exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteParticipant::info unknown exception");
   }
}

void 
RemoteParticipant::hold()
{
   mLocalHold=true;

   InfoLog(<< "RemoteParticipant::hold request: handle=" << mHandle);

   try
   {
      if(mPendingRequest.mType == None)
      {
         if(mState == Connected && mInviteSessionHandle.isValid())
         {
            provideOffer(false /* postOfferAccept */);
            stateTransition(Holding);
         }
         else
         {
            mPendingRequest.mType = Hold;
         }
      }
      else if(mPendingRequest.mType == Unhold)
      {
         mPendingRequest.mType = None;  // Unhold pending, so move to do nothing
         return;
      } 
      else if(mPendingRequest.mType == Hold)
      {
         return;  // Hold already pending
      }
      else
      {
         WarningLog(<< "RemoteParticipant::hold error: request already pending");
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteParticipant::hold exception: " << e);
   }   
   catch(...)
   {
      WarningLog(<< "RemoteParticipant::hold unknown exception");
   }
}

void 
RemoteParticipant::unhold()
{
   mLocalHold=false;

   InfoLog(<< "RemoteParticipant::unhold request: handle=" << mHandle);

   try
   {
      if(mPendingRequest.mType == None)
      {
         if(mState == Connected && mInviteSessionHandle.isValid())
         {
            provideOffer(false /* postOfferAccept */);
            stateTransition(Unholding);
         }
         else
         {
            mPendingRequest.mType = Unhold;
         }
      }
      else if(mPendingRequest.mType == Hold)
      {
         mPendingRequest.mType = None;  // Hold pending, so move do nothing
         return;
      } 
      else if(mPendingRequest.mType == Unhold)
      {
         return;  // Unhold already pending
      }
      else
      {
         WarningLog(<< "RemoteParticipant::unhold error: request already pending");
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "RemoteParticipant::unhold exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "RemoteParticipant::unhold unknown exception");
   }
}

void
RemoteParticipant::requestKeyframeFromPeer()
{
   std::shared_ptr<resip::SdpContents> _sdp = getRemoteSdp();
   if(!_sdp)
   {
      ErrLog(<<"we need to request a keyframe but there is no peer SDP");
      return;
   }
   std::set<Data> streamIDs = _sdp->session().getMediaStreamLabels();
   if(!streamIDs.empty())
   {
      DebugLog(<<"requesting a keyframe for stream(s)");
      MediaControlContents mcc;
      mcc.mediaControl() = MediaControlContents::MediaControl(streamIDs, true);
      info(mcc);
   }
   else
   {
      WarningLog(<<"we need to request a keyframe but the peer SDP does not contain a stream ID (RFC 4574)");
   }
}

void
RemoteParticipant::setRemoteHold(bool remoteHold)
{
   bool stateChanged = (remoteHold != mRemoteHold);
   mRemoteHold = remoteHold;
   if(stateChanged)
   {
      mConversationManager.onParticipantRequestedHold(mHandle, mRemoteHold);
   }
}

void 
RemoteParticipant::setPendingOODReferInfo(ServerOutOfDialogReqHandle ood, const SipMessage& referMsg)
{
   stateTransition(PendingOODRefer);
   mPendingOODReferMsg = referMsg;
   mPendingOODReferNoSubHandle = ood;
}

void 
RemoteParticipant::setPendingOODReferInfo(ServerSubscriptionHandle ss, const SipMessage& referMsg)
{
   stateTransition(PendingOODRefer);
   mPendingOODReferMsg = referMsg;
   mPendingOODReferSubHandle = ss;
}

void 
RemoteParticipant::acceptPendingOODRefer()
{
   if(mState == PendingOODRefer)
   {
      std::shared_ptr<UserProfile> profile;
      bool accepted = false;
      if(mPendingOODReferNoSubHandle.isValid())
      {
         mPendingOODReferNoSubHandle->send(mPendingOODReferNoSubHandle->accept(202));  // Accept OOD Refer
         profile = mPendingOODReferNoSubHandle->getUserProfile();
         accepted = true;
      }
      else if(mPendingOODReferSubHandle.isValid())
      {
         mPendingOODReferSubHandle->send(mPendingOODReferSubHandle->accept(202));  // Accept OOD Refer
         profile = mPendingOODReferSubHandle->getUserProfile();
         accepted = true;
      }
      if(accepted)
      {
         // Create offer
         ParticipantHandle handleId = mHandle;
         ConversationManager& cm = mConversationManager;
         buildSdpOffer(mLocalHold, [this, handleId, &cm, profile](bool success, std::unique_ptr<SdpContents> _offer){
            if(!cm.getParticipant(handleId))
            {
               WarningLog(<<"handle no longer valid");
               return;
            }
            if(!success)
            {
               ErrLog(<<"failed to create an SDP offer");
               mConversationManager.onParticipantTerminated(mHandle, 500);
               delete this;
               return;
            }
            SdpContents& offer = *_offer;
            // Build the Invite
            auto invitemsg = mDum.makeInviteSessionFromRefer(mPendingOODReferMsg,
                     profile,
                     mPendingOODReferSubHandle,  // Note will be invalid if refer no-sub, which is fine
                     &offer,
                     DialogUsageManager::None,  //EncryptionLevel
                     0,     // Alternative Contents
                     &mDialogSet);
            mDialogSet.sendInvite(std::move(invitemsg));

            adjustRTPStreams(true);

            stateTransition(Connecting);
         });
      }
      else
      {
         WarningLog(<< "acceptPendingOODRefer - no valid handles");
         mConversationManager.onParticipantTerminated(mHandle, 500);
         delete this;
      }
   }
}

void 
RemoteParticipant::rejectPendingOODRefer(unsigned int statusCode)
{
   if(mState == PendingOODRefer)
   {
      if(mPendingOODReferNoSubHandle.isValid())
      {
         mPendingOODReferNoSubHandle->send(mPendingOODReferNoSubHandle->reject(statusCode));
         mConversationManager.onParticipantTerminated(mHandle, statusCode);
      }
      else if(mPendingOODReferSubHandle.isValid())
      {
         mPendingOODReferSubHandle->send(mPendingOODReferSubHandle->reject(statusCode));  
         mConversationManager.onParticipantTerminated(mHandle, statusCode);
      }
      else
      {
         WarningLog(<< "rejectPendingOODRefer - no valid handles");
         mConversationManager.onParticipantTerminated(mHandle, 500);
      }
      mDialogSet.destroy();  // Will also cause "this" to be deleted
   }
}

void 
RemoteParticipant::redirectPendingOODRefer(resip::NameAddr& destination)
{
   if(mState == PendingOODRefer)
   {
      if(mPendingOODReferNoSubHandle.isValid())
      {
         auto redirect = mPendingOODReferNoSubHandle->reject(302 /* Moved Temporarily */);
         redirect->header(h_Contacts).clear();
         redirect->header(h_Contacts).push_back(destination);
         mPendingOODReferNoSubHandle->send(std::move(redirect));
         mConversationManager.onParticipantTerminated(mHandle, 302 /* Moved Temporarily */);
      }
      else if(mPendingOODReferSubHandle.isValid())
      {
         auto redirect = mPendingOODReferSubHandle->reject(302 /* Moved Temporarily */);
         redirect->header(h_Contacts).clear();
         redirect->header(h_Contacts).push_back(destination);
         mPendingOODReferSubHandle->send(std::move(redirect));  
         mConversationManager.onParticipantTerminated(mHandle, 302 /* Moved Temporarily */);
      }
      else
      {
         WarningLog(<< "rejectPendingOODRefer - no valid handles");
         mConversationManager.onParticipantTerminated(mHandle, 500);
      }
      mDialogSet.destroy();  // Will also cause "this" to be deleted
   }
}

void
RemoteParticipant::processReferNotify(ClientSubscriptionHandle h, const SipMessage& notify)
{
   unsigned int code = 400;  // Bad Request - default if for some reason a valid sipfrag is not present

   mReferSubscriptionHandle = h;

   SipFrag* frag  = dynamic_cast<SipFrag*>(notify.getContents());
   if (frag)
   {
      // Get StatusCode from SipFrag
      if (frag->message().isResponse())
      {
         code = frag->message().header(h_StatusLine).statusCode();
      }
   }

   // Check if response code in SipFrag meets our completion conditions
   if (mRedirectSuccessCondition == ConversationManager::RedirectSuccessOnTrying && code == 100)
   {
      if (mState == Redirecting)
      {
         if (mHandle) mConversationManager.onParticipantRedirectSuccess(mHandle);
         h->end();  // We don't care about any more updates, end the implied subscription
         stateTransition(Connected);
      }
   }
   else if ((mRedirectSuccessCondition == ConversationManager::RedirectSuccessOnTrying ||
             mRedirectSuccessCondition == ConversationManager::RedirectSuccessOnRinging) &&
            code >= 180 && code <= 189)
   {
      if (mState == Redirecting)
      {
         if (mHandle) mConversationManager.onParticipantRedirectSuccess(mHandle);
         h->end();  // We don't care about any more updates, end the implied subscription
         stateTransition(Connected);
      }
   }
   else if(code >= 200 && code < 300)
   {
      if(mState == Redirecting)
      {
         if (mHandle) mConversationManager.onParticipantRedirectSuccess(mHandle);
         h->end();  // We don't care about any more updates, end the implied subscription - shouldn't be required here, since 200's are normally sent with subscription state terminated
         stateTransition(Connected);
      }
   }
   else if(code >= 300)
   {
      if(mState == Redirecting)
      {
         if (mHandle) mConversationManager.onParticipantRedirectFailure(mHandle, code);
         stateTransition(Connected);
      }
   }
}

void 
RemoteParticipant::provideOffer(bool postOfferAccept, bool preferExistingSdp)
{
   resip_assert(mInviteSessionHandle.isValid());
   
   ParticipantHandle handleId = mHandle;
   ConversationManager& cm = mConversationManager;
   buildSdpOffer(mLocalHold,[this, handleId, &cm, postOfferAccept](bool success, std::unique_ptr<SdpContents> offer){
      if(!cm.getParticipant(handleId))
      {
         WarningLog(<<"handle no longer valid");
         return;
      }
      if(!success)
      {
         ErrLog(<<"buildSdpOffer failed");
         mConversationManager.onParticipantTerminated(mHandle, 500);
         delete this;
         return;
      }
      mDialogSet.provideOffer(std::move(offer), mInviteSessionHandle, postOfferAccept);
      mOfferRequired = false;
   }, preferExistingSdp);
}

void
RemoteParticipant::provideAnswer(const SdpContents& offer, bool postAnswerAccept, bool postAnswerAlert)
{
   resip_assert(mInviteSessionHandle.isValid());
   InviteSessionHandle h = getInviteSessionHandle();
   buildSdpAnswer(offer, [this, h, postAnswerAccept, postAnswerAlert](bool answerOk, std::unique_ptr<SdpContents> answer){
      if(!h.isValid())
      {
         WarningLog(<<"handle no longer valid");
         return;
      }
      if(answerOk)
      {
         mDialogSet.provideAnswer(std::move(answer), mInviteSessionHandle, postAnswerAccept, postAnswerAlert);
         if(postAnswerAccept && mState == Replacing)
         {
            stateTransition(Connecting);
         }
      }
      else
      {
         ErrLog(<<"buildSdpAnswer failed");
         mInviteSessionHandle->reject(488);
      }
      mPendingOffer.release();
   });
}

void 
RemoteParticipant::destroyConversations()
{
   ConversationMap temp = mConversations;  // Copy since we may end up being destroyed
   ConversationMap::iterator it;
   for(it = temp.begin(); it != temp.end(); it++)
   {
      it->second->destroy();
   }
}

void RemoteParticipant::notifyTerminating()
{
   // The dialogset is being cancelled, we are now terminating
   stateTransition(Terminating);

   ConversationMap::iterator it;
   for (it = mConversations.begin(); it != mConversations.end(); it++)
   {
      it->second->unregisterParticipant(this);
   }
   mConversations.clear();

   // It can take up to 32 seconds for a cancelled leg Dialog/DialogSet to actually get destroyed, we
   // don't want to make recon users wait this time, so signal the participant as destroyed immediately
   // and let DUM handle the rest in the background.
   if (mHandle != 0)
   {
      mConversationManager.onParticipantTerminated(mHandle, 0);
      mConversationManager.onParticipantDestroyed(mHandle);
      setHandle(0);        // unregister from Conversation Manager
   }
}

void 
RemoteParticipant::setProposedSdp(const resip::SdpContents& sdp)
{
   mDialogSet.setProposedSdp(mHandle, sdp);
}

void 
RemoteParticipant::setLocalSdp(const resip::SdpContents& sdp)
{
   InfoLog(<< "setLocalSdp: handle=" << mHandle << ", localSdp=" << sdp);
   mLocalSdp.reset(new SdpContents(sdp));
}

void 
RemoteParticipant::setRemoteSdp(const resip::SdpContents& sdp, bool answer)
{
   InfoLog(<< "setRemoteSdp: handle=" << mHandle << ", remoteSdp=" << sdp);
   mRemoteSdp.reset(new SdpContents(sdp));
   if(answer && mDialogSet.getProposedSdp())
   {
      mLocalSdp = mDialogSet.getProposedSdp();
   }
}

void 
RemoteParticipant::replaceWithParticipant(Participant* _replacingParticipant)
{
   RemoteParticipant* replacingParticipant = dynamic_cast<RemoteParticipant*>(_replacingParticipant);
    // Copy our local hold setting to the replacing participant to replace us
    replacingParticipant->mLocalHold = mLocalHold;         

    // We are about to adjust the participant handle of the replacing participant to ours
    // ensure that the mapping is also adjusted in the replacing participants dialog set
    if(replacingParticipant->mHandle == replacingParticipant->mDialogSet.getActiveRemoteParticipantHandle())
    {
        replacingParticipant->mDialogSet.setActiveRemoteParticipantHandle(mHandle);
    }

    Participant::replaceWithParticipant(replacingParticipant);
}

void
RemoteParticipant::onNewSession(ClientInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   InfoLog(<< "onNewSession(Client): handle=" << mHandle << ", " << msg.brief());
   mInviteSessionHandle = h->getSessionHandle();
   mDialogId = getDialogId();
}

void
RemoteParticipant::onNewSession(ServerInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   InfoLog(<< "onNewSession(Server): handle=" << mHandle << ", " << msg.brief());

   mInviteSessionHandle = h->getSessionHandle();         
   mDialogId = getDialogId();
            
   // First check if this INVITE is to replace an existing session
   if(msg.exists(h_Replaces))
   {
      pair<InviteSessionHandle, int> presult;
      presult = mDum.findInviteSession(msg.header(h_Replaces));
      if(!(presult.first == InviteSessionHandle::NotValid())) 
      {         
         RemoteParticipant* participantToReplace = dynamic_cast<RemoteParticipant *>(presult.first->getAppDialog().get());
         InfoLog(<< "onNewSession(Server): handle=" << mHandle << ", to replace handle=" << participantToReplace->getParticipantHandle() << ", " << msg.brief());

         // Assume Participant Handle of old call
         participantToReplace->replaceWithParticipant(this);      // adjust conversation mappings

         // Session to replace was found - end old session and flag to auto-answer this session after SDP offer-answer is complete
         participantToReplace->destroyParticipant();
         stateTransition(Replacing);
         return;
      }      
   }

   // Check for Auto-Answer indication - support draft-ietf-answer-mode-01 
   // and Answer-After parameter of Call-Info header
   ConversationProfile* profile = dynamic_cast<ConversationProfile*>(h->getUserProfile().get());
   bool autoAnswer = false;
   if(profile)
   {
      bool autoAnswerRequired;
      autoAnswer = profile->shouldAutoAnswer(msg, &autoAnswerRequired);
      if(!autoAnswer && autoAnswerRequired)  // If we can't autoAnswer but it was required, we must reject the call
      {
         WarningCategory warning;
         warning.hostname() = DnsUtil::getLocalHostName();
         warning.code() = 399; /* Misc. */
         warning.text() = "automatic answer forbidden";
         setHandle(0); // Don't generate any callbacks for this rejected invite
         h->reject(403 /* Forbidden */, &warning);
         return;
      }
   }
   else
   {
      WarningLog(<<"bypassing logic for Auto-Answer");
   }
  
   // notify of new participant
   notifyIncomingParticipant(msg, autoAnswer, *profile);
}

void 
RemoteParticipant::notifyIncomingParticipant(const resip::SipMessage& msg, bool autoAnswer, ConversationProfile& conversationProfile)
{
   if(mHandle) mConversationManager.onIncomingParticipant(mHandle, msg, autoAnswer, conversationProfile);
}

void
RemoteParticipant::onFailure(ClientInviteSessionHandle h, const SipMessage& msg)
{
   stateTransition(Terminating);
   InfoLog(<< "onFailure: handle=" << mHandle << ", " << msg.brief());
   // If ForkSelectMode is automatic, then ensure we destory any conversations, except the original
   if((mDialogSet.getForkSelectMode() == ConversationManager::ForkSelectAutomatic ||
       mDialogSet.getForkSelectMode() == ConversationManager::ForkSelectAutomaticEx) &&
      mHandle != mDialogSet.getActiveRemoteParticipantHandle())
   {
      destroyConversations();
   }
}

void
RemoteParticipant::onEarlyMedia(ClientInviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   InfoLog(<< "onEarlyMedia: handle=" << mHandle << ", " << msg.brief());
   if(!mDialogSet.isStaleFork(getDialogId()))
   {
      setRemoteSdp(sdp, true);
      adjustRTPStreams();
      if(sdp.session().isTrickleIceSupported())
      {
         enableTrickleIce();
      }
   }
}

void
RemoteParticipant::onProvisional(ClientInviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onProvisional: handle=" << mHandle << ", " << msg.brief());
   resip_assert(msg.header(h_StatusLine).responseCode() != 100);

   if(!mDialogSet.isStaleFork(getDialogId()))
   {
      if(mHandle) mConversationManager.onParticipantAlerting(mHandle, msg);
   }
}

void
RemoteParticipant::onConnected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onConnected(Client): handle=" << mHandle << ", " << msg.brief());
   
   // Check if this is the first leg in a potentially forked call to send a 200
   if(!mDialogSet.isUACConnected())  
   {
      if(mHandle) mConversationManager.onParticipantConnected(mHandle, msg);

      mDialogSet.setUACConnected(getDialogId(), mHandle);
      stateTransition(Connected);
   }
   else
   {
      // We already have a 200 response - send a BYE to this leg
      h->end();
   }
}

void
RemoteParticipant::onConnected(InviteSessionHandle, const SipMessage& msg)
{
   InfoLog(<< "onConnected: handle=" << mHandle << ", " << msg.brief());
   stateTransition(Connected);
}

void
RemoteParticipant::onConnectedConfirmed(InviteSessionHandle, const SipMessage& msg)
{
   InfoLog(<< "onConnectedConfirmed: handle=" << mHandle << ", " << msg.brief());
   ConversationMap::const_iterator it;
   for (it = mConversations.begin(); it != mConversations.end(); it++)
   {
      it->second->confirmParticipant(this);
   }
   if (mHandle) mConversationManager.onParticipantConnectedConfirmed(mHandle, msg);
   stateTransition(Connected);
}

void
RemoteParticipant::onStaleCallTimeout(ClientInviteSessionHandle)
{
   WarningLog(<< "onStaleCallTimeout: handle=" << mHandle);
}

void
RemoteParticipant::onTerminated(InviteSessionHandle h, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg)
{
   stateTransition(Terminating);
   switch(reason)
   {
   case InviteSessionHandler::RemoteBye:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", received a BYE from peer");
      break;
   case InviteSessionHandler::RemoteCancel:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", received a CANCEL from peer");
      break;
   case InviteSessionHandler::Rejected:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", received a rejection from peer");
      break;
   case InviteSessionHandler::LocalBye:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", ended locally via BYE");
      break;
   case InviteSessionHandler::LocalCancel:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", ended locally via CANCEL");
      break;
   case InviteSessionHandler::Replaced:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", ended due to being replaced");
      break;
   case InviteSessionHandler::Referred:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", ended due to being reffered");
      break;
   case InviteSessionHandler::Error:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", ended due to an error");
      break;
   case InviteSessionHandler::Timeout:
      InfoLog(<< "onTerminated: handle=" << mHandle << ", ended due to a timeout");
      break;
   default:
      resip_assert(false);
      break;
   }
   unsigned int statusCode = 0;
   if(msg)
   {
      if(msg->isResponse())
      {
         statusCode = msg->header(h_StatusLine).responseCode();
      }
   }

   // If this is a referred call and the refer is still around - then switch back to referrer (ie. failed transfer recovery)
   if(mHandle && mReferringAppDialog.isValid())
   {
      RemoteParticipant* participant = (RemoteParticipant*)mReferringAppDialog.get();

      replaceWithParticipant(participant);      // adjust conversation mappings
      if(participant->getParticipantHandle())
      {
         participant->adjustRTPStreams();
         return;
      }
   }

   // Ensure terminating party is from answered fork before generating event
   if(!mDialogSet.isStaleFork(getDialogId()))
   {
      if(mHandle) mConversationManager.onParticipantTerminated(mHandle, statusCode);
   }
}

void
RemoteParticipant::onRedirected(ClientInviteSessionHandle, const SipMessage& msg)
{
   InfoLog(<< "onRedirected: handle=" << mHandle << ", " << msg.brief());
}

void
RemoteParticipant::onAnswer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   InfoLog(<< "onAnswer: handle=" << mHandle << ", " << msg.brief());

   // Ensure answering party is from answered fork before generating event
   if(!mDialogSet.isStaleFork(getDialogId()))
   {
      setRemoteSdp(sdp, true);
      adjustRTPStreams();
   }
   stateTransition(Connected);  // This is valid until PRACK is implemented
   requestKeyframeFromPeer();
}

void
RemoteParticipant::onOffer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& offer)
{         
   InfoLog(<< "onOffer: handle=" << mHandle << ", " << msg.brief());
   if(mState == Connecting && mInviteSessionHandle.isValid())
   {
      ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mInviteSessionHandle.get());
      if(sis && !sis->isAccepted())
      {
         // Don't set answer now - store offer and set when needed - so that sendHoldSdp() can be calculated at the right instant
         // we need to allow time for app to add to a conversation before alerting(with early flag) or answering
         mPendingOffer = std::unique_ptr<SdpContents>(static_cast<SdpContents*>(offer.clone()));
         return;
      }
   }

   if(!mediaStackPortAvailable())
   {
      WarningLog(<< "RemoteParticipant::onOffer cannot continue due to no free RTP ports, rejecting offer.");
      h->reject(480);  // Temporarily Unavailable
   }
   else
   {
      provideAnswer(offer, mState==Replacing /* postAnswerAccept */, false /* postAnswerAlert */);
   }
}

void
RemoteParticipant::onOfferRequired(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onOfferRequired: handle=" << mHandle << ", " << msg.brief());
   // We are being asked to provide SDP to the remote end - we should no longer be considering that
   // remote end wants us to be on hold
   setRemoteHold(false);

   if(mState == Connecting && !h->isAccepted())  
   {
      // If we haven't accepted yet - delay providing the offer until accept is called (this allows time 
      // for a localParticipant to be added before generating the offer)
      mOfferRequired = true;
   }
   else
   {
      if(!mediaStackPortAvailable())
      {
         WarningLog(<< "RemoteParticipant::onOfferRequired cannot continue due to no free RTP ports, rejecting offer request.");
         h->reject(480);  // Temporarily Unavailable
      }
      else
      {
         provideOffer(mState == Replacing /* postOfferAccept */);
         if(mState == Replacing)
         {
            stateTransition(Connecting);
         }
      }
   }
}

void
RemoteParticipant::onOfferRejected(InviteSessionHandle, const SipMessage* msg)
{
   if(msg)
   {
      InfoLog(<< "onOfferRejected: handle=" << mHandle << ", " << msg->brief());
   }
   else
   {
      InfoLog(<< "onOfferRejected: handle=" << mHandle);
   }
}

void
RemoteParticipant::onOfferRequestRejected(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onOfferRequestRejected: handle=" << mHandle << ", " << msg.brief());
   resip_assert(0);  // We never send a request for an offer (ie. Invite with no SDP)
}

void
RemoteParticipant::onRemoteSdpChanged(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   InfoLog(<< "onRemoteSdpChanged: handle=" << mHandle << ", " << msg.brief());      
   setRemoteSdp(sdp);
   adjustRTPStreams();
}

bool
RemoteParticipant::onMediaControlEvent(MediaControlContents::MediaControl& mediaControl)
{
   InfoLog(<<"onMediaControlEvent: not implemented by this ConversationManager");
   return false;
}

bool
RemoteParticipant::onTrickleIce(TrickleIceContents& trickleIce)
{
   InfoLog(<<"onTrickleIce: not implemented by this ConversationManager");
   return false;
}

void
RemoteParticipant::onInfo(InviteSessionHandle session, const SipMessage& msg)
{
   InfoLog(<< "onInfo: handle=" << mHandle << ", " << msg.brief());
   if(mHandle)
   {
      bool accepted = false;

      DtmfPayloadContents* dtmfContents = dynamic_cast<DtmfPayloadContents*>(msg.getContents());
      if(dtmfContents)
      {
         DtmfPayloadContents::DtmfPayload& payload = dtmfContents->dtmfPayload();
         mConversationManager.onDtmfEvent(mHandle, payload.getEventCode(), payload.getDuration(), true);
         session->acceptNIT();
         accepted = true;
      }

      MediaControlContents* mediaControlContents = dynamic_cast<MediaControlContents*>(msg.getContents());
      if(mediaControlContents)
      {
         MediaControlContents::MediaControl& payload = mediaControlContents->mediaControl();
         if(onMediaControlEvent(payload))
         {
            session->acceptNIT();
            accepted = true;
         }
      }

      TrickleIceContents* trickleIceContents = dynamic_cast<TrickleIceContents*>(msg.getContents());
      if(trickleIceContents)
      {
         if(onTrickleIce(*trickleIceContents))
         {
            session->acceptNIT();
            accepted = true;
         }
      }

      if(!accepted)
      {
         WarningLog(<<"INFO message without recognized payload content-type, rejecting");
         session->rejectNIT();
      }
   }
   else
   {
      WarningLog(<<"INFO message received, but mHandle not set, rejecting");
      session->rejectNIT();
   }
}

void
RemoteParticipant::onInfoSuccess(InviteSessionHandle, const SipMessage& msg)
{
   WarningLog(<< "(unhandled) onInfoSuccess: handle=" << mHandle << ", " << msg.brief());
}

void
RemoteParticipant::onInfoFailure(InviteSessionHandle, const SipMessage& msg)
{
   WarningLog(<< "(unhandled) onInfoFailure: handle=" << mHandle << ", " << msg.brief());
}

void
RemoteParticipant::onRefer(InviteSessionHandle is, ServerSubscriptionHandle ss, const SipMessage& msg)
{
   InfoLog(<< "onRefer: handle=" << mHandle << ", " << msg.brief());

   try
   {
      // Accept the Refer
      ss->send(ss->accept(202 /* Refer Accepted */));

      // Figure out hold SDP before removing ourselves from the conversation
      bool holdSdp = mLocalHold;  

      // Use same ConversationProfile as this participant
      auto profile = mDialogSet.getConversationProfile();

      // Create new Participant - but use same participant handle
      RemoteParticipantDialogSet* participantDialogSet;
      if (dynamic_cast<RemoteIMSessionParticipant*>(this) != nullptr)
      {
         // If current participant is an IMSession participant then create new IM Session participant to handle refer
         participantDialogSet = mConversationManager.createRemoteIMSessionParticipantDialogSetInstance(mDialogSet.getForkSelectMode(), profile);
      }
      else
      {
         // otherwise, create a normal media based RemoteParticipant
         participantDialogSet = mConversationManager.getMediaStackAdapter().createRemoteParticipantDialogSetInstance(mDialogSet.getForkSelectMode(), profile);
      }
      RemoteParticipant *participant = participantDialogSet->createUACOriginalRemoteParticipant(mHandle); // This will replace old participant in ConversationManager map
      participant->mReferringAppDialog = getHandle();

      replaceWithParticipant(participant);      // adjust conversation mappings 

      // Create offer
      InviteSessionHandle h = getInviteSessionHandle();
      participant->buildSdpOffer(holdSdp, [this, h, msg, profile, ss, participantDialogSet, participant](bool success, unique_ptr<SdpContents> _offer){
         if(!h.isValid())
         {
            WarningLog(<<"handle no longer valid");
            return;
         }
         if(!success)
         {
            ErrLog(<<"failed to create an SDP offer");
            mConversationManager.onParticipantTerminated(mHandle, 500);
            delete this;
            return;
         }
         SdpContents& offer = *_offer;
         // Build the Invite
         auto NewInviteMsg = mDum.makeInviteSessionFromRefer(msg, profile, ss->getHandle(), &offer, DialogUsageManager::None, 0, participantDialogSet);
         participantDialogSet->sendInvite(std::move(NewInviteMsg));

         // Set RTP stack to listen
         participant->adjustRTPStreams(true);
      });

   }
   catch(BaseException &e)
   {
      WarningLog(<< "onRefer exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "onRefer unknown exception");
   }
}

void 
RemoteParticipant::doReferNoSub(const SipMessage& msg)
{
   // Figure out hold SDP before removing ourselves from the conversation
   bool holdSdp = mLocalHold;  

   // Use same ConversationProfile as this participant
   auto profile = mDialogSet.getConversationProfile();

   // Create new Participant - but use same participant handle
   RemoteParticipantDialogSet* participantDialogSet;
   if (dynamic_cast<RemoteIMSessionParticipant*>(this) != nullptr)
   {
      // If current participant is an IMSession participant then create new IM Session participant to handle refer
      participantDialogSet = mConversationManager.createRemoteIMSessionParticipantDialogSetInstance(mDialogSet.getForkSelectMode(), profile);
   }
   else
   {
      // otherwise, create a normal media based RemoteParticipant
      participantDialogSet = mConversationManager.getMediaStackAdapter().createRemoteParticipantDialogSetInstance(mDialogSet.getForkSelectMode(), profile);
   }
   RemoteParticipant *participant = participantDialogSet->createUACOriginalRemoteParticipant(mHandle); // This will replace old participant in ConversationManager map
   participant->mReferringAppDialog = getHandle();

   replaceWithParticipant(participant);      // adjust conversation mappings

   // Create offer
   ParticipantHandle handleId = mHandle;
   ConversationManager& cm = mConversationManager;
   participant->buildSdpOffer(holdSdp, [this, handleId, &cm, msg, profile, participantDialogSet, participant](bool success, unique_ptr<SdpContents> _offer){
      if(!cm.getParticipant(handleId))
      {
         WarningLog(<<"handle no longer valid");
         return;
      }
      if(!success)
      {
         ErrLog(<<"failed to create SDP offer");
         mConversationManager.onParticipantTerminated(mHandle, 500);
         delete this;
         return;
      }
      SdpContents& offer = *_offer;
      // Build the Invite
      auto NewInviteMsg = mDum.makeInviteSessionFromRefer(msg, profile, &offer, participantDialogSet);
      participantDialogSet->sendInvite(std::move(NewInviteMsg));

      // Set RTP stack to listen
      participant->adjustRTPStreams(true);
   });
}

void
RemoteParticipant::onReferNoSub(InviteSessionHandle is, const SipMessage& msg)
{
   InfoLog(<< "onReferNoSub: handle=" << mHandle << ", " << msg.brief());

   try
   {
      // Accept the Refer
		is->acceptReferNoSub(202 /* Refer Accepted */);

      doReferNoSub(msg);
   }
   catch(BaseException &e)
   {
      WarningLog(<< "onReferNoSub exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "onReferNoSub unknown exception");
   }
}

void
RemoteParticipant::onReferAccepted(InviteSessionHandle, ClientSubscriptionHandle, const SipMessage& msg)
{
   InfoLog(<< "onReferAccepted: handle=" << mHandle << ", " << msg.brief());
}

void
RemoteParticipant::onReferRejected(InviteSessionHandle, const SipMessage& msg)
{
   InfoLog(<< "onReferRejected: handle=" << mHandle << ", " << msg.brief());
   if(msg.isResponse() && mState == Redirecting)
   {
      if(mHandle) mConversationManager.onParticipantRedirectFailure(mHandle, msg.header(h_StatusLine).responseCode());
      stateTransition(Connected);
   }
}

void
RemoteParticipant::onMessage(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onMessage: handle=" << mHandle << ", " << msg.brief());

   h->acceptNIT();

   bool relay = false;
   if (mHandle) relay = mConversationManager.onReceiveIMFromParticipant(mHandle, msg);

   // Don't relay if sending the message to ourselves, this could end up in an infinite loop if both clients are using recon
   if (h->peerAddr().uri().getAOR(false) == h->myAddr().uri().getAOR(false))
   {
      relay = false;
   }

   // Relay to others in our conversations, if requested to do so
   if (relay)
   {
      Data remoteDisplayName = h->peerAddr().displayName();
      if (remoteDisplayName.empty())
      {
         remoteDisplayName = h->peerAddr().uri().user();
      }

      ConversationMap::iterator it;
      for (it = mConversations.begin(); it != mConversations.end(); it++)
      {
         it->second->relayInstantMessageToRemoteParticipants(mHandle, remoteDisplayName, msg);
      }
   }
}

void
RemoteParticipant::onMessageSuccess(InviteSessionHandle, const SipMessage& msg)
{
   InfoLog(<< "onMessageSuccess: handle=" << mHandle << ", " << msg.brief());
}

void
RemoteParticipant::onMessageFailure(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onMessageFailure: handle=" << mHandle << ", " << msg.brief());
   auto failedMessage = h->getLastSentNITRequest();
   resip_assert(failedMessage->getContents() != nullptr);

   std::unique_ptr<Contents> contents(failedMessage->getContents()->clone());
   if (mHandle) mConversationManager.onParticipantSendIMFailure(mHandle, msg, std::move(contents));
}

void
RemoteParticipant::onForkDestroyed(ClientInviteSessionHandle)
{
   InfoLog(<< "onForkDestroyed: handle=" << mHandle);
}


////////////////////////////////////////////////////////////////////////////////
// ClientSubscriptionHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
RemoteParticipant::onUpdatePending(ClientSubscriptionHandle h, const SipMessage& notify, bool outOfOrder)
{
   InfoLog(<< "onUpdatePending(ClientSub): handle=" << mHandle << ", " << notify.brief());
   if (notify.exists(h_Event) && notify.header(h_Event).value() == "refer")
   {
      h->acceptUpdate();
      processReferNotify(h, notify);
   }
   else
   {
      h->rejectUpdate(400, Data("Only notifies for refers are allowed."));
   }
}

void
RemoteParticipant::onUpdateActive(ClientSubscriptionHandle h, const SipMessage& notify, bool outOfOrder)
{
   InfoLog(<< "onUpdateActive(ClientSub): handle=" << mHandle << ", " << notify.brief());
   if (notify.exists(h_Event) && notify.header(h_Event).value() == "refer")
   {
      h->acceptUpdate();
      processReferNotify(h, notify);
   }
   else
   {
      h->rejectUpdate(400, Data("Only notifies for refers are allowed."));
   }
}

void
RemoteParticipant::onUpdateExtension(ClientSubscriptionHandle h, const SipMessage& notify, bool outOfOrder)
{
   InfoLog(<< "onUpdateExtension(ClientSub): handle=" << mHandle << ", " << notify.brief());
   if (notify.exists(h_Event) && notify.header(h_Event).value() == "refer")
   {
      h->acceptUpdate();
      processReferNotify(h, notify);
   }
   else
   {
      h->rejectUpdate(400, Data("Only notifies for refers are allowed."));
   }
}

void
RemoteParticipant::onTerminated(ClientSubscriptionHandle h, const SipMessage* notify)
{
   if(notify)
   {
      InfoLog(<< "onTerminated(ClientSub): handle=" << mHandle << ", " << notify->brief());
      if (notify->isRequest() && notify->exists(h_Event) && notify->header(h_Event).value() == "refer")
      {
         // Note:  Final notify is sometimes only passed in the onTerminated callback
         processReferNotify(h, *notify);
      }
      else if(notify->isResponse() && mState == Redirecting)
      {
         if(mHandle) mConversationManager.onParticipantRedirectFailure(mHandle, notify->header(h_StatusLine).responseCode());
         stateTransition(Connected);
      }
   }
   else
   {
      // Timed out waiting for notify
      InfoLog(<< "onTerminated(ClientSub): handle=" << mHandle);
      if(mState == Redirecting)
      {
         if(mHandle) mConversationManager.onParticipantRedirectFailure(mHandle, 408);
         stateTransition(Connected);
      }
   }
}

void
RemoteParticipant::onNewSubscription(ClientSubscriptionHandle h, const SipMessage& notify)
{
   InfoLog(<< "onNewSubscription(ClientSub): handle=" << mHandle << ", " << notify.brief());
}

int 
RemoteParticipant::onRequestRetry(ClientSubscriptionHandle h, int retryMinimum, const SipMessage& notify)
{
   InfoLog(<< "onRequestRetry(ClientSub): handle=" << mHandle << ", " << notify.brief());
   return -1;
}


/* ====================================================================

 Copyright (c) 2021-2022, SIP Spectrum, Inc. www.sipspectrum.com
 Copyright (c) 2021, Daniel Pocock https://danielpocock.com
 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
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
