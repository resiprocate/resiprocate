#include "resip/dum/SubscriptionHandler.hxx"
#include "resip/dum/ServerSubscription.hxx"
#include "resip/dum/ClientSubscription.hxx"
#include "resip/stack/SecurityAttributes.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

static Mimes empty;

void 
ClientSubscriptionHandler::onReadyToSend(ClientSubscriptionHandle h, SipMessage& msg)
{
   // default is to do nothing. this is for adornment
}

void 
ClientSubscriptionHandler::onNotifyNotReceived(ClientSubscriptionHandle h)
{
   // By default, tear down the sub.
   h->end();
}

void 
ClientSubscriptionHandler::onFlowTerminated(ClientSubscriptionHandle h)
{
   // re-Subscribe
   InfoLog(<< "ClientSubscriptionHandler::onFlowTerminated");
   h->reSubscribe();
}

const Mimes& 
ServerSubscriptionHandler::getSupportedMimeTypes() const
{
   return empty;
}

void 
ServerSubscriptionHandler::onError(ServerSubscriptionHandle, const SipMessage& msg)
{
}

void 
ServerSubscriptionHandler::onExpiredByClient(ServerSubscriptionHandle, const SipMessage& sub, SipMessage& notify)
{
}

void 
ServerSubscriptionHandler::onExpired(ServerSubscriptionHandle, SipMessage& notify)
{
}

//!dcm! -- a bit clunky, but really want these objects to not have state
bool 
ServerSubscriptionHandler::hasDefaultExpires() const
{
   return false;
}

UInt32 
ServerSubscriptionHandler::getDefaultExpires() const
{
   return 0;
}

bool 
ServerSubscriptionHandler::hasMinExpires() const
{
   return false;
}

UInt32 
ServerSubscriptionHandler::getMinExpires() const
{
   return 0;
}
bool 
ServerSubscriptionHandler::hasMaxExpires() const
{
   return false;
}

UInt32 
ServerSubscriptionHandler::getMaxExpires() const
{
   return 0;
}

void
ServerSubscriptionHandler::getExpires(const SipMessage &msg, UInt32 &expires, int &errorResponseCode)
{
   if (msg.exists(h_Expires))
   {         
      expires = msg.header(h_Expires).value();

      if (expires > 0)
      {
         if (hasMinExpires() && (expires < getMinExpires()))
         {
            errorResponseCode = 423;
         }
         else if (hasMaxExpires() && (expires > getMaxExpires()))
         {
            expires = getMaxExpires();
         }
      }
   }
   else if (hasDefaultExpires())
   {
      expires = getDefaultExpires();
   }
   else
   {
      errorResponseCode = 400;	   
   }
}

void 
ServerSubscriptionHandler::onRefresh(ServerSubscriptionHandle handle, const SipMessage& sub)
{
   handle->send(handle->accept(200));
   handle->send(handle->neutralNotify());
}

void 
ServerSubscriptionHandler::onPublished(ServerSubscriptionHandle associated, 
                                       ServerPublicationHandle publication, 
                                       const Contents* contents,
                                       const SecurityAttributes* attrs)
{
   // do nothing by default
}

void 
ServerSubscriptionHandler::onNotifyAccepted(ServerSubscriptionHandle h, const SipMessage& msg)
{
}

void 
ServerSubscriptionHandler::onNotifyRejected(ServerSubscriptionHandle h, const SipMessage& msg)
{
}

void 
ServerSubscriptionHandler::onReadyToSend(ServerSubscriptionHandle h, SipMessage& msg)
{
   // default is to do nothing. this is for adornment   
}

void
ServerSubscriptionHandler::onNewSubscriptionFromRefer(ServerSubscriptionHandle, const SipMessage& sub)
{
}

void 
ServerSubscriptionHandler::onFlowTerminated(ServerSubscriptionHandle h)
{
   InfoLog(<< "ServerSubscriptionHandler::onFlowTerminated");
   // By default, tear down the sub.
   h->end();
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
