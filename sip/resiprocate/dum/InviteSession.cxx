#include "resiprocate/SdpContents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/InviteSession.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/UsageUseException.hxx"
#include "resiprocate/os/Logger.hxx"

#if defined(WIN32) && defined(_DEBUG) &&defined(LEAK_CHECK)// Used for tracking down memory leaks in Visual Studio
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new   new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif // defined(WIN32) && defined(_DEBUG)


#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

unsigned long
InviteSession::T1 = 500;

unsigned long
InviteSession::T2 = 8 * T1;

unsigned long
InviteSession::TimerH = 64 * T1;

InviteSession::InviteSession(DialogUsageManager& dum, Dialog& dialog, State initialState)
   : DialogUsage(dum, dialog),
     mState(initialState),
     mOfferState(Nothing),
     mCurrentLocalSdp(0),
     mCurrentRemoteSdp(0),
     mProposedLocalSdp(0),
     mProposedRemoteSdp(0),
     mNextOfferOrAnswerSdp(0),
     mDestroyer(this),
     mCurrentRetransmit200(0)
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
   mDialog.mInviteSession = 0;
}

SipMessage& 
InviteSession::modifySession()
{
   if (mNextOfferOrAnswerSdp == 0 || mState != Connected)
   {
      throw new UsageUseException("Must be in the connected state and have propsed an offer to call modifySession", 
                                  __FILE__, __LINE__);
   }
   mState = ReInviting;
   mDialog.makeRequest(mLastRequest, INVITE);
   return mLastRequest;
}


SipMessage& 
InviteSession::acceptOffer(int statusCode)
{
   if (mNextOfferOrAnswerSdp == 0 || mState != ReInviting)
   {
      throw new UsageUseException("Must be in the ReInviting state and have propsed an answer to call answerModifySession", 
                                  __FILE__, __LINE__);
   }
   mState = AcceptingReInvite;
   mDialog.makeResponse(mFinalResponse, mLastRequest, statusCode);
   return mFinalResponse;
} 

void
InviteSession::setOffer(const SdpContents* sdp)
{
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
   if (mProposedLocalSdp )
   {
      throw UsageUseException("Cannot set an answer with an oustanding offer", __FILE__, __LINE__);
   }
   assert(mNextOfferOrAnswerSdp == 0);
   mNextOfferOrAnswerSdp = static_cast<SdpContents*>(sdp->clone());
}

const SdpContents* 
InviteSession::getLocalSdp()
{
   return mCurrentLocalSdp;
}

const SdpContents* 
InviteSession::getRemoteSdp()
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
   if (timeout.type() == DumTimeout::Retransmit200 && (mState == Accepting || mState == AcceptingReInvite ))
   {
      mDum.send(mFinalResponse);      
      mDum.addTimerMs(DumTimeout::Retransmit200, resipMin(T2, mCurrentRetransmit200*2), getBaseHandle(),  0);
   }
   else if (timeout.type() == DumTimeout::WaitForAck && mState != Connected)
   {
      mDialog.makeResponse(mLastResponse, mLastRequest, 408);
      mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), mLastResponse);      
      guard.destroy();      
   }
}

