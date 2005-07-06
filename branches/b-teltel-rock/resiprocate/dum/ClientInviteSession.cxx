#include "resiprocate/SdpContents.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/DumTimeout.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/ServerInviteSession.hxx"
#include "resiprocate/dum/ServerSubscription.hxx"
#include "resiprocate/dum/UsageUseException.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/SipFrag.hxx"

#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK)// Used for tracking down memory leaks in Visual Studio
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new   new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif // defined(WIN32) && defined(_DEBUG)

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

ClientInviteSession::ClientInviteSession(DialogUsageManager& dum, 
                                         Dialog& dialog,
                                         const SipMessage& request, 
                                         const SdpContents* initialOffer,
                                         ServerSubscriptionHandle serverSub) :
   InviteSession(dum, dialog, Initial),
   lastReceivedRSeq(0),
   lastExpectedRSeq(0),
   mStaleCallTimerSeq(1),
   mServerSub(serverSub)
{
   assert(request.isRequest());
   if (initialOffer)
   {
      sendSdp(static_cast<SdpContents*>(initialOffer->clone()));
   }
   mLastRequest = request;
   mLastRequest.releaseContents();   
}

ClientInviteSessionHandle 
ClientInviteSession::getHandle()
{
   return ClientInviteSessionHandle(mDum, getBaseHandle().getId());
}

void
ClientInviteSession::dispatch(const SipMessage& msg)
{
   Destroyer::Guard guard(mDestroyer);
   std::pair<OfferAnswerType, const SdpContents*> offans;
   offans = InviteSession::getOfferOrAnswer(msg);
   
   // !jf! consider UPDATE method

   switch(mState)
   {
      case Initial:
      {
         //!dcm! -- really can't do this assert, prob. kill dialog(erroneous
         //request) and send a 4xx, but which 4xx?
         assert(msg.isResponse());
         int code = msg.header(h_StatusLine).statusCode();         
         if (code == 100)
         {
            mDum.addTimer(DumTimeout::StaleCall, mDum.getProfile()->getDefaultStaleCallTime(), 
                          getBaseHandle(),  ++mStaleCallTimerSeq);
            mState = Proceeding;
            mDum.mInviteSessionHandler->onNewSession(getHandle(), None, msg);
         }
         else if (code < 200)
         {
            mDum.addTimer(DumTimeout::StaleCall, mDum.getProfile()->getDefaultStaleCallTime(), 
                          getBaseHandle(),  ++mStaleCallTimerSeq);
            mState = Early;
            mDum.mInviteSessionHandler->onNewSession(getHandle(), offans.first, msg);
            mDum.mInviteSessionHandler->onProvisional(getHandle(), msg);
            
            if (offans.first != None)
            {
               InviteSession::incomingSdp(msg, offans.second);
            }
            else if (offans.second)
            {
               mDum.mInviteSessionHandler->onEarlyMedia(getHandle(), msg, offans.second);
            }
         }
         else if (code < 300)
         {
            sendSipFrag(msg);            
            ++mStaleCallTimerSeq;  // call is not stale - increment timer Seq - so that when timer expires nothing happens

            // Handle any Session Timer headers in response
            handleSessionTimerResponse(msg);

            mState = Connected;
            mDum.mInviteSessionHandler->onNewSession(getHandle(), offans.first, msg);
            mUserConnected = true;            
            mDum.mInviteSessionHandler->onConnected(getHandle(), msg);
            
            if (offans.first == Answer)
            {
               //no late media required, so just send the ACK
               send(makeAck());
            }
            if (offans.first != None)
            {
               InviteSession::incomingSdp(msg, offans.second);
            }
         }
         else if (code >= 300)
         {
            sendSipFrag(msg);            
            mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), msg);
            guard.destroy();
         }
         break;
      }
      
      case Forked:     
      case Proceeding:
      case Early:
      {
         if (msg.isResponse())
         {
            int code = msg.header(h_StatusLine).statusCode();
            if (code == 100)
            {
            }
            else if (code < 200)
            {
               mDum.addTimer(DumTimeout::StaleCall, mDum.getProfile()->getDefaultStaleCallTime(), getBaseHandle(), ++mStaleCallTimerSeq);
               mState = Early;
               mDum.mInviteSessionHandler->onProvisional(getHandle(), msg);
            
               if (offans.first != None)
               {
                  InviteSession::incomingSdp(msg, offans.second);
               }
               else if (offans.second)
               {
                  mDum.mInviteSessionHandler->onEarlyMedia(getHandle(), msg, offans.second);
               }
            }
            else if (code < 300)
            {
               sendSipFrag(msg);            
               ++mStaleCallTimerSeq;  // call is not stale - increment timer Seq - so that when timer expires nothing happens
               mState = Connected;

               // Handle any Session Timer headers in response
               handleSessionTimerResponse(msg);

               mUserConnected = true;            
               mDum.mInviteSessionHandler->onConnected(getHandle(), msg);
            
               if (offans.first != None)
               {
                  InviteSession::incomingSdp(msg, offans.second);
               }
               if (mOfferState == Answered)
               {
                  //no late media required, so just send the ACK
                  send(makeAck());
               }
            }
            else if (code >= 300)
            {
               sendSipFrag(msg);            
               mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), msg);
               guard.destroy();
            }
         }
         break;
      }
      //!dcm! -- cancel handling needs work
      case IgnoreFork:
         if (msg.isResponse())
         {
            int code = msg.header(h_StatusLine).statusCode();
            if (code / 100 == 2 && msg.header(h_CSeq).method() == INVITE)
            {
               //!dcm! -- ack the crossover 200?
               mState = Connected;               
               end();
            }
            else if (code >= 300 && msg.header(h_CSeq).method() == INVITE)
            {
	           mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), msg);
               guard.destroy();
            }
         }
         break;         
      case Cancelled:
      {
         if (msg.isResponse())
         {
            int code = msg.header(h_StatusLine).statusCode();
            if (code / 100 == 2 && msg.header(h_CSeq).method() == INVITE)
            {
               //!dcm! -- ack the crossover 200?
               sendSipFrag(msg); //strange refer race
               mState = Connected;               
               end();
            }
            else if (code >= 300 && msg.header(h_CSeq).method() == INVITE)
            {
               sendSipFrag(msg);            
	           mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), msg);
               guard.destroy();
            }
         }
         break;
      }
      default:
         InviteSession::dispatch(msg);
   }
}

