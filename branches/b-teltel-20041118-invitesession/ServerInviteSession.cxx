#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/DumTimeout.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/ServerInviteSession.hxx"
#include "resiprocate/dum/UsageUseException.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/compat.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

ServerInviteSession::ServerInviteSession(DialogUsageManager& dum, Dialog& dialog, const SipMessage& request)
   : InviteSession(dum, dialog)
{
   assert(request.isRequest());
   mFirstRequest = request;   
   mState = UAS_Start;
}

ServerInviteSessionHandle 
ServerInviteSession::getHandle()
{
   return ServerInviteSessionHandle(mDum, getBaseHandle().getId());
}

void 
ServerInviteSession::dispatch(const SipMessage& msg)
{
}

void 
ServerInviteSession::dispatch(const DumTimeout& msg)
{
}

void 
ServerInviteSession::redirect(const NameAddrs& contacts, int code)
{
   Destroyer::Guard guard(mDestroyer);

   // !jf! the cleanup for 3xx may be a bit strange if we are in the middle of
   // an offer/answer exchange with PRACK. 
   // e.g. we sent 183 reliably and then 302 before PRACK was received. Ideally,
   // we should send 200PRACK
   SipMessage response;
   mDialog.makeResponse(mFirstRequest, response, code);
   response.header(h_Contacts) = contacts;
   mDum.send(response);
   transition(Terminated);

   guard.destroy();
}

