#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/Pidf.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/ClientInviteSession.hxx"
#include "resip/dum/ServerInviteSession.hxx"
#include "resip/dum/ClientSubscription.hxx"
#include "resip/dum/ServerSubscription.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/ServerRegistration.hxx"
#include "resip/dum/ClientPublication.hxx"
#include "resip/dum/ServerPublication.hxx"
#include "resip/dum/ServerOutOfDialogReq.hxx"

#if defined (USE_SSL)
#if defined(WIN32) 
#include "resip/stack/ssl/WinSecurity.hxx"
#else
#include "resip/stack/ssl/Security.hxx"
#endif
#endif

#include "UserAgent.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

UserAgent::UserAgent(int argc, char** argv) : 
   CommandLineParser(argc, argv),
   mProfile(new MasterProfile),
#if defined(USE_SSL)
   mSecurity(new Security(mCertPath)),
   mStack(mSecurity),
#else
   mSecurity(0),
   mStack(mSecurity),
#endif
   mDum(mStack),
   mStackThread(mStack)
{
   Log::initialize(mLogType, mLogLevel, argv[0]);

#if defined(USE_SSL)
   if (mGenUserCert)
   {
      mSecurity->generateUserCert(mAor.getAor());
   }
#endif

   addTransport(UDP, mUdpPort);
   addTransport(TCP, mTcpPort);
#if defined(USE_SSL)
   addTransport(TLS, mTlsPort);
#endif
#if defined(USED_DTLS)
   addTransport(DTLS, mDtlsPort);
#endif

   mProfile->setDefaultRegistrationTime(mRegisterDuration);
   mProfile->addSupportedMethod(NOTIFY);
   mProfile->validateAcceptEnabled() = false;
   mProfile->validateContentEnabled() = false;
   mProfile->addSupportedMimeType(NOTIFY, Pidf::getStaticType());
   mProfile->setDefaultFrom(NameAddr(mAor));
   mProfile->setDigestCredential(mAor.host(), mAor.user(), mPassword);
   
   if (!mContact.host().empty())
   {
      mProfile->setOverrideHostAndPort(mContact);
   }
   if (!mOutboundProxy.host().empty())
   {
      mProfile->setOutboundProxy(Uri(mOutboundProxy));
   }
   mProfile->setUserAgent("limpc/1.0");
   
   mDum.setMasterProfile(mProfile);
   mDum.setClientRegistrationHandler(this);
   mDum.addClientSubscriptionHandler(Symbols::Presence, this);
   mDum.addClientPublicationHandler(Symbols::Presence, this);
   mDum.addOutOfDialogHandler(OPTIONS, this);
   mDum.setClientAuthManager(std::auto_ptr<ClientAuthManager>(new ClientAuthManager));
   mDum.setInviteSessionHandler(this);
   
   mStackThread.run(); 
}

UserAgent::~UserAgent()
{
   mStackThread.shutdown();
   mStackThread.join();
}

void
UserAgent::startup()
{
   if (mRegisterDuration)
   {
      InfoLog (<< "register for " << mAor);
      mDum.send(mDum.makeRegistration(NameAddr(mAor)));
   }

   //for (std::vector<Uri> i = mBuddies.begin(); i != mBuddies.end(); ++i)
   {
   }

#if 0
   mDum.send(mDum.makePublish);

   auto_ptr<SipMessage> msg( sa.dialog->makeInitialPublish(NameAddr(sa.uri),NameAddr(mAor)) );
   Pidf* pidf = new Pidf( *mPidf );
   msg->header(h_Event).value() = "presence";
   msg->setContents( pidf );
   setOutbound( *msg );
   mStack->send( *msg );
#endif
}

void
UserAgent::shutdown()
{
}

void
UserAgent::process()
{
   while (mDum.process());
}

void
UserAgent::addTransport(TransportType type, int port)
{
   for (int i=0; i < 10; ++i)
   {
      try
      {
         if (port)
         {
            if (!mNoV4)
            {
               mDum.addTransport(type, port+i, V4, Data::Empty, mTlsDomain);
               return;
            }

            if (!mNoV6)
            {
               mDum.addTransport(type, port+i, V6, Data::Empty, mTlsDomain);
               return;
            }
         }
      }
      catch (BaseException& e)
      {
         InfoLog (<< "Caught: " << e);
         WarningLog (<< "Failed to add " << Tuple::toData(type) << " transport on " << port);
      }
   }
   throw Transport::Exception("Port already in use", __FILE__, __LINE__);
}


