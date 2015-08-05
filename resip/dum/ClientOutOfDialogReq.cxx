#include "resip/stack/SipMessage.hxx"
#include "resip/stack/MethodTypes.hxx"
#include "resip/dum/ClientOutOfDialogReq.hxx"
#include "resip/dum/OutOfDialogHandler.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/Dialog.hxx"
#include "rutil/Logger.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

ClientOutOfDialogReqHandle 
ClientOutOfDialogReq::getHandle()
{
   return ClientOutOfDialogReqHandle(mDum, getBaseHandle().getId());
}

ClientOutOfDialogReq::ClientOutOfDialogReq(DialogUsageManager& dum,
                                           DialogSet& dialogSet,
                                           const SipMessage& req)
   : NonDialogUsage(dum, dialogSet), 
     mRequest(req)
{
}

ClientOutOfDialogReq::~ClientOutOfDialogReq()
{
   mDialogSet.mClientOutOfDialogRequests.remove(this);
}

void
ClientOutOfDialogReq::end()
{
   delete this;
}

void 
ClientOutOfDialogReq::dispatch(const SipMessage& msg)
{
	resip_assert(msg.isResponse());
    
    if (msg.header(h_StatusLine).statusCode() >= 200)
    {
       OutOfDialogHandler *handler = mDum.getOutOfDialogHandler(msg.header(h_CSeq).method());
       if(handler != NULL)
       {
          
          if(msg.header(h_StatusLine).statusCode() >= 200 && msg.header(h_StatusLine).statusCode() < 300)
          {
             // Pass Response to Handler
             DebugLog ( << "ClientOutOfDialogReq::dispatch - handler found for " 
                        << getMethodName(msg.header(h_CSeq).method()) 
                        << " method success response.");   
             handler->onSuccess(getHandle(), msg);  
          }
          else
          {
             // Pass Response to Handler
             DebugLog ( << "ClientOutOfDialogReq::dispatch - handler found for " 
                      << getMethodName(msg.header(h_CSeq).method()) 
                        << " method failure response.");   
             handler->onFailure(getHandle(), msg);  
          }
       }
       else
       {
          DebugLog ( << "ClientOutOfDialogReq::dispatch - handler not found for " 
                     << getMethodName(msg.header(h_CSeq).method()) 
                     << " method response.");   
       }

       delete this;
    }
    else
    {
       // Wait for final response
       DebugLog ( << "ClientOutOfDialogReq::dispatch - encountered provisional response" << msg.brief() );
    }    
}

const SipMessage&
ClientOutOfDialogReq::getRequest() const
{
   return mRequest;
}

void 
ClientOutOfDialogReq::dispatch(const DumTimeout& timer)
{
}

bool 
ClientOutOfDialogReq::matches(const SipMessage& msg) const
{
   return (DialogSetId(mRequest) == DialogSetId(msg));
}

EncodeStream& 
ClientOutOfDialogReq::dump(EncodeStream& strm) const
{
   strm << "ClientOutOfDialogReq " << getMethodName(mRequest.header(h_RequestLine).method()) 
        << " cseq=" << mRequest.header(h_CSeq).sequence();
   return strm;
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
