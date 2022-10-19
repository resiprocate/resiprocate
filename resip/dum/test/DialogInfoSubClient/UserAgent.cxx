#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/MD5Stream.hxx>
#include <rutil/FdPoll.hxx>
#include <resip/stack/SdpContents.hxx>
#include <resip/stack/PlainContents.hxx>
#include <resip/stack/DialogInfoContents.hxx>
#include <resip/stack/ConnectionTerminated.hxx>
#include <resip/stack/Helper.hxx>
#include <resip/dum/AppDialogSetFactory.hxx>
#include <resip/dum/ClientAuthManager.hxx>
#include <resip/dum/KeepAliveManager.hxx>
#include <resip/dum/ClientInviteSession.hxx>
#include <resip/dum/ServerInviteSession.hxx>
#include <resip/dum/ClientSubscription.hxx>
#include <resip/dum/ServerSubscription.hxx>
#include <resip/dum/ClientRegistration.hxx>
#include <resip/dum/ServerRegistration.hxx>
#include <resip/dum/ServerOutOfDialogReq.hxx>
#include <rutil/dns/AresDns.hxx>

#if defined (USE_SSL)
#if defined(WIN32) 
#include "resip/stack/ssl/WinSecurity.hxx"
#else
#include "resip/stack/ssl/Security.hxx"
#endif
#endif

#include "UserAgent.hxx"
#include "Call.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

static unsigned int MaxRegistrationRetryTime = 1800;              // RFC5626 section 4.5 default
static unsigned int BaseRegistrationRetryTimeAllFlowsFailed = 30; // RFC5626 section 4.5 default
//static unsigned int BaseRegistrationRetryTime = 90;               // RFC5626 section 4.5 default
static unsigned int NotifySendTime = 30;  // If someone subscribes to our test event package, then send notifies every 30 seconds
static unsigned int FailedSubscriptionRetryTime = 60; 

//#define TEST_PASSING_A1_HASH_FOR_PASSWORD

namespace resip
{
class ClientAppDialogSetFactory : public AppDialogSetFactory
{
public:
   ClientAppDialogSetFactory(UserAgent& ua) : mUserAgent(ua) {}
   resip::AppDialogSet* createAppDialogSet(DialogUsageManager& dum, const SipMessage& msg)
   {
      switch(msg.method())
      {
         case INVITE:
            return new Call(mUserAgent);
            break;
         default:         
            return AppDialogSetFactory::createAppDialogSet(dum, msg); 
            break;
      }
   }
private:
   UserAgent& mUserAgent;
};

// Used to set the IP Address in outbound SDP to match the IP address chosen by the stack to send the message on
class SdpMessageDecorator : public MessageDecorator
{
public:
   SdpMessageDecorator(UserAgent* ua)
   :  mUserAgent(ua) {}
   virtual ~SdpMessageDecorator() {}
   virtual void decorateMessage(SipMessage &msg, 
                                const Tuple &source,
                                const Tuple &destination,
                                const Data& sigcompId)
   {
      SdpContents* sdp = dynamic_cast<SdpContents*>(msg.getContents());
      if(sdp)  
      {
         // Label the SDP with the UA AOR so we can tell which SDP belongs to which
         // TODO might be better to use the contact in SDP as it will have a unique port when
         // we run multiple instances of the DialogInfoSubClient on the same host with same AOR.
         DebugLog(<< "SdpMessageDecorator::decorateMessage default From: " << mUserAgent->getIncomingUserProfile(msg)->getDefaultFrom());
         sdp->session().uri() = mUserAgent->getIncomingUserProfile(msg)->getDefaultFrom().uri();
         // Set origin address independent of hold state
         sdp->session().origin().setAddress(Tuple::inet_ntop(source), source.ipVersion() == V6 ? SdpContents::IP6 : SdpContents::IP4);

         assert(mUserAgent);
         if(mUserAgent->shouldHoldCalls())
         {
            sdp->session().connection().setAddress("0.0.0.0", SdpContents::IP4);
            std::list<SdpContents::Session::Medium>::iterator mediaIt = sdp->session().media().begin();

            // For this UA should always be exactly 1 Media 
            assert(sdp->session().media().begin() != sdp->session().media().end());
            assert(sdp->session().media().size() == 1);
            mediaIt->clearAttribute("sendrecv");
            mediaIt->clearAttribute("sendonly");
            mediaIt->clearAttribute("recvonly");
            mediaIt->clearAttribute("inactive");
            mediaIt->addAttribute("inactive");
         }
         else
         {
            // Fill in IP and Port from source
            sdp->session().connection().setAddress(Tuple::inet_ntop(source), source.ipVersion() == V6 ? SdpContents::IP6 : SdpContents::IP4);
            std::list<SdpContents::Session::Medium>::iterator mediaIt = sdp->session().media().begin();

            // For this UA should always be exactly 1 Media 
            assert(sdp->session().media().begin() != sdp->session().media().end());
            assert(sdp->session().media().size() == 1);
            mediaIt->clearAttribute("sendrecv");
            mediaIt->clearAttribute("sendonly");
            mediaIt->clearAttribute("recvonly");
            mediaIt->clearAttribute("inactive");
            mediaIt->addAttribute("sendrecv");
         }
         InfoLog( << "SdpMessageDecorator: src=" << source << ", dest=" << destination << ", msg=" << endl << msg.brief());
      }
   }
   virtual void rollbackMessage(SipMessage& msg) {}  // Nothing to do
   virtual MessageDecorator* clone() const { return new SdpMessageDecorator(mUserAgent); }