void
UserAgent::onNewSession(ClientInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   InfoLog(<< h->myAddr().uri().user() << " 180 from  " << h->peerAddr().uri().user());

   // Schedule an end() call; checks handle validity, and whether the Session
   // is already tearing down (isTerminated() check)
   mStack.post(std::auto_ptr<ApplicationMessage>(new EndInviteSessionCommand(h->getSessionHandle())), 60, &mDum);
}

void
UserAgent::onNewSession(ServerInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   InfoLog(<< h->myAddr().uri().user() << " INVITE from  " << h->peerAddr().uri().user());
         
   h->provisional(180);
   // .bwc. Er, this doesn't look right. We should provide an answer when we get
   // the onOffer() callback, not the onNewSession() callback.
   // SdpContents* sdp = dynamic_cast<SdpContents*>(msg.getContents());
   // h->provideAnswer(*sdp);
   // h->accept();

   // Schedule an end() call; checks handle validity, and whether the Session
   // is already tearing down (isTerminated() check)
   mStack.post(std::auto_ptr<ApplicationMessage>(new EndInviteSessionCommand(h->getSessionHandle())), 60, &mDum);

   // might update presence here
}

void
UserAgent::onFailure(ClientInviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< h->myAddr().uri().user() 
           << " outgoing call failed " 
           << h->peerAddr().uri().user() 
           << " status=" << msg.header(h_StatusLine).statusCode());
}
      
void
UserAgent::onEarlyMedia(ClientInviteSessionHandle, const SipMessage&, const SdpContents&)
{
}

void
UserAgent::onProvisional(ClientInviteSessionHandle, const SipMessage& msg)
{
}

void
UserAgent::onConnected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< h->myAddr().uri().user() << " in INVITE session with " << h->peerAddr().uri().user());
}

void
UserAgent::onConnected(InviteSessionHandle, const SipMessage& msg)
{
}

void
UserAgent::onStaleCallTimeout(ClientInviteSessionHandle)
{
   WarningLog(<< "onStaleCallTimeout");
}

void
UserAgent::onTerminated(InviteSessionHandle h, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg)
{
   WarningLog(<< h->myAddr().uri().user() << " call terminated with " << h->peerAddr().uri().user() << " reason=" << reason);
}

void
UserAgent::onRedirected(ClientInviteSessionHandle, const SipMessage& msg)
{
   assert(false);
}

void
UserAgent::onAnswer(InviteSessionHandle, const SipMessage& msg, const SdpContents&)
{
}

void
UserAgent::onOffer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& offer)
{
   InfoLog(<< h->myAddr().uri().user() << " offer from  " << h->peerAddr().uri().user());

   SdpContents* sdp = dynamic_cast<SdpContents*>(msg.getContents());
   h->provideAnswer(*sdp);
   if(msg.isRequest() && msg.method()==INVITE && !h->isConnected())
   {
      ServerInviteSession* uas = dynamic_cast<ServerInviteSession*>(h.get());
      if(uas)
      {
         uas->accept();
      }
   }
}

void
UserAgent::onOfferRequired(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< h->myAddr().uri().user() << " offer requested by  " 
            << h->peerAddr().uri().user());

   static Data txt("v=0\r\n"
                  "o=1900 369696545 369696545 IN IP4 192.168.2.15\r\n"
                  "s=X-Lite\r\n"
                  "c=IN IP4 192.168.2.15\r\n"
                  "t=0 0\r\n"
                  "m=audio 8000 RTP/AVP 8 3 98 97 101\r\n"
                  "a=rtpmap:8 pcma/8000\r\n"
                  "a=rtpmap:3 gsm/8000\r\n"
                  "a=rtpmap:98 iLBC\r\n"
                  "a=rtpmap:97 speex/8000\r\n"
                  "a=rtpmap:101 telephone-event/8000\r\n"
                  "a=fmtp:101 0-15\r\n");

   static HeaderFieldValue hfv(txt.data(), txt.size());
   static Mime type("application", "sdp");
   static SdpContents sdp(hfv, type);
   h->provideOffer(sdp);
}

void
UserAgent::onOfferRejected(InviteSessionHandle h, const SipMessage* msg)
{
   InfoLog(<< h->myAddr().uri().user() << " offer rejected by  " 
            << h->peerAddr().uri().user());
}

