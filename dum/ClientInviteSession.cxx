#include "resiprocate/SdpContents.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/DumTimeout.hxx"
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
            mDum.addTimer(DumTimeout::StaleCall, DumTimeout::StaleCallTimeout, getBaseHandle(),  ++mStaleCallTimerSeq);
            mState = Proceeding;
            mDum.mInviteSessionHandler->onNewSession(getHandle(), None, msg);
         }
         else if (code < 200)
         {
            mDum.addTimer(DumTimeout::StaleCall, DumTimeout::StaleCallTimeout, getBaseHandle(),  ++mStaleCallTimerSeq);
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
            //!dcm! -- pretty sure the following timer was bogus
//            mDum.addTimer(DumTimeout::StaleCall, DumTimeout::StaleCallTimeout, getBaseHandle(),  ++mStaleCallTimerSeq);
            ++mStaleCallTimerSeq;  //unifies timer handling logic

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
               mDum.addTimer(DumTimeout::StaleCall, DumTimeout::StaleCallTimeout, getBaseHandle(), ++mStaleCallTimerSeq);
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
               //!dcm! -- pretty sure the following timer was bogus
//            mDum.addTimer(DumTimeout::StaleCall, DumTimeout::StaleCallTimeout, getBaseHandle(),  ++mStaleCallTimerSeq);
               ++mStaleCallTimerSeq;  //unifies timer handling logic
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
      case Cancelled:
      {
         if (msg.isResponse())
         {
            int code = msg.header(h_StatusLine).statusCode();
            if (code / 100 == 2 && msg.header(h_CSeq).method() == INVITE)
            {
               //!dcm! -- ack the crossover 200?
               mState = Connected;               
               send(end());
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
      SipMessage& notify = (code >= 200) ? mServerSub->end(NoResource, &contents) : mServerSub->update(&contents);
//      mDum.mInviteSessionHandler->onReadyToSend(getSessionHandle(), notify);
      mServerSub->send(notify);
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
         mServerSub->send(mServerSub->end(NoResource, &contents));
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

SipMessage&
ClientInviteSession::end()
{
   switch (mState)
   {
      case Early:
         //if there is no fork, CANCEL, if there is a fork send a BYE
         if (mDialog.mDialogSet.mDialogs.size() > 1)
         {
            InfoLog ( << "ClientInviteSession::end, Early(forking)" );        
            mDialog.makeRequest(mLastRequest, BYE);
            assert(mLastRequest.header(h_Vias).size() == 1);
            mLastRequest.header(h_Vias).front().param(p_branch).reset();
            mState = Terminated;
            return mLastRequest;
         }         
      case Initial:
         InfoLog ( << "ClientInviteSession::end, Early/Initial)" );        
         mDialog.makeCancel(mLastRequest);
         //!dcm! -- it could be argued that this(and similar) should happen in send so users
         //can't toast themselves
         mState = Cancelled;
         return mLastRequest;
         break;
      case Terminated: 
      case Connected:
      case ReInviting:
         InfoLog ( << "ClientInviteSession::end, Terminated/Connected/ReInviting)" );        
         return InviteSession::end();
         break;
      case Cancelled: //user error
         InfoLog ( << "ClientInviteSession::end, Cannot end a session that has already been cancelled.)" );        
         throw UsageUseException("Cannot end a session that has already been cancelled.", __FILE__, __LINE__);
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

#if 0 //?dcm? --PRACKISH dispatch, or just cruft?
// void
// ClientInviteSession::dispatch(const SipMessage& msg)
// {
//    InviteSessionHandler* handler = mDum.mInviteSessionHandler;
//    assert(handler);
   
//    if (msg.isRequest())
//    {
//       InviteSession::dispatch(msg);
//       return;
//    }
//    else if (msg.isResponse())
//    {
//       switch (msg.header(h_CSeq).method())
//       {
//          case INVITE:
//             break;
            
//          case PRACK:
//             handlePrackResponse(msg);
//             return;
            
//          case CANCEL:
//             if (msg.header(h_StatusLine).statusCode() >= 400)
//             {
//                mState = Terminated;
//                end(); // cleanup the mess
//             }
//             return;            
            
//          default:
//             InviteSession::dispatch(msg);
//             return;
//       }
//    }
   
//    int code = msg.header(h_StatusLine).statusCode();
//    if (code < 300 && mState == Initial)
//    {
//       //handler->onNewSession(getHandle(), msg);
//    }
         
//    if (code < 200) // 1XX
//    {
//       if (mState == Initial || mState == Early)
//       {
//          mState = Early;
//          //handler->onEarly(getHandle(), msg);
            
//          SdpContents* sdp = dynamic_cast<SdpContents*>(msg.getContents());
//          bool reliable = msg.header(h_Supporteds).find(Token(Symbols::C100rel));
//          if (sdp)
//          {
//             if (reliable)
//             {
//                if (mProposedLocalSdp)
//                {
//                   mCurrentRemoteSdp = static_cast<SdpContents*>(sdp->clone());
//                   mCurrentLocalSdp = mProposedLocalSdp;
//                   mProposedLocalSdp = 0;

//                   //handler->onAnswer(getHandle(), msg);
//                }
//                else
//                {
//                   mProposedRemoteSdp = static_cast<SdpContents*>(sdp->clone());
//                   handler->onOffer(getSessionHandle(), msg);

//                   // handler must provide an answer
//                   assert(mProposedLocalSdp);
//                }
//             }
//             else
//             {
//                // do nothing, not an offer/answer
//             }
//          }
//          if (reliable)
//          {
//             sendPrack(msg);
//          }
//       }
//       else
//       {
//          // drop it on the floor. Late 1xx
//       }
//    }
//    else if (code < 300) // 2XX
//    {
//       if (mState == Cancelled)
//       {
//          //sendAck(the200);  
//          end();
//          return;
//       }
//       else if (mState != Terminated)
//       {
//          mState = Connected;
//          // !jf!
//          //if (mReceived2xx) // retransmit ACK
//          {
//             mDum.send(mAck);
//             return;
//          }
         
//          //mReceived2xx = true;
//          handler->onConnected(getHandle(), msg);
            
//          SdpContents* sdp = dynamic_cast<SdpContents*>(msg.getContents());
//          if (sdp)
//          {
//             if (mProposedLocalSdp) // got an answer
//             {
//                mCurrentRemoteSdp = static_cast<SdpContents*>(sdp->clone());
//                mCurrentLocalSdp = mProposedLocalSdp;
//                mProposedLocalSdp = 0;
                  
//                //handler->onAnswer(getHandle(), msg);
//             }
//             else  // got an offer
//             {
//                mProposedRemoteSdp = static_cast<SdpContents*>(sdp->clone());
//                handler->onOffer(getSessionHandle(), msg);
//             }
//          }
//          else
//          {
//             if (mProposedLocalSdp)
//             {
//                // Got a 2xx with no answer (sent an INVITE with an offer,
//                // unreliable provisionals)
//                end();
//                return;
//             }
//             else if (mCurrentLocalSdp == 0 && mProposedRemoteSdp == 0)
//             {        Transport::error( e );
//                InfoLog(<< "Unable to route to " << target << " : [" << e << "] " << strerror(e) );
//                throw Transport::Exception("Can't find source address for Via", __FILE__,__LINE__);
//                // Got a 2xx with no offer (sent an INVITE with no offer,
//                // unreliable provisionals)
//                end();
//                return;
//             }
//             else
//             {
//                assert(mCurrentLocalSdp != 0);
//                // do nothing
//             }
//          }
//          sendAck(msg);
//       }
//    }
//    else if (code >= 400)
//    {
//       if (mState != Terminated)
//       {
//          mState = Terminated;
//          handler->onTerminated(getSessionHandle(), msg);
//                   guard.destroy();
//       }
//    }
//    else // 3xx
//    {
//       assert(0);
//    }
// }
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