   UserAgent* mUserAgent;
};

class NotifyTimer : public resip::DumCommand
{
   public:
      NotifyTimer(UserAgent& userAgent, unsigned int timerId) : mUserAgent(userAgent), mTimerId(timerId) {}
      NotifyTimer(const NotifyTimer& rhs) : mUserAgent(rhs.mUserAgent), mTimerId(rhs.mTimerId) {}
      ~NotifyTimer() {}

      void executeCommand() { mUserAgent.onNotifyTimeout(mTimerId); }

      resip::Message* clone() const { return new NotifyTimer(*this); }
      EncodeStream& encode(EncodeStream& strm) const { strm << "NotifyTimer: id=" << mTimerId; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }

   private:
      UserAgent& mUserAgent;
      unsigned int mTimerId;
};

class EndCalls : public resip::DumCommandAdapter
{
public:
   EndCalls(UserAgent& userAgent) : mUserAgent(userAgent) {}
   EndCalls(const EndCalls& rhs) : mUserAgent(rhs.mUserAgent) {}
   ~EndCalls() {}

   void executeCommand() { mUserAgent.onEndCalls(); }

   EncodeStream& encode(EncodeStream& strm) const { strm << "EndCalls"; return strm; }
   EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }

private:
   UserAgent& mUserAgent;
};

class ToggleHold : public resip::DumCommandAdapter
{
public:
   ToggleHold(UserAgent& userAgent) : mUserAgent(userAgent) {}
   ToggleHold(const ToggleHold& rhs) : mUserAgent(rhs.mUserAgent) {}
   ~ToggleHold() {}

   void executeCommand() { mUserAgent.onToggleHold(); }

   EncodeStream& encodeBrief(EncodeStream& strm) const { strm << "ToggleHold"; return strm; }

private:
   UserAgent& mUserAgent;
};

class InvokePick : public resip::DumCommandAdapter
{
public:
   InvokePick(UserAgent& userAgent) : mUserAgent(userAgent) {}
   InvokePick(const InvokePick& rhs) : mUserAgent(rhs.mUserAgent) {}
   ~InvokePick() {}

   void executeCommand() { mUserAgent.onInvokePick(); }

   EncodeStream& encodeBrief(EncodeStream& strm) const { strm << "InvokePick"; return strm; }

private:
   UserAgent& mUserAgent;
};

class InvokeJoin : public resip::DumCommandAdapter
{
public:
   InvokeJoin(UserAgent& userAgent) : mUserAgent(userAgent) {}
   InvokeJoin(const InvokeJoin& rhs) : mUserAgent(rhs.mUserAgent) {}
   ~InvokeJoin() {}

   void executeCommand() { mUserAgent.onInvokeJoin(); }

   EncodeStream& encodeBrief(EncodeStream& strm) const { strm << "InvokeJoin"; return strm; }

private:
   UserAgent& mUserAgent;
};

} // end namespace

UserAgent::UserAgent(int argc, char** argv) : 
   CmdLineParser(argc, argv),
   mProfile(new MasterProfile),
#if defined(USE_SSL)
   mSecurity(new Security(mCertPath)),
#else
   mSecurity(0),
