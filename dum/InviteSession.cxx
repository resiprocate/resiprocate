#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/InviteSession.hxx"

using namespace resip;

InviteSession::InviteSession(DialogUsageManager& dum, Dialog& dialog)
   : BaseUsage(dum, dialog),
     mOfferState(None),
     mCurrentLocalSdp(0),
     mCurrentRemoteSdp(0),
     mProposedLocalSdp(0),
     mProposedRemoteSdp(0)
{
}

SipMessage& 
InviteSession::getOfferOrAnswer()
{
   return mLastRequest;
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

SipMessage&
InviteSession::end()
{
   //assert(mState == Connected);

#if 0
   // no way for the application to modify the BYE yet
   SipMessage bye;
   mDialog.makeBye(bye);
   copyAuthorizations(bye);
   //mDum.send(bye);
#endif
   return mLastRequest;
}

// If sdp==0, it means the last offer failed
void 
InviteSession::incomingSdp(SdpContents* sdp)
{
   switch (mOfferState)
   {
      case None:
         assert(mCurrentLocalSdp == 0);
         assert(mCurrentRemoteSdp == 0);
         mProposedRemoteSdp = sdp;
         mOfferState = Offerred;
         break;
         
      case Offerred:
         mCurrentLocalSdp = mProposedLocalSdp;
         mCurrentRemoteSdp = sdp;
         mProposedLocalSdp = 0;
         mProposedRemoteSdp = 0;
         mOfferState = Answered;
         break;

      case Answered:
         assert(mProposedLocalSdp == 0);
         assert(mProposedRemoteSdp == 0);
         mProposedRemoteSdp = sdp;
         mOfferState = CounterOfferred;
         break;
         
         
      case CounterOfferred:
         assert(mCurrentLocalSdp);
         assert(mCurrentRemoteSdp);
         if (sdp)
         {
            mCurrentLocalSdp = mProposedLocalSdp;
            mCurrentRemoteSdp = sdp;
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

void
InviteSession::sendSdp(SdpContents* sdp)
{
   switch (mOfferState)
   {
      case None:
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
            mCurrentLocalSdp = sdp;
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

InviteSession::Handle::Handle(DialogUsageManager& dum)
   : BaseUsage::Handle(dum)
{}

InviteSession*
InviteSession::Handle::operator->()
{
   return static_cast<InviteSession*>(get());
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
