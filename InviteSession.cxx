#include "resiprocate/SipMessage.hxx"
#include "resiprocate/SdpContents.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/InviteSession.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/UsageUseException.hxx"
#include "resiprocate/os/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

InviteSession::InviteSession(DialogUsageManager& dum, Dialog& dialog, State initialState)
   : BaseUsage(dum, dialog),
     mState(initialState),
     mOfferState(Nothing),
     mCurrentLocalSdp(0),
     mCurrentRemoteSdp(0),
     mProposedLocalSdp(0),
     mProposedRemoteSdp(0),
     mNextOfferOrAnswerSdp(0)
{
   assert(mDum.mInviteSessionHandler);
}

InviteSession::~InviteSession()
{
   delete mCurrentLocalSdp;
   delete mCurrentRemoteSdp;
   delete mProposedLocalSdp;
   delete mProposedRemoteSdp;
   delete mNextOfferOrAnswerSdp;
   mDialog.mInviteSession = 0;
}

void
InviteSession::setOffer(const SdpContents* sdp)
{
   if (mProposedRemoteSdp)
   {
      throw UsageUseException("Cannot set an offer with an oustanding remote offer", __FILE__, __LINE__);
   }
   mNextOfferOrAnswerSdp = static_cast<SdpContents*>(sdp->clone());
}

void
InviteSession::setAnswer(const SdpContents* sdp)
{
   if (mProposedLocalSdp )
   {
      throw UsageUseException("Cannot set an answer with an oustanding offer", __FILE__, __LINE__);
   }
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
InviteSession::dispatch(const SipMessage& msg)
{
   std::pair<OfferAnswerType, const SdpContents*> offans;
   offans = InviteSession::getOfferOrAnswer(msg);

   switch(mState)
   {
      case Terminated:
         //!dcm! -- 481 behaviour here, should pretty much die on anything
         if (msg.isResponse() && msg.header(h_StatusLine).statusCode() == 200 && msg.header(h_CSeq).method() == BYE)
         {
            delete this;
         }
         break;
      case Connected:
         // reINVITE
         if (msg.isRequest())
         {
            switch(msg.header(h_RequestLine).method())
            {
               case INVITE:
                  mDialog.update(msg);
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
                  assert(0); // !jf! 
                  mDum.mInviteSessionHandler->onRefer(getSessionHandle(), msg);
                  break;
                  
               default:
                  InfoLog (<< "Ignoring request in an INVITE dialog: " << msg.brief());
                  break;
            }
         }      
      case Accepting:
         if (msg.isRequest() && msg.header(h_RequestLine).method() == ACK)
         {
            //cancel 200 retransmission timer
            mState = Connected;
            mDum.mInviteSessionHandler->onConnected(getSessionHandle(), msg);
            if (offans.first != None)
            {
               InviteSession::incomingSdp(msg, offans.second);
            }
         }
         else
         {
            assert(0); // !dcm! -- usual nonsence message behaviour question            
         }
         break;         
      default:
         assert(0);  //all other cases should be handled in base classes
   }
}

SipMessage& 
InviteSession::makeRefer(const H_ReferTo::Type& referTo)
{
}

SipMessage&
InviteSession::end()
{
   switch (mState)
   {
      case Terminated: 
         throw UsageUseException("Cannot end a session that has already been cancelled.", __FILE__, __LINE__);
         break;
      case Connected:
         mDialog.makeRequest(mLastRequest, BYE);
         mState = Terminated;
         return mLastRequest;
         break;
      default:
         assert(0); // out of states
   }
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
         mProposedRemoteSdp = static_cast<SdpContents*>(sdp->clone());
         mOfferState = Offerred;
         mDum.mInviteSessionHandler->onOffer(getSessionHandle(), msg, sdp);
         break;
         
      case Offerred:
         mCurrentLocalSdp = mProposedLocalSdp;
         mCurrentRemoteSdp = static_cast<SdpContents*>(sdp->clone());
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
         if (sdp)
         {
            delete mCurrentLocalSdp;
            delete mCurrentRemoteSdp;
            mCurrentLocalSdp = mProposedLocalSdp;
            mCurrentRemoteSdp = static_cast<SdpContents*>(sdp->clone());
            mProposedLocalSdp = 0;
            mProposedRemoteSdp = 0;
            mOfferState = Answered;
            mDum.mInviteSessionHandler->onAnswer(getSessionHandle(), msg, sdp);
         }
         else
         {
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
   if (msg.isRequest())
   {
      //unless the message is an ACK(in which case it is mAck)
      //strip out the SDP after sending
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
         delete this;
      }
      else if (code >= 200 && code < 300 && msg.header(h_CSeq).method() == INVITE)
      {
         assert(&msg == &mFinalResponse);
         //!dcm! -- start timer...this should be mFinalResponse...maybe assign here in
         //case the user wants to be very strange
         if (mNextOfferOrAnswerSdp)
         {
            msg.setContents(static_cast<SdpContents*>(mNextOfferOrAnswerSdp->clone()));
            sendSdp(mNextOfferOrAnswerSdp);
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
         mCurrentLocalSdp = sdp;
         mCurrentRemoteSdp = mProposedRemoteSdp;
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
            mCurrentLocalSdp = static_cast<SdpContents*>(sdp->clone());
            mCurrentRemoteSdp = mProposedRemoteSdp;
         }
         else
         {
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
   mDialog.makeResponse(mLastResponse, mLastRequest, statusCode);
   return mLastResponse;
}

SipMessage& 
InviteSession::targetRefresh(const NameAddr& localUri)
{
   assert(0);
}

void 
InviteSession::makeAck()
{
   mAck = mLastRequest;
   mDialog.makeRequest(mAck, ACK);
   if (mNextOfferOrAnswerSdp)
   {
      mAck.setContents(static_cast<SdpContents*>(mNextOfferOrAnswerSdp->clone()));
      sendSdp(mNextOfferOrAnswerSdp);
   }   
}

SipMessage& 
InviteSession::reInvite(const SdpContents* offer)
{
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