#endif
   mPollGrp(FdPollGrp::create()),  // Will create EPoll implementation if available, otherwise FdPoll
   mInterruptor(new EventThreadInterruptor(*mPollGrp)),
   mStack(new SipStack(mSecurity, DnsStub::EmptyNameserverList, mInterruptor, false, 0, 0, mPollGrp)),
   mStackThread(new EventStackThread(*mStack, *mInterruptor, *mPollGrp)),
   mDum(new DialogUsageManager(*mStack)),
   mDumShutdownRequested(false),
   mShuttingdown(false),
   mDumShutdown(false),
   mRegistrationRetryDelayTime(0),
   mCurrentNotifyTimerId(0),
   mCurrentDialogInfoVersion(0),
   mCurrentDialogInfoVersionSet(false),
   mSubTargetDialogId(Data::Empty, Data::Empty, Data::Empty),
   mRingNoAnswer(false),
   mHoldCalls(false)
{
   Log::initialize(mLogType, mLogLevel, argv[0]);

   if(mHostFileLookupOnlyDnsMode)
   {
      AresDns::enableHostFileLookupOnlyMode(true);
   }

   addTransport(UDP, mUdpPort);
   addTransport(TCP, mTcpPort);
#if defined(USE_SSL)
   addTransport(TLS, mTlsPort);
#endif
#if defined(USE_DTLS)
   addTransport(DTLS, mDtlsPort);
#endif

   // Disable Statistics Manager
   mStack->statisticsManagerEnabled() = false;

   // Supported Methods
   mProfile->clearSupportedMethods();
   mProfile->addSupportedMethod(INVITE);
   mProfile->addSupportedMethod(ACK);
   mProfile->addSupportedMethod(CANCEL);
   mProfile->addSupportedMethod(OPTIONS);
   mProfile->addSupportedMethod(BYE);
   //mProfile->addSupportedMethod(REFER);
   mProfile->addSupportedMethod(NOTIFY);
   mProfile->addSupportedMethod(SUBSCRIBE);
   //mProfile->addSupportedMethod(UPDATE);
   mProfile->addSupportedMethod(INFO);
   mProfile->addSupportedMethod(MESSAGE);
   mProfile->addSupportedMethod(PRACK);
   //mProfile->addSupportedOptionTag(Token(Symbols::C100rel));  // Automatically added when using setUacReliableProvisionalMode
   mProfile->setUacReliableProvisionalMode(MasterProfile::Supported);
   mProfile->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);  

   // Support Languages
   mProfile->clearSupportedLanguages();
   mProfile->addSupportedLanguage(Token("en"));  

   // Support Mime Types
   mProfile->clearSupportedMimeTypes();
   mProfile->addSupportedMimeType(INVITE, Mime("application", "sdp"));
   mProfile->addSupportedMimeType(INVITE, Mime("multipart", "mixed"));  
   mProfile->addSupportedMimeType(INVITE, Mime("multipart", "signed"));  
   mProfile->addSupportedMimeType(INVITE, Mime("multipart", "alternative"));  
   mProfile->addSupportedMimeType(OPTIONS,Mime("application", "sdp"));
   mProfile->addSupportedMimeType(OPTIONS,Mime("multipart", "mixed"));  
   mProfile->addSupportedMimeType(OPTIONS, Mime("multipart", "signed"));  
   mProfile->addSupportedMimeType(OPTIONS, Mime("multipart", "alternative"));  
   mProfile->addSupportedMimeType(PRACK,  Mime("application", "sdp"));  
   mProfile->addSupportedMimeType(PRACK,  Mime("multipart", "mixed"));  
   mProfile->addSupportedMimeType(PRACK,  Mime("multipart", "signed"));  
   mProfile->addSupportedMimeType(PRACK,  Mime("multipart", "alternative"));  
   mProfile->addSupportedMimeType(UPDATE, Mime("application", "sdp"));  
   mProfile->addSupportedMimeType(UPDATE, Mime("multipart", "mixed"));  
   mProfile->addSupportedMimeType(UPDATE, Mime("multipart", "signed"));  
   mProfile->addSupportedMimeType(UPDATE, Mime("multipart", "alternative"));  
   mProfile->addSupportedMimeType(MESSAGE, Mime("text","plain")); // Invite session in-dialog routing testing
   mProfile->addSupportedMimeType(NOTIFY, Mime("application","dialog-info+xml"));
   //mProfile->addSupportedMimeType(NOTIFY, Mime("message", "sipfrag"));  

   // Supported Options Tags
   mProfile->clearSupportedOptionTags();
   //mMasterProfile->addSupportedOptionTag(Token(Symbols::Replaces));      
   mProfile->addSupportedOptionTag(Token(Symbols::Timer));     // Enable Session Timers
   if(mOutboundEnabled)
   {
      mProfile->addSupportedOptionTag(Token(Symbols::Outbound));  // RFC 5626 - outbound
      mProfile->addSupportedOptionTag(Token(Symbols::Path));      // RFC 3327 - path
   }
   //mMasterProfile->addSupportedOptionTag(Token(Symbols::NoReferSub));
   //mMasterProfile->addSupportedOptionTag(Token(Symbols::TargetDialog));

   // Supported Schemes
   mProfile->clearSupportedSchemes();
   mProfile->addSupportedScheme("sip");  
#if defined(USE_SSL)
   mProfile->addSupportedScheme("sips");  
#endif

   // Validation Settings
   mProfile->validateContentEnabled() = false;
   mProfile->validateContentLanguageEnabled() = false;
   mProfile->validateAcceptEnabled() = false;

   // Have stack add Allow/Supported/Accept headers to INVITE dialog establishment messages
   mProfile->clearAdvertisedCapabilities(); // Remove Profile Defaults, then add our preferences
   mProfile->addAdvertisedCapability(Headers::Allow);  
   //mProfile->addAdvertisedCapability(Headers::AcceptEncoding);  // This can be misleading - it might specify what is expected in response
   mProfile->addAdvertisedCapability(Headers::AcceptLanguage);  
   mProfile->addAdvertisedCapability(Headers::Supported);  
   mProfile->setMethodsParamEnabled(true);

   // Install Sdp Message Decorator
   std::shared_ptr<MessageDecorator> outboundDecorator(new SdpMessageDecorator(this));
   mProfile->setOutboundDecorator(outboundDecorator);

   // Other Profile Settings
   mProfile->setUserAgent("DialogInfoSubClient/1.0");
   mProfile->setDefaultRegistrationTime(mRegisterDuration);
   mProfile->setDefaultRegistrationRetryTime(120);
   if(!mContact.host().empty())
   {
      mProfile->setOverrideHostAndPort(mContact);
   }
   if(!mOutboundProxy.host().empty())
   {
      mProfile->setOutboundProxy(Uri(mOutboundProxy));
      //mProfile->setForceOutboundProxyOnAllRequestsEnabled(true);
      mProfile->setExpressOutboundAsRouteSetEnabled(true);
   }

   // UserProfile Settings
   mProfile->setDefaultFrom(NameAddr(mAor));
#ifdef TEST_PASSING_A1_HASH_FOR_PASSWORD
   MD5Stream a1;
   a1 << mAor.user()
      << Symbols::COLON
      << mAor.host()
      << Symbols::COLON
      << mPassword;
   mProfile->setDigestCredential(mAor.host(), mAor.user(), a1.getHex(), true);   
#else
   mProfile->setDigestCredential(mAor.host(), mAor.user(), mPassword);   
