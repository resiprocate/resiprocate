#include "resiprocate/SdpContents.hxx"
#include "resiprocate/MultipartMixedContents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/InviteSession.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"
#include "resiprocate/dum/ServerInviteSession.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/UsageUseException.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Timer.hxx"
#include "resiprocate/os/Inserter.hxx"

#if defined(WIN32) && defined(_DEBUG) &&defined(LEAK_CHECK)// Used for tracking down memory leaks in Visual Studio
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new   new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif // defined(WIN32) && defined(_DEBUG)

// Remove warning about 'this' use in initiator list - pointer is only stored
#if defined(WIN32)
#pragma warning( disable : 4355 ) // using this in base member initializer list 
#endif

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

InviteSession::InviteSession(DialogUsageManager& dum, Dialog& dialog, State initialState)
   : DialogUsage(dum, dialog),
     mState(initialState),
     mNitState(NitComplete),
     mOfferState(Nothing),
     mCurrentLocalSdp(0),
     mCurrentRemoteSdp(0),
     mProposedLocalSdp(0),
     mProposedRemoteSdp(0),
     mNextOfferOrAnswerSdp(0),
     mUserConnected(false),
     mQueuedBye(0),
     mSessionInterval(0),
     mSessionRefresherUAS(false),
     mSessionTimerSeq(0),
     mDestroyer(this),
     mCurrentRetransmit200(Timer::T1)
{
   DebugLog ( << "^^^ InviteSession::InviteSession " << this);   
   assert(mDum.mInviteSessionHandler);
}

InviteSession::~InviteSession()
{
   DebugLog ( << "^^^ InviteSession::~InviteSession " << this);   
   delete mCurrentLocalSdp;
   delete mCurrentRemoteSdp;
   delete mProposedLocalSdp;
   delete mProposedRemoteSdp;
   delete mNextOfferOrAnswerSdp;
   delete mQueuedBye;   
   mDialog.mInviteSession = 0;
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
   mState = ReInviting;
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
   if (mNextOfferOrAnswerSdp == 0 || mState != ReInviting)
   {
      throw UsageUseException("Must be in the ReInviting state and have propsed an answer to call answerModifySession", 
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

const SdpContents* 
InviteSession::getLocalSdp() const
{
   return mCurrentLocalSdp;
}

const SdpContents* 
InviteSession::getRemoteSdp() const
{
   return mCurrentRemoteSdp;
}

InviteSessionHandle 
InviteSession::getSessionHandle()
{
   return InviteSessionHandle(mDum, getBaseHandle().getId());
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
            mState = ReInviting;
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
      return;
   }         

   if (msg.isRequest() && msg.header(h_RequestLine).method() == ACK && 
       (mState == Connected || mState == ReInviting))
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
         else if (mState != ReInviting && mOfferState != Answered)
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
               {
                  if (mOfferState == Answered)
                  {
                     mState = ReInviting;
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
      case ReInviting:
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
            InfoLog ( << "InviteSession::end, Connected" );  
            mDialog.makeRequest(mLastRequest, BYE);
            //new transaction
            assert(mLastRequest.header(h_Vias).size() == 1);
            mState = Terminated;
            send(mLastRequest);
         }
         break;
      case ReInviting:
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

void InviteSession::dialogDestroyed(const SipMessage& msg)
{
   mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), msg);   
   delete this;   
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

std::pair<InviteSession::OfferAnswerType, const SdpContents*>
InviteSession::getOfferOrAnswer(const SipMessage& msg) const
{
   std::pair<InviteSession::OfferAnswerType, const SdpContents*> ret;
   ret.first = None;
   const SdpContents* contents = NULL;

   MultipartMixedContents* mixed = dynamic_cast<MultipartMixedContents*>(msg.getContents());
   if ( mixed )
   {
      // Look for first SDP Contents in a multipart contents    
      MultipartMixedContents::Parts& parts = mixed->parts();
      for( MultipartMixedContents::Parts::const_iterator i = parts.begin(); 
           i != parts.end();  
           ++i)
      {
	     contents = dynamic_cast<const SdpContents*>(*i);
		 if(contents != NULL) break;  // Found SDP contents
      }
   }
   else
   {
      contents = dynamic_cast<const SdpContents*>(msg.getContents());
   }

   if (contents)
   {
      static Token c100rel(Symbols::C100rel);
      if (msg.isRequest() || msg.header(h_StatusLine).responseCode() == 200 ||
          (msg.exists(h_Requires) && msg.header(h_Requires).find(c100rel)))
      {
         switch (mOfferState)
         {
            case Nothing: 
               ret.first = Offer;
               ret.second = contents;
               break;
               
            case Offerred:
               ret.first = Answer;
               ret.second = contents;
               break;

            case Answered:
               ret.first = Offer;
               ret.second = contents;
               break;
               
            case CounterOfferred:
               ret.first = Answer;
               ret.second = contents;
               break;
         }
      }
      else if (msg.isResponse() && 
               msg.header(h_StatusLine).responseCode() < 200 && 
               msg.header(h_StatusLine).responseCode() >= 180)
      {
         ret.second = contents;
      }
   }
   return ret;
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
