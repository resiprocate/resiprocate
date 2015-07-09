#include "resip/stack/SipMessage.hxx"
#include "resip/stack/MethodTypes.hxx"
#include "resip/dum/ServerPagerMessage.hxx"
#include "resip/dum/OutOfDialogHandler.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/PagerMessageHandler.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

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
   mRequest(req),
   mResponse(new SipMessage)
{
}

ServerPagerMessage::~ServerPagerMessage()
{
   mDialogSet.mServerPagerMessage = 0;
}


void
ServerPagerMessage::end()
{
   delete this;
}

class ServerPagerMessageEndCommand : public DumCommandAdapter
{
public:
   ServerPagerMessageEndCommand(const ServerPagerMessageHandle& serverPagerMessageHandle)
      : mServerPagerMessageHandle(serverPagerMessageHandle)
   {

   }

   virtual void executeCommand()
   {
      if(mServerPagerMessageHandle.isValid())
      {
         mServerPagerMessageHandle->end();
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "ServerPagerMessageEndCommand";
   }
private:
   ServerPagerMessageHandle mServerPagerMessageHandle;
};

void ServerPagerMessage::endCommand()
{
   mDum.post(new ServerPagerMessageEndCommand(getHandle()));
}

void 
ServerPagerMessage::dispatch(const SipMessage& msg)
{
	resip_assert(msg.isRequest());
    ServerPagerMessageHandler* handler = mDum.mServerPagerMessageHandler;
    
    //?dcm? check in DialogUsageManager
    if (!handler)
    {
       mDum.makeResponse(*mResponse, msg, 405);
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
ServerPagerMessage::send(SharedPtr<SipMessage> response)
{
   resip_assert(response->isResponse());
   mDum.send(response);
   delete this;
}

SharedPtr<SipMessage>
ServerPagerMessage::accept(int statusCode)
{   
   //!dcm! -- should any responses include a contact?
   mDum.makeResponse(*mResponse, mRequest, statusCode);
   mResponse->remove(h_Contacts);   
   return mResponse;
}

class ServerPagerMessageAcceptCommand : public DumCommandAdapter
{
public:
   ServerPagerMessageAcceptCommand(const ServerPagerMessageHandle& serverPagerMessageHandle, int statusCode)
      : mServerPagerMessageHandle(serverPagerMessageHandle),
        mStatusCode(statusCode)
   {
   }

   virtual void executeCommand()
   {
      if(mServerPagerMessageHandle.isValid())
      {
         mServerPagerMessageHandle->send(mServerPagerMessageHandle->accept(mStatusCode));
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "ServerPagerMessageAcceptCommand";
   }
private:
   ServerPagerMessageHandle mServerPagerMessageHandle;
   int mStatusCode;
};

void
ServerPagerMessage::acceptCommand(int statusCode)
{   
   mDum.post(new ServerPagerMessageAcceptCommand(getHandle(), statusCode));
}

SharedPtr<SipMessage>
ServerPagerMessage::reject(int statusCode)
{
   //!dcm! -- should any responses include a contact?
   mDum.makeResponse(*mResponse, mRequest, statusCode);
   return mResponse;
}

class ServerPagerMessageRejectCommand : public DumCommandAdapter
{
public:
   ServerPagerMessageRejectCommand(const ServerPagerMessageHandle& serverPagerMessageHandle, int statusCode)
      : mServerPagerMessageHandle(serverPagerMessageHandle),
        mStatusCode(statusCode)
   {
   }

   virtual void executeCommand()
   {
      if(mServerPagerMessageHandle.isValid())
      {
         mServerPagerMessageHandle->send(mServerPagerMessageHandle->reject(mStatusCode));
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "ServerPagerMessageRejectCommand";
   }
private:
   ServerPagerMessageHandle mServerPagerMessageHandle;
   int mStatusCode;
};

void
ServerPagerMessage::rejectCommand(int statusCode)
{
   mDum.post(new ServerPagerMessageRejectCommand(getHandle(), statusCode));
}

EncodeStream& 
ServerPagerMessage::dump(EncodeStream& strm) const
{
   strm << "ServerPagerMessage ";
   mRequest.encodeBrief(strm);
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