void
UserAgent::onDialogModified(InviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   InfoLog(<< h->myAddr().uri().user() << " dialog modified " 
            << h->peerAddr().uri().user());
}

void
UserAgent::onInfo(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< h->myAddr().uri().user() << " INFO " 
            << h->peerAddr().uri().user());
   h->acceptNIT();
}

void
UserAgent::onInfoSuccess(InviteSessionHandle, const SipMessage& msg)
{
}

void
UserAgent::onInfoFailure(InviteSessionHandle, const SipMessage& msg)
{
}

void
UserAgent::onRefer(InviteSessionHandle, ServerSubscriptionHandle, const SipMessage& msg)
{
    assert(0);
}

void
UserAgent::onReferAccepted(InviteSessionHandle, ClientSubscriptionHandle, const SipMessage& msg)
{
   assert(false);
}

void
UserAgent::onReferRejected(InviteSessionHandle, const SipMessage& msg)
{
   assert(0);
}

void
UserAgent::onReferNoSub(InviteSessionHandle, const SipMessage& msg)
{
   assert(0);
}

void
UserAgent::onMessage(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(<< h->myAddr().uri().user() << " MESSAGE " 
            << h->peerAddr().uri().user());
   h->acceptNIT();
}

void
UserAgent::onMessageSuccess(InviteSessionHandle, const SipMessage& msg)
{
}

void
UserAgent::onMessageFailure(InviteSessionHandle, const SipMessage& msg)
{
}


////////////////////////////////////////////////////////////////////////////////
// Registration Handler ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
UserAgent::onSuccess(ClientRegistrationHandle h, const SipMessage& response)
{
}

void
UserAgent::onFailure(ClientRegistrationHandle h, const SipMessage& response)
{
}

void
UserAgent::onRemoved(ClientRegistrationHandle h, const SipMessage&)
{
}

int 
UserAgent::onRequestRetry(ClientRegistrationHandle h, int retryMinimum, const SipMessage& msg)
{
   //assert(false);
   return -1;
}

////////////////////////////////////////////////////////////////////////////////
// ClientSubscriptionHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
UserAgent::onUpdatePending(ClientSubscriptionHandle h, const SipMessage& notify, bool outOfOrder)
{
}

void
UserAgent::onUpdateActive(ClientSubscriptionHandle h, const SipMessage& notify, bool outOfOrder)
{
}

void
UserAgent::onUpdateExtension(ClientSubscriptionHandle, const SipMessage& notify, bool outOfOrder)
{
}

void
UserAgent::onTerminated(ClientSubscriptionHandle h, const SipMessage* notify)
{
}

void
UserAgent::onNewSubscription(ClientSubscriptionHandle h, const SipMessage& notify)
{
}

int 
UserAgent::onRequestRetry(ClientSubscriptionHandle h, int retryMinimum, const SipMessage& notify)
{
   return -1;
}

////////////////////////////////////////////////////////////////////////////////
// ClientPublicationHandler ////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
UserAgent::onSuccess(ClientPublicationHandle h, const SipMessage& status)
{
}

void
UserAgent::onRemove(ClientPublicationHandle h, const SipMessage& status)
{
}

void
UserAgent::onFailure(ClientPublicationHandle h, const SipMessage& response)
{
}

int 
UserAgent::onRequestRetry(ClientPublicationHandle h, int retryMinimum, const SipMessage& response)
{
   return -1;
}
      
////////////////////////////////////////////////////////////////////////////////
// OutOfDialogHandler //////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
UserAgent::onSuccess(ClientOutOfDialogReqHandle, const SipMessage& response)
{
   InfoLog(<< response.header(h_CSeq).method() << "::OK: " << response );
}

void 
UserAgent::onFailure(ClientOutOfDialogReqHandle, const SipMessage& response)
{
   ErrLog(<< response.header(h_CSeq).method() << "::failure: " << response );
   if (response.exists(h_Warnings)) ErrLog  (<< response.header(h_Warnings).front());
}

void 
UserAgent::onReceivedRequest(ServerOutOfDialogReqHandle h, const SipMessage& request)
{
   InfoLog(<< request.method() << ": Just respond.");
   h->send(h->accept());
}

void
UserAgent::onForkDestroyed(ClientInviteSessionHandle)
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