void
InviteSession::dispatch(const SipMessage& msg)
{
   Destroyer::Guard guard(mDestroyer);
   std::pair<OfferAnswerType, const SdpContents*> offans;
   offans = InviteSession::getOfferOrAnswer(msg);

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
                  mState = ReInviting;
                  mDialog.update(msg);
				  mLastRequest = msg; // !slg!
                  mDum.mInviteSessionHandler->onDialogModified(getSessionHandle(), msg);
                  if (offans.first != None)
                  {
                     incomingSdp(msg, offans.second);
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
                  
               default:
                  InfoLog (<< "Ignoring request in an INVITE dialog: " << msg.brief());
                  break;
            }
         }
         else
         {
            //!dcm! -- need to change this logic for when we don't have an ACK yet
            if ( msg.header(h_StatusLine).statusCode() == 200)
            {
               //retransmist ack
               mDum.send(mAck);
            }
         }
         break;
      case ReInviting:
         if (msg.isResponse() && msg.header(h_CSeq).method() == INVITE)
         {
            int code = msg.header(h_StatusLine).statusCode();
            if (code >=200 && code < 300)
            {
               mState = Connected;
               send(ackConnection());
               if (offans.first != None)
               {
                  incomingSdp(msg, offans.second);
               }
            }
            else
            {
               mDum.mInviteSessionHandler->onOfferRejected(getSessionHandle(), msg);
               mState = Connected;               
            }
         }
         else
         {
            ErrLog ( << "Spurious message sent to UAS " << msg );            
            return;            
         }
         break;
      case Accepting:
         if (msg.isRequest() && msg.header(h_RequestLine).method() == ACK)
         {
            mState = Connected;
            mDum.mInviteSessionHandler->onConnected(getSessionHandle(), msg);
            if (offans.first != None)
            {
               InviteSession::incomingSdp(msg, offans.second);
            }
         }
         else
         {
            ErrLog ( << "Spurious message sent to UAS " << msg );            
            return;            
         }
         break;         
      case AcceptingReInvite:
         if (msg.isRequest() && msg.header(h_RequestLine).method() == ACK)
         {
            mState = Connected;            
            //this shouldn't happen, but it may be allowed(DUM API doesn't
            //support this for re-invite)
            if (offans.first != None)
            {
               InviteSession::incomingSdp(msg, offans.second);
            }
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
InviteSession::makeRefer(const NameAddr& referTo)
{
   mDialog.makeRequest(mLastRequest, REFER);
   mLastRequest.header(h_ReferTo) = referTo;
   return mLastRequest;   
}

SipMessage& 
InviteSession::makeRefer(const NameAddr& referTo, InviteSessionHandle sessionToReplace)
{
   if (!sessionToReplace.isValid())
   {
      throw new UsageUseException("Attempted to make a refer w/ and invalid replacement target", __FILE__, __LINE__);
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

SipMessage&
InviteSession::end()
{
   InfoLog ( << "InviteSession::end, state: " << mState);  
   switch (mState)
   {
      case Terminated: 
         throw UsageUseException("Cannot end a session that has already been cancelled.", __FILE__, __LINE__);
         break;
      case Connected:
      case Accepting:
         InfoLog ( << "InviteSession::end, connected or Accepting" );  
         mDialog.makeRequest(mLastRequest, BYE);
         //new transaction
         assert(mLastRequest.header(h_Vias).size() == 1);
//         mLastRequest.header(h_Vias).front().param(p_branch).reset();
         mState = Terminated;
         return mLastRequest;
         break;
      default:
         assert(0); // out of states
   }
   throw UsageUseException("Programmer error", __FILE__, __LINE__); //make VC++ happy
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
            mDum.mInviteSessionHandler->onOfferRejected(getSessionHandle(), msg);
         }
         break;
   }
}

void 
InviteSession::send(SipMessage& msg)
{
   Destroyer::Guard guard(mDestroyer);
   if (msg.isRequest())
   {
      //unless the message is an ACK(in which case it is mAck)
      //strip out the SDP after sending
      switch(msg.header(h_RequestLine).getMethod())
      {
         case INVITE:
         case UPDATE:
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
      
      if (msg.header(h_RequestLine).getMethod() == ACK)
      {
         mDum.send(msg);
      }
      else
      {
         mDum.send(msg);
         msg.releaseContents();
      }
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
         assert(&msg == &mFinalResponse);
         mCurrentRetransmit200 = T1;         
         mDum.addTimerMs(DumTimeout::Retransmit200, mCurrentRetransmit200, getBaseHandle(),  0);
         mDum.addTimerMs(DumTimeout::WaitForAck, TimerH, getBaseHandle(),  0);
            
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
         msg.releaseContents();
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
         if (sdp)  // !slg! There currenlty doesn't seem to be anyone calling this with sdp == 0
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
   
   const SdpContents* contents = dynamic_cast<const SdpContents*>(msg.getContents());
   if (contents)
   {
      static Token c100rel(Symbols::C100rel);
      if (msg.isRequest() || msg.header(h_StatusLine).responseCode() == 200 ||
          msg.exists(h_Supporteds) && msg.header(h_Supporteds).find(c100rel))
      {
         switch (mOfferState)
         {
            case None: 
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
   }
   return ret;
}

void
InviteSession::copyAuthorizations(SipMessage& request)
{
#if 0
   if (mLastRequest.exists(h_ProxyAuthorizations))
   {
      // should make the next auth (change nextNonce)
      request.header(h_ProxyAuthorizations) = mLastRequest.header(h_ProxyAuthorizations);
   }
   if (mLastRequest.exists(h_ProxyAuthorizations))
   {
      // should make the next auth (change nextNonce)
      request.header(h_ProxyAuthorizations) = mLastRequest.header(h_ProxyAuthorizations);
   }
#endif
}

SipMessage& 
InviteSession::rejectOffer(int statusCode)
{
   if (statusCode < 400)
   {
      throw new UsageUseException("Must reject with a 4xx", __FILE__, __LINE__);
   }
   //sdp state change here--go to initial state?
   mDialog.makeResponse(mLastResponse, mLastRequest, statusCode);
   return mLastResponse;
}

SipMessage& 
InviteSession::targetRefresh(const NameAddr& localUri)
{
   assert(0);
   return mLastRequest;
}

SipMessage& 
InviteSession::ackConnection()
{
   //if not a reinvite, and a pending offer exists, throw
   makeAck();
   //new transaction
   assert(mAck.header(h_Vias).size() == 1);
//   mAck.header(h_Vias).front().param(p_branch).reset();
   return mAck;
}

void 
InviteSession::makeAck()
{
   mAck = mLastRequest;

   InfoLog ( << "InviteSession::makeAck:before: " << mAck );   

   mDialog.makeRequest(mAck, ACK);
   if (mNextOfferOrAnswerSdp)
   {
      mAck.setContents(mNextOfferOrAnswerSdp);
      sendSdp(mNextOfferOrAnswerSdp);
      mNextOfferOrAnswerSdp = 0;
   }

   InfoLog ( << "InviteSession::makeAck:after: " << mAck );   
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
