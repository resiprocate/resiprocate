#include "resip/stack/SipMessage.hxx"
#include "resip/stack/MethodTypes.hxx"
#include "resip/dum/ServerOutOfDialogReq.hxx"
#include "resip/dum/OutOfDialogHandler.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

ServerOutOfDialogReqHandle 
ServerOutOfDialogReq::getHandle()
{
   return ServerOutOfDialogReqHandle(mDum, getBaseHandle().getId());
}


ServerOutOfDialogReq::ServerOutOfDialogReq(DialogUsageManager& dum,
                                           DialogSet& dialogSet,
                                           const SipMessage& req)
   : NonDialogUsage(dum, dialogSet),
     mResponse(new SipMessage)
{

}

ServerOutOfDialogReq::~ServerOutOfDialogReq()
{
   mDialogSet.mServerOutOfDialogRequest = 0;
}

void
ServerOutOfDialogReq::end()
{
   delete this;
}

void 
ServerOutOfDialogReq::dispatch(const SipMessage& msg)
{
   resip_assert(msg.isRequest());

   OutOfDialogHandler *pHandler = mDum.getOutOfDialogHandler(msg.header(h_CSeq).method());
   if(pHandler != NULL)
   {
      // Let handler deal with message
      mRequest = msg; 
      DebugLog ( << "ServerOutOfDialogReq::dispatch - handler found for " << getMethodName(msg.header(h_CSeq).method()) << " method.");   
      pHandler->onReceivedRequest(getHandle(), msg);  // Wait for application to send response
   }
   else
   {
      if(msg.header(h_CSeq).method() == OPTIONS)
      {
         DebugLog ( << "ServerOutOfDialogReq::dispatch - handler not found for OPTIONS - sending autoresponse.");   
         // If no handler exists for OPTIONS then handle internally
         mRequest = msg; 
         mDum.send(answerOptions());
         delete this;
      }
      else
      {
         DebugLog ( << "ServerOutOfDialogReq::dispatch - handler not found for " << getMethodName(msg.header(h_CSeq).method()) << " method - sending 405.");   
         // No handler found for out of dialog request - return a 405
         mDum.makeResponse(*mResponse, msg, 405);
         mDum.send(mResponse);
         delete this;
      }
   }
}

void
ServerOutOfDialogReq::dispatch(const DumTimeout& msg)
{
}

SharedPtr<SipMessage>
ServerOutOfDialogReq::answerOptions()
{
   mDum.makeResponse(*mResponse, mRequest, 200);

   // Add in Allow, Accept, Accept-Encoding, Accept-Language, and Supported Headers from Profile
   mResponse->header(h_Allows) = mDum.getMasterProfile()->getAllowedMethods();
   mResponse->header(h_Accepts) = mDum.getMasterProfile()->getSupportedMimeTypes(INVITE);
   mResponse->header(h_AcceptEncodings) = mDum.getMasterProfile()->getSupportedEncodings();
   mResponse->header(h_AcceptLanguages) = mDum.getMasterProfile()->getSupportedLanguages();
   mResponse->header(h_AllowEvents) = mDum.getMasterProfile()->getAllowedEvents();
   mResponse->header(h_Supporteds) = mDum.getMasterProfile()->getSupportedOptionTags();

   return mResponse;
}

void 
ServerOutOfDialogReq::send(SharedPtr<SipMessage> response)
{
   resip_assert(response->isResponse());
   mDum.send(response);
   delete this;
}

SharedPtr<SipMessage>
ServerOutOfDialogReq::accept(int statusCode)
{   
   //!dcm! -- should any responses should include a contact?
   mDum.makeResponse(*mResponse, mRequest, statusCode);
   return mResponse;
}

SharedPtr<SipMessage>
ServerOutOfDialogReq::reject(int statusCode)
{
   //!dcm! -- should any responses should include a contact?
   mDum.makeResponse(*mResponse, mRequest, statusCode);
   return mResponse;
}

EncodeStream& 
ServerOutOfDialogReq::dump(EncodeStream& strm) const
{
   if(mRequest.exists(h_CSeq))
   {
      strm << "ServerOutOfDialogReq " << getMethodName(mRequest.header(h_RequestLine).method()) 
           << " cseq=" << mRequest.header(h_CSeq).sequence();
   }
   else
   {
      strm << "ServerOutOfDialogReq, dispatch has not occured yet.";
   }
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