void
ClientInviteSession::dispatch(const DumTimeout& timeout)
{
   Destroyer::Guard guard(mDestroyer);
   if (timeout.type() == DumTimeout::StaleCall 
       && timeout.seq() == mStaleCallTimerSeq)
   {
      mDum.mInviteSessionHandler->onStaleCallTimeout(getHandle());
      end();  // Terminate call
   }
   else if (timeout.type() == DumTimeout::Forked && (mState == Forked || mState == IgnoreFork))
   {
      mDum.mInviteSessionHandler->onForkDestroyed(getHandle());
      guard.destroy();
   }
   else if(timeout.type() == DumTimeout::Cancelled)
   {
      SipMessage response;            
      mDialog.makeResponse(response, mLastRequest, 487);
      if (mServerSub.isValid())
      {
         sendSipFrag(response);
      }
      InviteSessionHandler* handler = mDum.mInviteSessionHandler;
      handler->onTerminated(getSessionHandle(), response);
      guard.destroy();
   }
   else
   {
      InviteSession::dispatch(timeout);      
   }
}

void 
ClientInviteSession::sendSipFrag(const SipMessage& response)
{
   if (mServerSub.isValid())
   {
      SipFrag contents;
      contents.message().header(h_StatusLine) = response.header(h_StatusLine);
      //will be cloned...ServerSub may not have the most efficient API possible
      int code = response.header(h_StatusLine).statusCode();
      if (code >= 200)
      {
         mServerSub->end(NoResource, &contents);
      }
      else
      {
         mServerSub->send(mServerSub->update(&contents));
      }
   }   
}

void
ClientInviteSession::send(SipMessage& msg)
{
   Destroyer::Guard guard(mDestroyer);
   //last ack logic lives in InviteSession(to be re-used for reinvite
   if (mState == Connected || mState == Terminated || mState == ReInviting)
   {
      InviteSession::send(msg);
      return;
   }

   if (msg.isRequest() && msg.header(h_RequestLine).method() == CANCEL)
   {
      mDum.send(msg);
      if (mServerSub.isValid())
      {
         SipFrag contents;
         contents.message().header(h_StatusLine).statusCode() = 487;
         contents.message().header(h_StatusLine).reason() = "Request Cancelled";
         //will be cloned...ServerSub may not have the most efficient API possible
         mServerSub->end(NoResource, &contents);
      }   
      mDum.mInviteSessionHandler->onTerminated(getSessionHandle(), msg);
      guard.destroy();
      return;
   }   

   //!dcm! -- strawman, no knowledge of prack, so just ack(handled in
   //InviteSession) and Invite(already done) for now complain bitterly
   if (mNextOfferOrAnswerSdp)
   {
      assert(0);
   }
   assert(msg.isRequest());    //!dcm! -- is this correct?   
   mLastRequest = msg;
   mDum.send(msg);
}