#endif
   // Generate InstanceId appropriate for testing only.  Should be UUID that persists 
   // across machine re-starts and is unique to this application instance.  The one used 
   // here is only as unique as the hostname of this machine.  If someone runs two 
   // instances of this application on the same host for the same Aor, then things will 
   // break.  See RFC5626 section 4.1
   Data hostname = DnsUtil::getLocalHostName();
   Data instanceHash = hostname.md5().uppercase();
   assert(instanceHash.size() == 32);
   Data instanceId(48, Data::Preallocate);
   instanceId += "<urn:uuid:";
   instanceId += instanceHash.substr(0, 8);
   instanceId += "-";
   instanceId += instanceHash.substr(8, 4);
   instanceId += "-";
   instanceId += instanceHash.substr(12, 4);
   instanceId += "-";
   instanceId += instanceHash.substr(16, 4);
   instanceId += "-";
   instanceId += instanceHash.substr(20, 12);
   instanceId += ">";
   mProfile->setInstanceId(instanceId);  
   if(mOutboundEnabled)
   {
      mProfile->setRegId(1);
      mProfile->clientOutboundEnabled() = true;
   }

   resip::Timer::TcpConnectTimeout = 10000;

   // Install Managers
   mDum->setClientAuthManager(std::unique_ptr<ClientAuthManager>(new ClientAuthManager));
   mDum->setKeepAliveManager(std::unique_ptr<KeepAliveManager>(new KeepAliveManager));
   mProfile->setKeepAliveTimeForDatagram(30);
   mProfile->setKeepAliveTimeForStream(120);

   // Install Handlers
   mDum->setInviteSessionHandler(this); 
   mDum->setDialogSetHandler(this);
   mDum->addOutOfDialogHandler(OPTIONS, this);
   //mDum->addOutOfDialogHandler(REFER, this);
   mDum->setRedirectHandler(this);
   mDum->setClientRegistrationHandler(this);   
   mDum->addClientSubscriptionHandler("dialog", this);
   //mDum->addServerSubscriptionHandler("dialog", this);

   // Set AppDialogSetFactory
   unique_ptr<AppDialogSetFactory> dsf(new ClientAppDialogSetFactory(*this));
   mDum->setAppDialogSetFactory(std::move(dsf));

   mDum->setMasterProfile(mProfile);

   mDum->registerForConnectionTermination(this);
}

UserAgent::~UserAgent()
{
   mStack->shutdownAndJoinThreads();
   mStackThread->shutdown();
   mStackThread->join();

   delete mDum;
   delete mStack;
   delete mStackThread;
   delete mInterruptor;
   delete mPollGrp;
   // Note:  mStack descructor will delete mSecurity
}

void
UserAgent::startup()
{
   mStack->run();
   mStackThread->run(); 

   if (mRegisterDuration)
   {
      InfoLog (<< "register for " << mAor);
      mDum->send(mDum->makeRegistration(NameAddr(mAor)));
   }
   else
   {
      // If not registering then form subscription and/or call here.  If registering then we will start these
      // after the registration is successful.

      // Check if we should try to form a test subscription
      if(!mSubscribeTarget.host().empty())
      {
         auto sub = mDum->makeSubscription(NameAddr(mSubscribeTarget), mProfile, "dialog");
         mDum->send(std::move(sub));
      }

      // Check if we should try to form a test call
      if(!mCallTarget.host().empty())
      {
         Call* newCall = new Call(*this);
         newCall->initiateCall(mCallTarget, mProfile);
      }
   }
}

void
UserAgent::shutdown()
{
   assert(mDum);
   mDumShutdownRequested = true; // Set flag so that shutdown operations can be run in dum process thread
   mShuttingdown = true;  // This flag stays on during the shutdown process where as mDumShutdownRequested will get toggled back to false
}

void
UserAgent::endCalls()
{
   // queue message to dum thread
   mDum->post(new EndCalls(*this));
}

void
UserAgent::toggleHold()
{
   // queue message to dum thread
   mDum->post(new ToggleHold(*this));
}

void
UserAgent::invokePick()
{
   // queue message to dum thread
   mDum->post(new InvokePick(*this));
}

void
UserAgent::invokeJoin()
{
   // queue message to dum thread
   mDum->post(new InvokeJoin(*this));
}

bool
UserAgent::process(int timeoutMs)
{
   if(!mDumShutdown)
   {
      if(mDumShutdownRequested)
      {
         // unregister
         if(mRegHandle.isValid())
         {
            mRegHandle->end();
         }

         // end any subscriptions
         if(mServerSubscriptionHandle.isValid())
         {
            mServerSubscriptionHandle->end();
         }
         if(mClientSubscriptionHandle.isValid())
         {
            mClientSubscriptionHandle->end();
         }

         onEndCalls();

         mDum->shutdown(this);
         mDumShutdownRequested = false;
      }
      mDum->process(timeoutMs);
      return true;
   }
   return false;
}

