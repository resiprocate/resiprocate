#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/ServerInviteSession.hxx"

using namespace resip;

ServerInviteSession::ServerInviteSession(DialogUsageManager& dum, Dialog& dialog, const SipMessage& msg)
   : InviteSession(dum, dialog)
{
}

ServerInviteSessionHandle 
ServerInviteSession::getHandle()
{
   return ServerInviteSessionHandle(mDum, getBaseHandle().getId());
}

void 
ServerInviteSession::setOffer(const SdpContents* offer)
{
}

void 
ServerInviteSession::sendOfferInAnyMessage()
{
}

void 
ServerInviteSession::setAnswer(const SdpContents* answer)
{
}

void 
ServerInviteSession::sendAnswerInAnyMessage()
{
}

SipMessage&
ServerInviteSession::end()
{
   assert(0);
   return mLastRequest;
}

SipMessage& 
ServerInviteSession::rejectOffer(int statusCode)
{
   assert(0);
   return mLastRequest; // bogus
}

void 
ServerInviteSession::accept()
{
}

void 
ServerInviteSession::provisional(int statusCode)
{
}

void
ServerInviteSession::reject(int statusCode)
{
   assert(0);
}

void 
ServerInviteSession::handle(const SipMessage& msg)
{
}

void 
ServerInviteSession::dispatch(const SipMessage& msg)
{
}

void 
ServerInviteSession::dispatch(const DumTimeout& timer)
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