void
ClientInviteSession::end()
{
   switch (mState)
   {
      case Early:
         if (mDialog.mDialogSet.mDialogs.size() > 1)
         {
            InfoLog ( << "ClientInviteSession::end, Early(forking)" );        
            mDialog.makeRequest(mLastRequest, BYE);
            assert(mLastRequest.header(h_Vias).size() == 1);
            mLastRequest.header(h_Vias).front().param(p_branch).reset();
            mState = Terminated;
            send(mLastRequest);
         }
         else
         {
            mDialog.mDialogSet.end();
         }
         break;         
      case Initial:
#if 0
         //!dcm! -- don't think this code should be here/can ever be called
         InfoLog ( << "ClientInviteSession::end, Early/Initial)" );        
         mDialog.makeCancel(mLastRequest);
         //!dcm! -- it could be argued that this(and similar) should happen in send so users
         //can't toast themselves
         mState = Cancelled;
         send(mLastRequest);
#endif
         break;
      case Terminated: 
      case Connected:
      case ReInviting:
         InfoLog ( << "ClientInviteSession::end, Terminated/Connected/ReInviting)" );        
         InviteSession::end();
         break;
      case Cancelled: //user error
         InfoLog ( << "ClientInviteSession::end, Cannot end a session that has already been cancelled.)" );        
         throw UsageUseException("Cannot end a session that has already been cancelled.", __FILE__, __LINE__);
      case Forked:
         mState = IgnoreFork;
         break;
      default:
         InfoLog ( << "ClientInviteSession::end, Progammer error)" );        
         assert(false);//throw UsageUseException("Progammer error", __FILE__, __LINE__);
   }
}

//!dcm! -- probably kill
// void
// ClientInviteSession::sendAck(const SipMessage& ok)
// {
//    makeAck(ok);
//    if (mProposedLocalSdp)
//    {
//       // !jf! ?
//       //mDialog.setContents(mProposedLocalSdp);
//    }
//    mDum.send(mAck);
// }

//below here be the prack
void
ClientInviteSession::sendPrack(const SipMessage& response)
{
   assert(response.isResponse());
   assert(response.header(h_StatusLine).statusCode() > 100 && 
          response.header(h_StatusLine).statusCode() < 200);
   
   SipMessage prack;
   mDialog.makeRequest(prack, PRACK);
   
   if (mProposedRemoteSdp)
   {
      assert(mProposedLocalSdp);
      // send an answer
      prack.setContents(mProposedLocalSdp);
      
   }
   else if (mProposedLocalSdp)
   {
      // send a counter-offer
      prack.setContents(mProposedRemoteSdp);
   }
   else
   {
      // no sdp
   }
   
   // much later!!! the deep rathole ....
   // if there is a pending offer or answer, will include it in the PRACK body
   assert(0);
}

void
ClientInviteSession::handlePrackResponse(const SipMessage& response)
{
   // more PRACK goodness 
   assert(0);
}

void ClientInviteSession::redirected(const SipMessage& msg)
{
   if (mState == Initial || mState == Early || mState == Proceeding)
   {
      mDum.mInviteSessionHandler->onRedirected(getHandle(), msg);
      delete this;      
   }     
}

void 
ClientInviteSession::forked()   
{
   switch(mState)
   {
      case Initial:
      case Early:
      case Proceeding:         
         mState = Forked;
         mDum.addTimerMs(DumTimeout::Forked, Timer::TH, getBaseHandle(), 0);         
         break;
      default:
         return;         
   }
}

void 
ClientInviteSession::cancel()   
{
   switch(mState)
   {
      case Initial:
      case Early:
      case Proceeding:
         mState = Cancelled;
         mDum.addTimerMs(DumTimeout::Cancelled, Timer::TH, getBaseHandle(), 0);         
         break;
      default:
         return;         
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