void
UserAgent::addTransport(TransportType type, int port)
{
   if(port == 0) return;  // Transport disabled

   for (int i=0; i < 10; ++i)
   {
      try
      {
         if (!mNoV4)
         {
            mStack->addTransport(type, port+i, V4, StunEnabled, Data::Empty, mTlsDomain);
            return;
         }

         if (mEnableV6)
         {
            mStack->addTransport(type, port+i, V6, StunEnabled, Data::Empty, mTlsDomain);
            return;
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
UserAgent::post(Message* msg)
{
   ConnectionTerminated* terminated = dynamic_cast<ConnectionTerminated*>(msg);
   if (terminated)
   {
      InfoLog(<< "BasicClientUserAgent received connection terminated message for: " << terminated->getFlow());
      delete msg;
      return;
   }
   assert(false);
}

void 
UserAgent::onNotifyTimeout(unsigned int timerId)
{
   if(timerId == mCurrentNotifyTimerId)
   {
      sendNotify();
   }
}

void
UserAgent::sendNotify()
{
   if(mServerSubscriptionHandle.isValid())
   {
      PlainContents plain("test notify");
      mServerSubscriptionHandle->send(mServerSubscriptionHandle->update(&plain));

      // start timer for next one
      unique_ptr<ApplicationMessage> timer(new NotifyTimer(*this, ++mCurrentNotifyTimerId));
      mStack->post(std::move(timer), NotifySendTime, mDum);
   }
}

void 
UserAgent::onCallTimeout(Call* call)
{
   if(isValidCall(call))
   {
      call->timerExpired();
   }
   else  // call no longer exists
   {
      // If there are no more calls, then start a new one
      if(mCallList.empty() && !mCallTarget.host().empty())
      {
         // re-start a new call
         Call* newCall = new Call(*this);
         newCall->initiateCall(mCallTarget, mProfile);
      }
   }
}

void 
UserAgent::registerCall(Call* call)
{
   mCallList.insert(call);
}

void 
UserAgent::unregisterCall(Call* call)
{
   std::set<Call*>::iterator it = mCallList.find(call);
   if(it != mCallList.end())
   {
      mCallList.erase(it);
   }
}

bool 
UserAgent::isValidCall(Call* call)
{
   std::set<Call*>::iterator it = mCallList.find(call);
   if(it != mCallList.end())
   {
      return true;
   }
   return false;
}

void 
UserAgent::onDumCanBeDeleted()
{
   mDumShutdown = true;
}

void 
UserAgent::processDialogInfoEvent(ClientSubscriptionHandle h, const resip::SipMessage& msg)
{
   DialogInfoContents* dialogInfo = dynamic_cast<DialogInfoContents*>(msg.getContents());
   if (dialogInfo)
   {
      // Version Number processing
      if (!mCurrentDialogInfoVersionSet)
      {
         // This the first notify.  Following RFC4235 section 4.3, we store the version in the notify
         mCurrentDialogInfoVersion = dialogInfo->getVersion();
         mCurrentDialogInfoVersionSet = true;
      }
      else
      {
         // This is a subsequent notify.  RFC4235 section 4.3:
         // Each time a new document is received, the value of the local version
         // number is compared to the "version" attribute in the new document.
         // If the value in the new document is one higher than the local version
         // number, the local version number is increased by one and the document
         // is processed. If the value in the document is more than one higher
         // than the local version number, the local version number is set to the
         // value in the new document and the document is processed. If the
         // document did not contain full state, the subscriber SHOULD generate a
         // refresh request(SUBSCRIBE) to trigger a full state notification. If
         // the value in the document is less than the local version, the
         // document is discarded without processing.
         if (dialogInfo->getVersion() == mCurrentDialogInfoVersion + 1)
         {
            // all is well
            mCurrentDialogInfoVersion = dialogInfo->getVersion();
         }
         else if (dialogInfo->getVersion() > mCurrentDialogInfoVersion + 1)
         {
            if (dialogInfo->getDialogInfoState() != DialogInfoContents::Full)
            {
               h->requestRefresh();
            }
         }
         else
         {
            // Version is equal or less than - discard document
            WarningLog(<< "processDialogInfoEvent: version in notify (" << dialogInfo->getVersion() << ") is not one greater than last version received (" << mCurrentDialogInfoVersion << ")");
            return;  // do not process
         }
      }

      // Process dialog info
      InfoLog(<< "processDialogInfoEvent: " << *dialogInfo);

      // Store call info for first dialog that is not terminating
      mSubTargetDialogId = DialogId(Data::Empty, Data::Empty, Data::Empty);
      DialogInfoContents::DialogList::const_iterator it = dialogInfo->getDialogs().begin();
      for (; it != dialogInfo->getDialogs().end(); it++)
      {
         if (it->getState() != DialogInfoContents::Terminated)
         {
            mSubTargetDialogId = DialogId(it->getCallId(), it->getLocalTag(), it->getRemoteTag());
            DebugLog(<< "Saving dialog: " << mSubTargetDialogId << " in state: " << it->getState());
            break;
         }
      }
   }
}


////////////////////////////////////////////////////////////////////////////////
// Registration Handler ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
UserAgent::onSuccess(ClientRegistrationHandle h, const SipMessage& msg)
{
   InfoLog(<< "onSuccess(ClientRegistrationHandle): msg=" << msg.brief());
   if(mShuttingdown)
   {
       h->end();
       return;
   }
   if(mRegHandle.getId() == 0)  // Note: reg handle id will only be 0 on first successful registration
   {
      // Check if we should try to form a test subscription
      if(!mSubscribeTarget.host().empty())
      {
         auto sub = mDum->makeSubscription(NameAddr(mSubscribeTarget), mProfile, "dialog");
         mDum->send(std::move(sub));
      }

      // Check if we should try to form a test call
      if(!mCallTarget.host().empty())
      {
         Call* newCall = new Call(*this);
         newCall->initiateCall(mCallTarget, mProfile);
      }
   }
   mRegHandle = h;
   mRegistrationRetryDelayTime = 0;  // reset
}

void
UserAgent::onFailure(ClientRegistrationHandle h, const SipMessage& msg)
{
   InfoLog(<< "onFailure(ClientRegistrationHandle): msg=" << msg.brief());
   mRegHandle = h;
   if(mShuttingdown)
   {
       h->end();
   }
}

void
UserAgent::onRemoved(ClientRegistrationHandle h, const SipMessage&msg)
{
   InfoLog(<< "onRemoved(ClientRegistrationHandle): msg=" << msg.brief());
   mRegHandle = h;
}

bool UserAgent::onRefreshRequired(ClientRegistrationHandle, const SipMessage& lastRequest)
{
    InfoLog(<< "onRefreshRequired(ClientRegistrationHandle): msg=" << lastRequest );

    return(true);
}

int 
UserAgent::onRequestRetry(ClientRegistrationHandle h, int retryMinimum, const SipMessage& msg)
{
   mRegHandle = h;
   if(mShuttingdown)
   {
       return -1;
   }

   if(mRegistrationRetryDelayTime == 0)
   {
      mRegistrationRetryDelayTime = BaseRegistrationRetryTimeAllFlowsFailed; // We only have one flow in this test app
   }

   // Use back off procedures of RFC 5626 section 4.5
   mRegistrationRetryDelayTime = resipMin(MaxRegistrationRetryTime, mRegistrationRetryDelayTime * 2);

   // return an evenly distributed random number between 50% and 100% of mRegistrationRetryDelayTime
   int retryTime = Helper::jitterValue(mRegistrationRetryDelayTime, 50, 100);
   InfoLog(<< "onRequestRetry(ClientRegistrationHandle): msg=" << msg.brief() << ", retryTime=" << retryTime);

   return retryTime;
}


////////////////////////////////////////////////////////////////////////////////
// InviteSessionHandler ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
UserAgent::onNewSession(ClientInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onNewSession(h, oat, msg);
}

void
UserAgent::onNewSession(ServerInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onNewSession(h, oat, msg);
}

void
UserAgent::onFailure(ClientInviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onFailure(h, msg);
}

void
UserAgent::onEarlyMedia(ClientInviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onEarlyMedia(h, msg, sdp);
}

void
UserAgent::onProvisional(ClientInviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onProvisional(h, msg);
}

void
UserAgent::onConnected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onConnected(h, msg);
}

void
UserAgent::onConnected(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onConnected(h, msg);
}

void
UserAgent::onStaleCallTimeout(ClientInviteSessionHandle h)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onStaleCallTimeout(h);
}

void
UserAgent::onTerminated(InviteSessionHandle h, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onTerminated(h, reason, msg);
}

void
UserAgent::onRedirected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onRedirected(h, msg);
}

void
UserAgent::onAnswer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onAnswer(h, msg, sdp);
}

