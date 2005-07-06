#include "resiprocate/SipMessage.hxx"
#include "resiprocate/MethodTypes.hxx"
#include "resiprocate/dum/PagerMessageCreator.hxx"
#include "resiprocate/dum/ClientPagerMessage.hxx"
#include "resiprocate/dum/PagerMessageHandler.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/UsageUseException.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

ClientPagerMessageHandle 
ClientPagerMessage::getHandle()
{
   return ClientPagerMessageHandle(mDum, getBaseHandle().getId());
}

ClientPagerMessage::ClientPagerMessage(DialogUsageManager& dum, DialogSet& dialogSet)
   : NonDialogUsage(dum, dialogSet),
     mRequest(dialogSet.getCreator()->getLastRequest()),
     mInTransaction(false)
{
}

ClientPagerMessage::~ClientPagerMessage()
{
   mDialogSet.mClientPagerMessage = 0;   
}

SipMessage& 
ClientPagerMessage::getMessageRequest()
{
   return mRequest;
}

void 
ClientPagerMessage::page(std::auto_ptr<Contents> contents)
{
   if (mInTransaction)
   {
      throw UsageUseException("Cannot send a MESSAGE until the previous MESSAGE transaction has completed", 
                                  __FILE__, __LINE__);
   }
   mRequest.header(h_CSeq).sequence()++;
   mInTransaction = true;
   mRequest.setContents(contents);
   DebugLog(<< "ClientPagerMessage::page: " << mRequest);
   mDum.send(mRequest);
}


void 
ClientPagerMessage::dispatch(const SipMessage& msg)
{
	assert(msg.isResponse());
    ClientPagerMessageHandler* handler = mDum.mClientPagerMessageHandler;
    assert(handler);
    int code = msg.header(h_StatusLine).statusCode();    

    if (code < 200)
    {
       DebugLog ( << "ClientPagerMessageReq::dispatch - encountered provisional response" << msg.brief() );
    }
    else if (code < 300)
    {
       handler->onSuccess(getHandle(), msg);  
       mInTransaction = false;
    }
    else
    {
       handler->onFailure(getHandle(), msg);
       mInTransaction = false;
    }
}

void 
ClientPagerMessage::dispatch(const DumTimeout& timer)
{
}

void
ClientPagerMessage::end()
{
   delete this;
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
