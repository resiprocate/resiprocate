#if !defined(DUM_UserAgent_hxx)
#define DUM_UserAgent_hxx

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "CommandLineParser.hxx"

#include "resip/stack/StackThread.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/SubscriptionHandler.hxx"
#include "resip/dum/PublicationHandler.hxx"
#include "resip/dum/OutOfDialogHandler.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/DialogUsageManager.hxx"

namespace resip
{

class UserAgent : public CommandLineParser, 
                  public ClientRegistrationHandler, 
                  public ClientSubscriptionHandler, 
                  public ClientPublicationHandler,
                  public OutOfDialogHandler, 
                  public InviteSessionHandler
{
   public:
      UserAgent(int argc, char** argv);
      virtual ~UserAgent();

      virtual void startup();
      virtual void shutdown();

      void process();
      
   public:
      // Invite Session Handler /////////////////////////////////////////////////////
      virtual void onNewSession(ClientInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg);
      virtual void onNewSession(ServerInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg);
      virtual void onFailure(ClientInviteSessionHandle h, const SipMessage& msg);
      virtual void onEarlyMedia(ClientInviteSessionHandle, const SipMessage&, const SdpContents&);
      virtual void onProvisional(ClientInviteSessionHandle, const SipMessage& msg);
      virtual void onConnected(ClientInviteSessionHandle h, const SipMessage& msg);
      virtual void onConnected(InviteSessionHandle, const SipMessage& msg);
      virtual void onStaleCallTimeout(ClientInviteSessionHandle);
      virtual void onTerminated(InviteSessionHandle h, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg);
      virtual void onRedirected(ClientInviteSessionHandle, const SipMessage& msg);
      virtual void onAnswer(InviteSessionHandle, const SipMessage& msg, const SdpContents&);
      virtual void onOffer(InviteSessionHandle handle, const SipMessage& msg, const SdpContents& offer);
      virtual void onOfferRequired(InviteSessionHandle, const SipMessage& msg);
      virtual void onOfferRejected(InviteSessionHandle, const SipMessage* msg);
      virtual void onDialogModified(InviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg);
      virtual void onInfo(InviteSessionHandle, const SipMessage& msg);
      virtual void onInfoSuccess(InviteSessionHandle, const SipMessage& msg);
      virtual void onInfoFailure(InviteSessionHandle, const SipMessage& msg);
      virtual void onRefer(InviteSessionHandle, ServerSubscriptionHandle, const SipMessage& msg);
      virtual void onReferAccepted(InviteSessionHandle, ClientSubscriptionHandle, const SipMessage& msg);
      virtual void onReferRejected(InviteSessionHandle, const SipMessage& msg);
      virtual void onReferNoSub(InviteSessionHandle, const SipMessage& msg);

      virtual void onMessage(InviteSessionHandle, const SipMessage& msg);
      virtual void onMessageSuccess(InviteSessionHandle, const SipMessage& msg);
      virtual void onMessageFailure(InviteSessionHandle, const SipMessage& msg);
      
      // Registration Handler ////////////////////////////////////////////////////////
      virtual void onSuccess(ClientRegistrationHandle h, const SipMessage& response);
      virtual void onFailure(ClientRegistrationHandle h, const SipMessage& response);
      virtual void onRemoved(ClientRegistrationHandle h, const SipMessage& response);
      virtual int onRequestRetry(ClientRegistrationHandle h, int retryMinimum, const SipMessage& msg);

      // ClientSubscriptionHandler ///////////////////////////////////////////////////
      virtual void onUpdatePending(ClientSubscriptionHandle h, const SipMessage& notify, bool outOfOrder);
      virtual void onUpdateActive(ClientSubscriptionHandle h, const SipMessage& notify, bool outOfOrder);
      virtual void onUpdateExtension(ClientSubscriptionHandle, const SipMessage& notify, bool outOfOrder);
      virtual void onTerminated(ClientSubscriptionHandle h, const SipMessage* notify);
      virtual void onNewSubscription(ClientSubscriptionHandle h, const SipMessage& notify);
      virtual int onRequestRetry(ClientSubscriptionHandle h, int retryMinimum, const SipMessage& notify);

      // ClientPublicationHandler ////////////////////////////////////////////////////
      virtual void onSuccess(ClientPublicationHandle h, const SipMessage& status);
      virtual void onRemove(ClientPublicationHandle h, const SipMessage& status);
      virtual void onFailure(ClientPublicationHandle h, const SipMessage& response);
      virtual int onRequestRetry(ClientPublicationHandle h, int retryMinimum, const SipMessage& response);
      
      // OutOfDialogHandler //////////////////////////////////////////////////////////
      virtual void onSuccess(ClientOutOfDialogReqHandle, const SipMessage& response);
      virtual void onFailure(ClientOutOfDialogReqHandle, const SipMessage& response);
      virtual void onReceivedRequest(ServerOutOfDialogReqHandle, const SipMessage& request);
      virtual void onForkDestroyed(ClientInviteSessionHandle);

      class EndInviteSessionCommand : public DumCommand
      {
         public:
            EndInviteSessionCommand(InviteSessionHandle h) : mHandle(h)
            {}

            virtual void executeCommand()
            {
               if(mHandle.isValid() && !mHandle->isTerminated())
               {
                  mHandle->end();
               }
            }

            virtual Message* clone() const
            {
               return new EndInviteSessionCommand(mHandle);
            }

#ifdef RESIP_USE_STL_STREAMS
            virtual std::ostream& encode(std::ostream& str) const
#else
            virtual EncodeStream& encode(EncodeStream& str) const
#endif
            {
               return str << "EndInviteSessionCommand";
            }

#ifdef RESIP_USE_STL_STREAMS
            virtual std::ostream& encodeBrief(std::ostream& str) const
#else
            virtual EncodeStream& encodeBrief(EncodeStream& str) const
#endif
            {
               return encode(str);
            }
         
         private:
            InviteSessionHandle mHandle;
      };

   protected:
      void addTransport(TransportType type, int port);

      SharedPtr<MasterProfile> mProfile;
      Security* mSecurity;
      SipStack mStack;
      DialogUsageManager mDum;
      StackThread mStackThread;
};
 
}

#endif

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