void
UserAgent::onOffer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{         
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onOffer(h, msg, sdp);
}

void
UserAgent::onOfferRequired(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onOfferRequired(h, msg);
}

void
UserAgent::onOfferRejected(InviteSessionHandle h, const SipMessage* msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onOfferRejected(h, msg);
}

void
UserAgent::onOfferRequestRejected(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onOfferRequestRejected(h, msg);
}

void
UserAgent::onRemoteSdpChanged(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onRemoteSdpChanged(h, msg, sdp);
}

void
UserAgent::onInfo(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onInfo(h, msg);
}

void
UserAgent::onInfoSuccess(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onInfoSuccess(h, msg);
}

void
UserAgent::onInfoFailure(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onInfoFailure(h, msg);
}

void
UserAgent::onRefer(InviteSessionHandle h, ServerSubscriptionHandle ssh, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onRefer(h, ssh, msg);
}

void
UserAgent::onReferAccepted(InviteSessionHandle h, ClientSubscriptionHandle csh, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onReferAccepted(h, csh, msg);
}

void
UserAgent::onReferRejected(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onReferRejected(h, msg);
}

void
UserAgent::onReferNoSub(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onReferNoSub(h, msg);
}

void
UserAgent::onMessage(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onMessage(h, msg);
}

void
UserAgent::onMessageSuccess(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onMessageSuccess(h, msg);
}

void
UserAgent::onMessageFailure(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onMessageFailure(h, msg);
}

void
UserAgent::onForkDestroyed(ClientInviteSessionHandle h)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onForkDestroyed(h);
}

void 
UserAgent::onReadyToSend(InviteSessionHandle h, SipMessage& msg)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onReadyToSend(h, msg);
}

void 
UserAgent::onFlowTerminated(InviteSessionHandle h)
{
   dynamic_cast<Call *>(h->getAppDialogSet().get())->onFlowTerminated(h);
}


////////////////////////////////////////////////////////////////////////////////
// DialogSetHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
UserAgent::onTrying(AppDialogSetHandle h, const SipMessage& msg)
{
   Call *call = dynamic_cast<Call *>(h.get());
   if(call)
   {
      call->onTrying(h, msg);
   }
   else
   {
      InfoLog(<< "onTrying(AppDialogSetHandle): " << msg.brief());
   }
}

void 
UserAgent::onNonDialogCreatingProvisional(AppDialogSetHandle h, const SipMessage& msg)
{
   Call *call = dynamic_cast<Call *>(h.get());
   if(call)
   {
      call->onNonDialogCreatingProvisional(h, msg);
   }
   else
   {
      InfoLog(<< "onNonDialogCreatingProvisional(AppDialogSetHandle): " << msg.brief());
   }
}

