#include "resiprocate/os/Log.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/Pidf.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"
#include "resiprocate/dum/ServerInviteSession.hxx"
#include "resiprocate/dum/ClientSubscription.hxx"
#include "resiprocate/dum/ServerSubscription.hxx"
#include "resiprocate/dum/ClientRegistration.hxx"
#include "resiprocate/dum/ServerRegistration.hxx"
#include "resiprocate/dum/ClientPublication.hxx"
#include "resiprocate/dum/ServerPublication.hxx"

#include "UserAgent.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

UserAgent::UserAgent(int argc, char** argv) : 
   CommandLineParser(argc, argv),
#if defined(USE_SSL)
   mSecurity(new Security(mCertPath)),
   mDum(mSecurity)
#else
   mSecurity(0),
   mDum(mSecurity)
#endif
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
   addTransport(TLS, mTlsPort);
   addTransport(DTLS, mDtlsPort);

   mProfile.setDefaultRegistrationTime(mRegisterDuration);
   mProfile.addSupportedMethod(NOTIFY);
   mProfile.validateAcceptEnabled() = false;
   mProfile.validateContentEnabled() = false;
   mProfile.addSupportedMimeType(NOTIFY, Pidf::getStaticType());
   mProfile.setDefaultFrom(NameAddr(mAor));
   if (!mContact.host().empty())
   {
      mProfile.setOverrideHostAndPort(mContact);
   }
   if (!mOutboundProxy.host().empty())
   {
      mProfile.setOutboundProxy(Uri(mOutboundProxy));
   }
   mProfile.setUserAgent("limpc/0.3");
   
   mDum.setMasterProfile(&mProfile);
   mDum.setClientRegistrationHandler(this);
   mDum.addClientSubscriptionHandler(Symbols::Presence, this);
   mDum.addClientPublicationHandler(Symbols::Presence, this);
   mDum.addOutOfDialogHandler(OPTIONS, this);
   mDum.setClientAuthManager(std::auto_ptr<ClientAuthManager>(new ClientAuthManager));
   mDum.setInviteSessionHandler(this);
   
   mDum.run(); // starts a StackThread
}

UserAgent::~UserAgent()
{
   shutdown();
   join();
}

void
UserAgent::startup()
{
   if (mRegisterDuration)
   {
      mDum.send(mDum.makeRegistration(mAor));
   }

   for (std::vector<Uri> i = mBuddies.begin(); i != mBuddies.end(); ++i)
   {
   }

   mDum.send(mDum.makePublish);

   auto_ptr<SipMessage> msg( sa.dialog->makeInitialPublish(NameAddr(sa.uri),NameAddr(mAor)) );
   Pidf* pidf = new Pidf( *mPidf );
   msg->header(h_Event).value() = "presence";
   msg->setContents( pidf );
   setOutbound( *msg );
   mStack->send( *msg );

}


void
UserAgent::process()
{
   mDum.process();
}

void
UserAgent::thread()
{
   while(!waitForShutdown(1000))
   {
      process();
   }
}


void
UserAgent::addTransport(TransportType type, int port)
{
   try
   {
      if (port)
      {
         if (!mNoV4)
         {
            mDum.addTransport(type, port, V4, Data::Empty, mTlsDomain);
         }

         if (!mNoV6)
         {
            mDum.addTransport(type, port, V6, Data::Empty, mTlsDomain);
         }
      }
   }
   catch (BaseException& e)
   {
      InfoLog (<< "Caught: " << e);
      WarningLog (<< "Failed to add " << Tuple::toData(type) << " transport on " << port);
      throw;
   }
   
}


void
UserAgent::onNewSession(ClientInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   InfoLog(<< h->myAddr().uri().user() << " 180 from  " << h->peerAddr().uri().user());
}

void
UserAgent::onNewSession(ServerInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   InfoLog(<< h->myAddr().uri().user() << " INVITE from  " << h->peerAddr().uri().user());
         
   h->provisional(180);
   SdpContents* sdp = dynamic_cast<SdpContents*>(msg.getContents());
   h->provideAnswer(*sdp);
   h->accept();

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
   if (reason != InviteSessionHandler::PeerEnded)
   {
      WarningLog(<< h->myAddr().uri().user() << " call terminated with " << h->peerAddr().uri().user());
   }
   else
   {
      WarningLog(<< h->myAddr().uri().user() << " ended call with " << h->peerAddr().uri().user());
   }
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
UserAgent::onOffer(InviteSessionHandle handle, const SipMessage& msg, const SdpContents& offer)
{         
}

void
UserAgent::onOfferRequired(InviteSessionHandle, const SipMessage& msg)
{
   assert(false);
}

void
UserAgent::onOfferRejected(InviteSessionHandle, const SipMessage& msg)
{
    assert(0);
}

void
UserAgent::onDialogModified(InviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
    assert(0);
}

void
UserAgent::onInfo(InviteSessionHandle, const SipMessage& msg)
{
    assert(0);
}

void
UserAgent::onInfoSuccess(InviteSessionHandle, const SipMessage& msg)
{
    assert(0);
}

void
UserAgent::onInfoFailure(InviteSessionHandle, const SipMessage& msg)
{
    assert(0);
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
UserAgent::onRemoved(ClientRegistrationHandle h)
{
}

int 
UserAgent::onRequestRetry(ClientRegistrationHandle h, int retryMinimum, const SipMessage& msg)
{
   assert(false);
   return -1;
}

////////////////////////////////////////////////////////////////////////////////
// ClientSubscriptionHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
UserAgent::onRefreshRejected(ClientSubscriptionHandle h, const SipMessage& rejection)
{
}

void
UserAgent::onUpdatePending(ClientSubscriptionHandle h, const SipMessage& notify)
{
}

void
UserAgent::onUpdateActive(ClientSubscriptionHandle h, const SipMessage& notify)
{
}

void
UserAgent::onUpdateExtension(ClientSubscriptionHandle, const SipMessage& notify)
{
}

void
UserAgent::onTerminated(ClientSubscriptionHandle h, const SipMessage& notify)
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
UserAgent::onReceivedRequest(ServerOutOfDialogReqHandle, const SipMessage& request)
{
}

void
UserAgent::onForkDestroyed(ClientInviteSessionHandle)
{
}
