#include "resip/dum/ServerPagerMessage.hxx"

#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/DumShutdownHandler.hxx"
#include "resip/dum/PagerMessageHandler.hxx"

#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

namespace resip 
{


class TestClientRegistrationHandler : public ClientRegistrationHandler
{
   public:
      TestClientRegistrationHandler() 
      {
      }
      
      virtual ~TestClientRegistrationHandler()
      {
      }

      virtual void onSuccess(ClientRegistrationHandle,
                             const SipMessage& response)
      {
         InfoLog( << "TestClientRegistrationHandler::onSuccess" );
      }
      
      virtual void onRemoved(ClientRegistrationHandle, const SipMessage&)
      {
         InfoLog( << "TestClientRegistrationHander::onRemoved" );
         exit(-1);
      }
      
      virtual void onFailure(ClientRegistrationHandle,
                             const SipMessage& response)
      {
         InfoLog( << "TestClientRegistrationHandler::onFailure" );
         exit(-1);
      }

      virtual int onRequestRetry(ClientRegistrationHandle,
                                 int retrySeconds,
                                 const SipMessage& response)
      {
         InfoLog( << "TestClientRegistrationHandler::onRequestRetry" );
         exit(-1);
         return -1;
      }
};

class TestInviteSessionHandler : public InviteSessionHandler
{
   public:
      TestInviteSessionHandler()
      {
      }
      
      virtual ~TestInviteSessionHandler()
      {
      }

      virtual void onNewSession(ClientInviteSessionHandle,
                                InviteSession::OfferAnswerType oat,
                                const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHandler::onNewSession" );
      }
      
      virtual void onFailure(ClientInviteSessionHandle,
                             const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHander::onFailure" );
      }

      virtual void onProvisional(ClientInviteSessionHandle,
                                 const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHandler::onProvisional" );
      }

      virtual void onConnected(ClientInviteSessionHandle,
                               const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHandler::onConnected" );
      }

      virtual void onStaleCallTimeout(ClientInviteSessionHandle)
      {
         InfoLog( << "TestInviteSessionHandler::onStaleCallTimeout" );
      }

      virtual void onRedirected(ClientInviteSessionHandle,
                                const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHandler::onRedirected");
      }

      virtual void onEarlyMedia(ClientInviteSessionHandle,
                                const SipMessage& msg,
                                const SdpContents& sdp)
      {
         InfoLog( << "TestInviteSessionHandler::onEarlyMedia" );
      }

      virtual void onForkDestroyed(ClientInviteSessionHandle)
	  {
         InfoLog( << "TestInviteSessionHandler::onForkDestroyed" );
	  }
      
      virtual void onNewSession(ServerInviteSessionHandle is,
                                InviteSession::OfferAnswerType oat,
                                const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHandler::onNewSession" );
//         is->provisional(180);
      }

      virtual void onConnected(InviteSessionHandle, const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHandler::onConnected()" );
      }

      virtual void onTerminated(InviteSessionHandle,
                                InviteSessionHandler::TerminatedReason reason,
                                const SipMessage* msg)
      {
         InfoLog( << "TestInviteSessionHandler::onTerminated");
      }

      virtual void onOffer(InviteSessionHandle is,
                           const SipMessage& msg, const SdpContents& sdp)
      {
         InfoLog( << "TestInviteSessionHandler::onOffer");
      }
      
      virtual void onOfferRequired(InviteSessionHandle,
                                   const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHandler::onOfferRequired" );
      }

      virtual void onOfferRejected(InviteSessionHandle,
                                   const SipMessage* msg)
      {
         InfoLog( << "TestInviteSessionHandler::onOfferRejected" );
      }

      virtual void onAnswer(InviteSessionHandle,
                            const SipMessage& msg,
                            const SdpContents& sdp)
      {
         InfoLog( << "TestInviteSessionHandler::onAnswer");
      }

      virtual void onRefer(InviteSessionHandle,
                           ServerSubscriptionHandle,
                           const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHandler::onRefer" );
      }

      virtual void onReferAccepted(InviteSessionHandle,
                                   ClientSubscriptionHandle,
                                   const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHandler::onReferAccepted" );
      }

      virtual void onReferRejected(InviteSessionHandle,
                                   const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHandler::onReferRejected" );
      }

      virtual void onInfo(InviteSessionHandle,
                          const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHandler::onInfo" );
      }

      virtual void onInfoSuccess(InviteSessionHandle,
                                 const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHandler::onInfoSuccess" );
      }

      virtual void onInfoFailure(InviteSessionHandle,
                                 const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHandler::onInfoFailure" );
      }

      virtual void onMessage(InviteSessionHandle,
                             const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHandler::onMessage" );
      }

      virtual void onMessageSuccess(InviteSessionHandle,
                                    const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHandler::onMessageSuccess" );
      }

      virtual void onMessageFailure(InviteSessionHandle,
                                    const SipMessage& msg)
      {
         InfoLog( << "TestInviteSessionHandler::onMessageFailure" );
      }

};


class TestDumShutdownHandler : public DumShutdownHandler
{
   public:
      TestDumShutdownHandler()
      {
      }
      
      virtual ~TestDumShutdownHandler()
      {
      }

      virtual void onDumCanBeDeleted() 
      {
         InfoLog( << "TestDumShutdownHandler::onDumCanBeDeleted" );
      }
};


class TestClientPagerMessageHandler : public ClientPagerMessageHandler
{
   public:
      TestClientPagerMessageHandler()
      {
      }
      
      virtual ~TestClientPagerMessageHandler()
      {
      }

      virtual void onSuccess(ClientPagerMessageHandle,
                             const SipMessage& status)
      {
         InfoLog( << "TestClientMessageHandler::onSuccess" );
      }

      virtual void onFailure(ClientPagerMessageHandle,
                             const SipMessage& status,
                             std::auto_ptr<Contents> contents)
      {
         InfoLog( << "TestClientMessageHandler::onFailure" );
      }
};

class TestServerPagerMessageHandler : public ServerPagerMessageHandler
{
   public:
      TestServerPagerMessageHandler()
      {
      }
      
      virtual ~TestServerPagerMessageHandler()
      {
      }

      virtual void onMessageArrived(ServerPagerMessageHandle handle,
                                    const SipMessage& message)
      {
         InfoLog( << "TestServerPagerMessageHandler::onMessageArrived" );

         SharedPtr<SipMessage> ok = handle->accept();
         handle->send(ok);

         InfoLog( << "received type " << message.header(h_ContentType) );

         const SecurityAttributes *attr = message.getSecurityAttributes();
         InfoLog( << *attr );
      }

};

} // namespace

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