////////////////////////////////////////////////////////////////////////////////
// ClientSubscriptionHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
UserAgent::onUpdatePending(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   Call* call = dynamic_cast<Call *>(h->getAppDialogSet().get());
   if(call)
   {
      call->onUpdatePending(h, msg, outOfOrder);
      return;
   }
   InfoLog(<< "onUpdatePending(ClientSubscriptionHandle): " << msg.brief());
   if (msg.exists(h_Event) && msg.header(h_Event).value() == "dialog")
   {
      processDialogInfoEvent(h, msg);
   }
   h->acceptUpdate();
}

void
UserAgent::onUpdateActive(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   Call* call = dynamic_cast<Call *>(h->getAppDialogSet().get());
   if(call)
   {
      call->onUpdateActive(h, msg, outOfOrder);
      return;
   }
   InfoLog(<< "onUpdateActive(ClientSubscriptionHandle): " << msg.brief());
   if (msg.exists(h_Event) && msg.header(h_Event).value() == "dialog")
   {
      processDialogInfoEvent(h, msg);
   }
   h->acceptUpdate();
}

void
UserAgent::onUpdateExtension(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   Call* call = dynamic_cast<Call *>(h->getAppDialogSet().get());
   if(call)
   {
      call->onUpdateExtension(h, msg, outOfOrder);
      return;
   }
   InfoLog(<< "onUpdateExtension(ClientSubscriptionHandle): " << msg.brief());
   if (msg.exists(h_Event) && msg.header(h_Event).value() == "dialog")
   {
      processDialogInfoEvent(h, msg);
   }
   h->acceptUpdate();
}

void 
UserAgent::onNotifyNotReceived(ClientSubscriptionHandle h)
{
   Call* call = dynamic_cast<Call *>(h->getAppDialogSet().get());
   if(call)
   {
      call->onNotifyNotReceived(h);
      return;
   }
   WarningLog(<< "onNotifyNotReceived(ClientSubscriptionHandle)");
   h->end();
}

void
UserAgent::onTerminated(ClientSubscriptionHandle h, const SipMessage* msg)
{
   Call* call = dynamic_cast<Call *>(h->getAppDialogSet().get());
   if(call)
   {
      call->onTerminated(h, msg);
      return;
   }
   if(msg)
   {
      InfoLog(<< "onTerminated(ClientSubscriptionHandle): msg=" << msg->brief());
      if (msg->exists(h_Event) && msg->header(h_Event).value() == "dialog")
      {
         processDialogInfoEvent(h, *msg);
      }
   }
   else
   {
      InfoLog(<< "onTerminated(ClientSubscriptionHandle)");
   }
}

void
UserAgent::onNewSubscription(ClientSubscriptionHandle h, const SipMessage& msg)
{
   Call* call = dynamic_cast<Call *>(h->getAppDialogSet().get());
   if(call)
   {
      call->onNewSubscription(h, msg);
      return;
   }
   mClientSubscriptionHandle = h;
   mCurrentDialogInfoVersionSet = false;  // Ensure false when get a new subscription

   InfoLog(<< "onNewSubscription(ClientSubscriptionHandle): msg=" << msg.brief());
}

int 
UserAgent::onRequestRetry(ClientSubscriptionHandle h, int retrySeconds, const SipMessage& msg)
{
   Call* call = dynamic_cast<Call *>(h->getAppDialogSet().get());
   if(call)
   {
      return call->onRequestRetry(h, retrySeconds, msg);
   }
   InfoLog(<< "onRequestRetry(ClientSubscriptionHandle): msg=" << msg.brief());
   return FailedSubscriptionRetryTime;  
}

////////////////////////////////////////////////////////////////////////////////
// ServerSubscriptionHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
UserAgent::onNewSubscription(ServerSubscriptionHandle h, const SipMessage& msg)
{
   InfoLog(<< "onNewSubscription(ServerSubscriptionHandle): " << msg.brief());

   mServerSubscriptionHandle = h;
   mServerSubscriptionHandle->setSubscriptionState(Active);
   mServerSubscriptionHandle->send(mServerSubscriptionHandle->accept());
   sendNotify();
}

void 
UserAgent::onNewSubscriptionFromRefer(ServerSubscriptionHandle ss, const SipMessage& msg)
{
   InfoLog(<< "onNewSubscriptionFromRefer(ServerSubscriptionHandle): " << msg.brief());
   // Received an out-of-dialog refer request with implicit subscription
   try
   {
      if(msg.exists(h_ReferTo))
      {
         // Check if TargetDialog header is present
         if(msg.exists(h_TargetDialog))
         {
            pair<InviteSessionHandle, int> presult;
            presult = mDum->findInviteSession(msg.header(h_TargetDialog));
            if(!(presult.first == InviteSessionHandle::NotValid())) 
            {         
               Call* callToRefer = (Call*)presult.first->getAppDialogSet().get();

               callToRefer->onRefer(presult.first, ss, msg);
               return;
            }
         }

         // We don't support ood refers that don't target a dialog - reject request 
         WarningLog (<< "onNewSubscriptionFromRefer(ServerSubscriptionHandle): Received ood refer (noSub) w/out a Target-Dialog: " << msg.brief());
         ss->send(ss->reject(400));
      }
      else
      {
         WarningLog (<< "onNewSubscriptionFromRefer(ServerSubscriptionHandle): Received refer w/out a Refer-To: " << msg.brief());
         ss->send(ss->reject(400));
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "onNewSubscriptionFromRefer(ServerSubscriptionHandle): exception " << e);
   }
   catch(...)
   {
      WarningLog(<< "onNewSubscriptionFromRefer(ServerSubscriptionHandle): unknown exception");
   }
}