void 
ServerInviteSession::provisional(int code)
{
   switch (mState)
   {
      case UAS_Offer:
         transition(UAS_Early);
         sendProvisional(code);
         break;
         
      case UAS_Early:
         transition(UAS_Early);
         sendProvisional(code);
         break;
         
      case UAS_NoOffer:
         transition(UAS_EarlyNoOffer);
         sendProvisional(code);
         break;
         
      case UAS_EarlyNoOffer:
         transition(UAS_EarlyNoOffer);
         sendProvisional(code);
         break;         

      case UAS_NoOfferReliable:
      case UAS_EarlyReliable:
         // TBD
         assert(0);
         break;
         
      case UAS_Accepted:
      case UAS_FirstEarlyReliable:
      case UAS_FirstSentOfferReliable:
      case UAS_OfferReliable: 
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

void
ServerInviteSession::provideEarly(const SdpContents& early)
{
   // queue early media
   mEarlyMedia = InviteSession::makeSdp(early);
}

void 
ServerInviteSession::provideOffer(const SdpContents& offer)
{
   switch (mState)
   {
      case UAS_NoOffer:
      case UAS_EarlyNoOffer:
      case UAS_NoOfferReliable:
         // queue offer
         mProposedLocalSdp = InviteSession::makeSdp(offer);
         break;

      case UAS_EarlyReliable:
         // queue offer
         mProposedLocalSdp = InviteSession::makeSdp(offer);
         sendUpdate(offer);
         transition(UAS_SentUpdate);
         break;
         
      case UAS_Accepted:
      case UAS_FirstEarlyReliable:
      case UAS_FirstSentOfferReliable:
      case UAS_OfferReliable: 
      case UAS_ReceivedUpdate:
      case UAS_ReceivedUpdateWaitingAnswer:
      case UAS_SentUpdate:
      case UAS_SentUpdateAccepted:
      case UAS_Start:
      case UAS_WaitingToHangup:
      case UAS_WaitingToTerminate:
         assert(0);
         break;
      default:
         InviteSession::provideOffer(offer);
         break;
   }
}

void 
ServerInviteSession::provideAnswer(const SdpContents& answer)
{
   switch (mState)
   {
      case UAS_Offer:
      case UAS_Early:
         mCurrentRemoteSdp = mProposedRemoteSdp;
         mCurrentLocalSdp = InviteSession::makeSdp(answer);
         break;
         
      case UAS_OfferReliable: 
         // send1XX-answer, timer::1xx
         transition(UAS_FirstEarlyReliable);
         break;

      case UAS_ReceivedUpdate:
         // send::200U-answer
         transition(UAS_EarlyReliable);
         break;
         
      case UAS_ReceivedUpdateWaitingAnswer:
         // send::2XXU-answer
         // send::2XXI
         transition(Connected);
         break;

      case UAS_Start:
      case UAS_Accepted:
      case UAS_EarlyReliable:
      case UAS_FirstEarlyReliable:
      case UAS_FirstSentOfferReliable:
      case UAS_NoOfferReliable:
      case UAS_SentUpdate:
      case UAS_SentUpdateAccepted:
      case UAS_WaitingToHangup:
      case UAS_WaitingToTerminate:
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
   switch (mState)
   {
      case UAS_Start:
         break;
      case UAS_OfferReliable: 
         break;
      case UAS_ReceivedUpdate:
         break;
      case UAS_Accepted:
         break;
      case UAS_EarlyReliable:
         break;
      case UAS_FirstEarlyReliable:
         break;
      case UAS_FirstSentOfferReliable:
         break;
      case UAS_NoOfferReliable:
         break;
      case UAS_ReceivedUpdateWaitingAnswer:
         break;
      case UAS_SentUpdate:
         break;
      case UAS_SentUpdateAccepted:
         break;
      case UAS_WaitingToHangup:
         break;
      case UAS_WaitingToTerminate:
         break;
      default:
         InviteSession::end();
         break;
   }
}

void 
ServerInviteSession::reject(int code)
{
   switch (mState)
   {
      case UAS_EarlyReliable:
      case UAS_FirstEarlyReliable:
      case UAS_FirstSentOfferReliable:
      case UAS_NoOfferReliable:
      case UAS_OfferReliable: 
      case UAS_ReceivedUpdate:
      case UAS_SentUpdate:
      {
         Destroyer::Guard guard(mDestroyer);
         
         // !jf! the cleanup for 3xx may be a bit strange if we are in the middle of
         // an offer/answer exchange with PRACK. 
         // e.g. we sent 183 reliably and then 302 before PRACK was received. Ideally,
         // we should send 200PRACK
         SipMessage response;
         mDialog.makeResponse(mFirstRequest, response, code);
         mDum.send(response);
         transition(Terminated);
         guard.destroy();
         break;
      }

      case UAS_Accepted:
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

void 
ServerInviteSession::accept(int code)
{
   switch (mState)
   {
      case UAS_Early:
         // send::2xx-answer
         // timer::2xx
         // timer::NoAck
         transition(UAS_Accepted);
         break;

      case UAS_EarlyNoOffer:
         // send::2xx-offer
         // timer::2xx
         // timer::NoAck
         transition(UAS_Accepted);
         break;
         
      case UAS_FirstEarlyReliable:
         // queue 2xx
         // waiting for PRACK
         mDialog.makeResponse(mInvite200, mFirstRequest, code);
         transition(UAS_Accepted);
         break;
         
      case UAS_EarlyReliable:
         mDialog.makeResponse(mInvite200, mFirstRequest, code);
         mDum.send(mInvite200);
         startRetransmitTimer(); // 2xx timer
         transition(Connected);
         break;

      case UAS_SentUpdate:
         mDialog.makeResponse(mInvite200, mFirstRequest, code);
         mDum.send(mInvite200);
         startRetransmitTimer(); // 2xx timer
         transition(UAS_SentUpdateAccepted);
         break;

      case UAS_ReceivedUpdate:
         mDialog.makeResponse(mInvite200, mFirstRequest, code);// queue 2xx
         transition(UAS_ReceivedUpdateWaitingAnswer);
         break;
         
      case UAS_FirstSentOfferReliable:
      case UAS_NoOfferReliable:
      case UAS_OfferReliable: 
      case UAS_Accepted:
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

void
ServerInviteSession::targetRefresh (const NameAddr& localUri)
{
   WarningLog (<< "Can't refresh before Connected");
   assert(0);
   throw UsageUseException("Can't refresh before Connected", __FILE__, __LINE__);
}

void 
ServerInviteSession::refer(const NameAddr& referTo)
{
   WarningLog (<< "Can't refer before Connected");
   assert(0);
   throw UsageUseException("REFER not allowed in this context", __FILE__, __LINE__);
}

void 
ServerInviteSession::refer(const NameAddr& referTo, InviteSessionHandle sessionToReplace)
{
   WarningLog (<< "Can't refer before Connected");
   assert(0);
   throw UsageUseException("REFER not allowed in this context", __FILE__, __LINE__);
}

void 
ServerInviteSession::info(const Contents& contents)
{
   WarningLog (<< "Can't send INFO before Connected");
   assert(0);
   throw UsageUseException("Can't send INFO before Connected", __FILE__, __LINE__);
}

void
ServerInviteSession::sendProvisional(int code)
{
   mDialog.makeResponse(mFirstRequest, m1xx, code);
   if (mEarlyMedia.get())
   {
      setSdp(m1xx, *mEarlyMedia);
   }
   mDum.send(m1xx);
}

void
ServerInviteSession::sendUpdate(const SdpContents& sdp)
{
   if (peerSupportsUpdateMethod())
   {
      SipMessage update;
      mDialog.makeRequest(update, UPDATE);
      InviteSession::setSdp(update, sdp);
      mDum.send(update);
   }
   else
   {
      throw UsageUseException("Can't send UPDATE to peer", __FILE__, __LINE__);
   }
}


//////////////////////////////////////////
// OLD code follows
//////////////////////////////////////////
#if 0
void
ServerInviteSession::end()
{
   InfoLog ( << "ServerInviteSession::end" );  
   switch (mState)
   {
      case Terminated: 
      case Connected:
      case ReInviting:
         InviteSession::end();
         break;
      default:
         send(reject(410));
   }
}

void 
ServerInviteSession::send(SipMessage& msg)
{
   Destroyer::Guard guard(mDestroyer);
   if (mState == Connected || mState == Terminated || mState == ReInviting)
   {
      InviteSession::send(msg);
      return;
   }

   //!dcm! -- not considering prack, so offer/answer only happens in 2xx
   if(msg.isResponse())
   {
      int code = msg.header(h_StatusLine).statusCode();
      if (code < 200)
      {
         mDum.send(msg);
         msg.releaseContents();  //!dcm! -- maybe?         
      }
      else if (code < 300)
      {
         mState = Connected;         
         if (msg.header(h_CSeq).method() == INVITE)
         {
            InviteSession::send(msg);
            if (mOfferState == Answered)
            {
               mUserConnected = true;            
               mDum.mInviteSessionHandler->onConnected(getSessionHandle(), msg);
            }
         }
         else
         {
            mDum.send(msg);
         }
      }
      else 
      {
         mDum.send(msg);
         mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), msg);      
         guard.destroy();
      }
   }
   else
   {
      //!dcm!-- accepting logic is in InviteSession(merge w/ reinvite),
      //so no requests should be legal, which makes this user error? UPDATE eventually?
      throw UsageUseException("No request legal in this context.", __FILE__, __LINE__);
   }
}

SipMessage& 
ServerInviteSession::provisional(int statusCode)
{
   mDialog.makeResponse(mLastResponse, mLastIncomingRequest, statusCode);
   return mLastResponse;
}

SipMessage& 
ServerInviteSession::reject(int statusCode)
{
   mDialog.makeResponse(mLastResponse, mLastIncomingRequest, statusCode);
   return mLastResponse;
}

SipMessage& 
ServerInviteSession::accept()
{
   return makeFinalResponse(200);
}

void 
ServerInviteSession::dispatch(const SipMessage& msg)
{
   Destroyer::Guard guard(mDestroyer);
   std::pair<OfferAnswerType, const SdpContents*> offans;
   offans = InviteSession::getOfferOrAnswer(msg);
   if (msg.isRequest())
   {
      switch(mState)
      {
         case Initial:
            mLastIncomingRequest.releaseContents();  //!dcm! -- not sure, but seems right
            assert(msg.header(h_RequestLine).method() == INVITE);
            mState = Proceeding;
            mDum.mInviteSessionHandler->onNewSession(getHandle(), offans.first, msg);
            if (guard.destroyed())
            {
               return;
            }

            if (offans.first == Offer)
            {
               InviteSession::incomingSdp(msg, offans.second);
            }
            else
            {
               mDum.mInviteSessionHandler->onOfferRequired(getSessionHandle(), msg);
            }
            break;            
         case Proceeding:
            // !jf! consider UPDATE method
            if (msg.header(h_RequestLine).method() == CANCEL)
            {
               mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), msg);               
               mDialog.makeResponse(mLastResponse, msg, 200);
               mDum.send(mLastResponse);
               mDialog.makeResponse(mLastResponse, mLastIncomingRequest, 487);         
               mDum.send(mLastResponse);
               guard.destroy();
            }
            // RFC3261 - section 15 indicates callers UA can send a BYE on early dialogs
            else if (msg.header(h_RequestLine).method() == BYE)  
            {
               mState = Terminated;
               mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), msg);
               mDialog.makeResponse(mLastResponse, msg, 200);
               send(mLastResponse);
               break;
            }
            else
            {
               assert(0);  //!dcm! -- throw, toss away, inform other endpoint?
            }
            break;
      default:
         InviteSession::dispatch(msg);
      }
   }
   else
   {
      InviteSession::dispatch(msg);
   }
}
#endif


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
