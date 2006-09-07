#include "resiprocate/SipMessage.hxx"
#include "resiprocate/MethodTypes.hxx"
#include "resiprocate/dum/ServerPagerMessage.hxx"
#include "resiprocate/dum/OutOfDialogHandler.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/PagerMessageHandler.hxx"
#include "resiprocate/dum/InternalServerPagerMessage.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

ServerPagerMessageHandle 
ServerPagerMessage::getHandle()
{
   return ServerPagerMessageHandle(mDum, getBaseHandle().getId());
}

ServerPagerMessage::ServerPagerMessage(DialogUsageManager& dum,
                                       DialogSet& dialogSet,
                                       const SipMessage& req) : 
   NonDialogUsage(dum, dialogSet),
   mRequest(req)
{
}

ServerPagerMessage::~ServerPagerMessage()
{
   mDialogSet.mServerPagerMessage = 0;
}


void
ServerPagerMessage::endAsync()
{
   InfoLog(<< "End server pager message Async");
   mDum.post(new InternalServerPagerMessage_End(getHandle()));
}

void
ServerPagerMessage::sendAsync(const SipMessage& msg)
{
   InfoLog(<< "Send server pager message Async");
   mDum.post(new InternalServerPagerMessage_Send(getHandle(), msg));
}

void
ServerPagerMessage::sendAsync(bool accept, int statusCode)
{
   InfoLog(<< "Send server pager message Async(2)");
   mDum.post(new InternalServerPagerMessage_Send(getHandle(), accept, statusCode));
}

void
ServerPagerMessage::dispatchAsync(const SipMessage& msg)
{
   InfoLog(<< "Disptach server pager message Async");
   mDum.post(new InternalServerPagerMessage_DispatchSipMsg(getHandle(), msg));
}

void
ServerPagerMessage::dispatchAsync(const DumTimeout& timer)
{
   InfoLog(<< "Disptach server pager message timer Async: " << timer);
   mDum.post(new InternalServerPagerMessage_DispatchTimeoutMsg(getHandle(), timer));
}

void
ServerPagerMessage::end()
{
   delete this;
}

void 
ServerPagerMessage::dispatch(const SipMessage& msg)
{
	assert(msg.isRequest());
    ServerPagerMessageHandler* handler = mDum.mServerPagerMessageHandler;
    
    //?dcm? check in DialogUsageManager
    if (!handler)
    {
       mDum.makeResponse(mResponse, msg, 405);
       mDum.send(mResponse);
       delete this;
       return;
    }
    handler->onMessageArrived(getHandle(), msg);
}

void
ServerPagerMessage::dispatch(const DumTimeout& msg)
{
}

void 
ServerPagerMessage::send(SipMessage& response)
{
   assert(response.isResponse());
   mDum.send(response);
   delete this;
}

SipMessage& 
ServerPagerMessage::accept(int statusCode)
{   
   //!dcm! -- should any responses include a contact?
   mDum.makeResponse(mResponse, mRequest, statusCode);
   mResponse.remove(h_Contacts);   
   return mResponse;
}

SipMessage& 
ServerPagerMessage::reject(int statusCode)
{
   //!dcm! -- should any responses include a contact?
   mDum.makeResponse(mResponse, mRequest, statusCode);
   return mResponse;
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
