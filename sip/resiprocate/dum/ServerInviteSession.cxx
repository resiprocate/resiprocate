#include "resiprocate/SdpContents.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/DumTimeout.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/ServerInviteSession.hxx"
#include "resiprocate/dum/UsageUseException.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/os/Logger.hxx"


#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK)// Used for tracking down memory leaks in Visual Studio
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new   new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif // defined(WIN32) && defined(_DEBUG)

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

ServerInviteSession::ServerInviteSession(DialogUsageManager& dum, Dialog& dialog, const SipMessage& request)
   : InviteSession(dum, dialog, Initial)
{
   assert(request.isRequest());
   mLastIncomingRequest = request;   
}

ServerInviteSessionHandle 
ServerInviteSession::getHandle()
{
   return ServerInviteSessionHandle(mDum, getBaseHandle().getId());
}

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