void 
UserAgent::onRefresh(ServerSubscriptionHandle, const SipMessage& msg)
{
   InfoLog(<< "onRefresh(ServerSubscriptionHandle): " << msg.brief());
}

void 
UserAgent::onTerminated(ServerSubscriptionHandle)
{
   InfoLog(<< "onTerminated(ServerSubscriptionHandle)");
}

void 
UserAgent::onReadyToSend(ServerSubscriptionHandle, SipMessage&)
{
}

void 
UserAgent::onNotifyRejected(ServerSubscriptionHandle, const SipMessage& msg)
{
   WarningLog(<< "onNotifyRejected(ServerSubscriptionHandle): " << msg.brief());
}

void 
UserAgent::onError(ServerSubscriptionHandle, const SipMessage& msg)
{
   WarningLog(<< "onError(ServerSubscriptionHandle): " << msg.brief());
}

void 
UserAgent::onExpiredByClient(ServerSubscriptionHandle, const SipMessage& sub, SipMessage& notify)
{
   InfoLog(<< "onExpiredByClient(ServerSubscriptionHandle): " << notify.brief());
}

void 
UserAgent::onExpired(ServerSubscriptionHandle, SipMessage& msg)
{
   InfoLog(<< "onExpired(ServerSubscriptionHandle): " << msg.brief());
}

bool 
UserAgent::hasDefaultExpires() const
{
   return true;
}

uint32_t 
UserAgent::getDefaultExpires() const
{
   return 60;
}

////////////////////////////////////////////////////////////////////////////////
// OutOfDialogHandler //////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
UserAgent::onSuccess(ClientOutOfDialogReqHandle, const SipMessage& msg)
{
   InfoLog(<< "onSuccess(ClientOutOfDialogReqHandle): " << msg.brief());
}

void 
UserAgent::onFailure(ClientOutOfDialogReqHandle h, const SipMessage& msg)
{
   WarningLog(<< "onFailure(ClientOutOfDialogReqHandle): " << msg.brief());
}

void 
UserAgent::onReceivedRequest(ServerOutOfDialogReqHandle ood, const SipMessage& msg)
{
   InfoLog(<< "onReceivedRequest(ServerOutOfDialogReqHandle): " << msg.brief());

   switch(msg.method())
   {
   case OPTIONS:
      {
         auto optionsAnswer = ood->answerOptions();
         ood->send(std::move(optionsAnswer));
         break;
      }
   default:
      ood->send(ood->reject(501 /* Not Implemented*/));
      break;
   }
}

////////////////////////////////////////////////////////////////////////////////
// RedirectHandler /////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
UserAgent::onRedirectReceived(AppDialogSetHandle h, const SipMessage& msg)
{
   Call* call = dynamic_cast<Call *>(h.get());
   if(call)
   {
      call->onRedirectReceived(h, msg);
   }
   else
   {
      InfoLog(<< "onRedirectReceived(AppDialogSetHandle): " << msg.brief());
   }
}

bool 
UserAgent::onTryingNextTarget(AppDialogSetHandle, const SipMessage& msg)
{
   InfoLog(<< "onTryingNextTarget(AppDialogSetHandle): " << msg.brief());

   // Always allow redirection for now
   return true;
}

void UserAgent::onEndCalls()
{
   // End all calls - copy list in case delete/unregister of call is immediate
   std::set<Call*> tempCallList = mCallList;
   std::set<Call*>::iterator it = tempCallList.begin();
   for (; it != tempCallList.end(); it++)
   {
      (*it)->terminateCall();
   }
}

void UserAgent::onToggleHold()
{
   mHoldCalls = !mHoldCalls;

   int callsToggled = 0;
   for (std::set<Call*>::iterator callIt = mCallList.begin();
      callIt != mCallList.end();
      callIt++)
   {
      if ((*callIt)->toggleHold())
      {
         callsToggled++;
      }
   }
   InfoLog(<< "onToggleHold: toggled " << callsToggled << " calls.");
}

void
UserAgent::onInvokePick()
{
   if (mSubTargetDialogId.getCallId().empty())
   {
      InfoLog(<< "onInvokePick: subscribe target has no calls.");
   }
   else
   {
      InfoLog(<< "onInvokePick: callId=" << mSubTargetDialogId.getCallId() << ", localTag=" << mSubTargetDialogId.getLocalTag() << ", remoteTag=" << mSubTargetDialogId.getRemoteTag());

      Call* newCall = new Call(*this);
      newCall->initiateCall(mSubscribeTarget, mProfile, &mSubTargetDialogId, false);
   }
}

void
UserAgent::onInvokeJoin()
{
   if (mSubTargetDialogId.getCallId().empty())
   {
      InfoLog(<< "onInvokeJoin: subscribe target has no calls.");
   }
   else
   {
      InfoLog(<< "onInvokeJoin: callId=" << mSubTargetDialogId.getCallId() << ", localTag=" << mSubTargetDialogId.getLocalTag() << ", remoteTag=" << mSubTargetDialogId.getRemoteTag());

      Call* newCall = new Call(*this);
      newCall->initiateCall(mSubscribeTarget, mProfile, &mSubTargetDialogId, true);
   }
}



/* ====================================================================

 Copyright (c) 2021, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */

