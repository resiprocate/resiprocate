#ifdef WIN32
#include <process.h>
#else
#include <spawn.h>
#endif

#include "Server.hxx"
#include "B2BSession.hxx"
#include "ClickToCallCmds.hxx"
#include "AppSubsystem.hxx"

#include <rutil/DnsUtil.hxx>
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/ThreadIf.hxx>
#include <resip/stack/TransactionTerminated.hxx>
#include <resip/stack/ConnectionTerminated.hxx>
#include <resip/stack/ApplicationMessage.hxx>
#include <resip/stack/ssl/Security.hxx>
#include <resip/stack/UdpTransport.hxx>
#include <resip/stack/Helper.hxx>
#include <resip/dum/KeepAliveManager.hxx>
#include <resip/dum/ServerOutOfDialogReq.hxx>
#include <resip/dum/AppDialogSet.hxx>
#include <resip/dum/InviteSession.hxx>
#include <resip/dum/ClientInviteSession.hxx>
#include <resip/dum/ServerInviteSession.hxx>
#include <resip/dum/ClientSubscription.hxx>
#include <resip/dum/AppDialogSetFactory.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace clicktocall;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::CLICKTOCALL

namespace clicktocall
{

class SdpMessageDecorator : public MessageDecorator
{
public:
   virtual ~SdpMessageDecorator() {}
   virtual void decorateMessage(SipMessage &msg, 
                                const Tuple &source,
                                const Tuple &destination,
                                const Data& sigcompId)
   {
      SdpContents* sdp = dynamic_cast<SdpContents*>(msg.getContents());
      if(sdp && sdp->session().media().size() > 0 &&
         sdp->session().origin().user() == "clicktocall")
      {
         // Fill in IP and Port from source (if required)
         if(sdp->session().connection().getAddress() == Data("0.0.0.0"))
         {
            sdp->session().connection().setAddress(Tuple::inet_ntop(source), source.ipVersion() == V6 ? SdpContents::IP6 : SdpContents::IP4);
         }
         if(sdp->session().origin().getAddress() == Data("0.0.0.0"))
         {
            sdp->session().origin().setAddress(Tuple::inet_ntop(source), source.ipVersion() == V6 ? SdpContents::IP6 : SdpContents::IP4);
         }
         InfoLog( << "SdpMessageDecorator: src=" << source << ", dest=" << destination << ", msg=" << endl << msg);
      }
   }
   virtual void rollbackMessage(SipMessage& msg) {}  // Nothing to do
   virtual MessageDecorator* clone() const { return new SdpMessageDecorator; }
};

class ShutdownCmd  : public resip::DumCommand
{
   public:  
      ShutdownCmd(Server* server)
         : mServer(server) {}
      virtual void executeCommand()
      {
         mServer->shutdownImpl();
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " ShutdownCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      Server* mServer;
};

class ClickToCallDialogSetFactory : public resip::AppDialogSetFactory
{
public:
   ClickToCallDialogSetFactory(Server& server) : mServer(server) {}
   resip::AppDialogSet* createAppDialogSet(resip::DialogUsageManager& dum,
                                           const resip::SipMessage& msg)
   {
      switch(msg.method())
      {
      case INVITE:
         return new B2BSession(mServer);
         break;
      default:
         resip_assert(false);
         return 0;
         //return new DefaultDialogSet(mConversationManager);
         break;
      }
   }
private:
   Server& mServer;
};

class ClickToCallLogger : public ExternalLogger
{
public:
   virtual ~ClickToCallLogger() {}
   /** return true to also do default logging, false to supress default logging. */
   virtual bool operator()(Log::Level level,
                           const Subsystem& subsystem, 
                           const Data& appName,
                           const char* file,
                           int line,
                           const Data& message,
                           const Data& messageWithHeaders)
   {
      // Log any warnings/errors to the screen and all Gateway logging messages
      if(level <= Log::Warning || subsystem.getSubsystem() == AppSubsystem::CLICKTOCALL.getSubsystem())
      {
         resipCout << messageWithHeaders << endl;
      }
      return true;
   }
};
ClickToCallLogger g_ClickToCallLogger;

Server::Server(int argc, char** argv) : 
   ConfigParser(argc, argv),
   mProfile(new MasterProfile),
#if defined(USE_SSL)
   mSecurity(new Security(".")),
#else
   mSecurity(0),
#endif
   mStack(mSecurity, mDnsServers, &mSelectInterruptor),
   mDum(mStack),
   mStackThread(mStack, mSelectInterruptor),
   mDumShutdown(false),
   mCurrentB2BSessionHandle(1),
   mIsV6Avail(false),
   mWebAdminV4(0),
   mWebAdminV6(0),
   mWebAdminThread(0),
   mXmlRpcServerV4(0),
   mXmlRpcServerV6(0),
   mXmlRpcServerThread(0)
{
   GenericLogImpl::MaxLineCount = mLogFileMaxLines; 
   Log::initialize("file", mLogLevel, "", mLogFilename.c_str(), (ExternalLogger*)&g_ClickToCallLogger);
   //UserAgent::setLogLevel(Log::Warning, UserAgent::SubsystemAll);
   //UserAgent::setLogLevel(Log::Info, UserAgent::SubsystemGateway);

   InfoLog( << "ClickToCall settings:");
   InfoLog( << "  Local IP Address = " << mAddress);
   Data dnsServersString;
   for(unsigned int i = 0; i != (unsigned int)mDnsServers.size(); i++)
   {
      if(i!=0) dnsServersString += ", ";
      dnsServersString += Tuple(mDnsServers[i]).presentationFormat();
   }
   InfoLog( << "  Override DNS Servers = " << dnsServersString);
   InfoLog( << "  ClickToCall Identity = " << mClickToCallIdentity);
   InfoLog( << "  SIP Port = " << mSipPort);
   InfoLog( << "  TLS Port = " << mTlsPort);
   InfoLog( << "  TLS Domain = " << mTlsDomain);
   InfoLog( << "  Keepalives = " << (mKeepAlives ? "enabled" : "disabled"));
   InfoLog( << "  Outbound Proxy = " << mOutboundProxy);
   InfoLog( << "  Log Level = " << mLogLevel);
   InfoLog( << "  XML RPC Port = " << mXmlRpcPort);
   InfoLog( << "  HttpPort = " << mHttpPort);
   InfoLog( << "  HttpAuth = " << (mHttpAuth ? "enabled" : "disabled"));

   // Install Translation Rules
   TranslationList::iterator itTrans = mAddressTranslations.begin();
   for(;itTrans!=mAddressTranslations.end();itTrans++)
   {
      mAddressTranslator.addTranslation(itTrans->first, itTrans->second);
   }

   if(!mAddress.empty())
   {
      // If address is specified in config file, then just use this address only
      Tuple myTuple(mAddress, mSipPort, UDP);
      if(myTuple.ipVersion() == V6)
      {
         mIsV6Avail = true;
      }
   }
   else
   {
      // If address in config is empty - query network interface info
      std::list<std::pair<Data,Data> > interfaces = DnsUtil::getInterfaces();
      std::list<std::pair<Data,Data> >::iterator itInt = interfaces.begin();
      for(;itInt != interfaces.end(); itInt++)
      {
         Tuple myTuple(itInt->second, mSipPort, UDP);
         if(myTuple.ipVersion() == V6)
         {
            mIsV6Avail = true;
         }
      }
   }

   // Add transports
   try
   {
      UdpTransport* udpTransport = (UdpTransport*)mStack.addTransport(UDP, mSipPort, DnsUtil::isIpV6Address(mAddress) ? V6 : V4, StunEnabled, mAddress);
      mStack.addTransport(TCP, mSipPort, DnsUtil::isIpV6Address(mAddress) ? V6 : V4, StunEnabled, mAddress);
      mStack.addTransport(TLS, mTlsPort, DnsUtil::isIpV6Address(mAddress) ? V6 : V4, StunEnabled, mAddress, mTlsDomain);
      if(mAddress.empty() && mIsV6Avail)
      {
         // if address is empty (ie. all interfaces), then create V6 transports too
         udpTransport = (UdpTransport*)mStack.addTransport(UDP, mSipPort, V6, StunEnabled, mAddress);
         mStack.addTransport(TCP, mSipPort, V6, StunEnabled, mAddress);
         mStack.addTransport(TLS, mTlsPort, V6, StunEnabled, mAddress, mTlsDomain);
      }
   }
   catch (BaseException& e)
   {
      std::cerr << "Likely a port is already in use" << endl;
      InfoLog (<< "Caught: " << e);
      exit(-1);
   }

   // Disable Statisitics Manager
   mStack.statisticsManagerEnabled() = false;

   // Setup MasterProfile
   if(mKeepAlives)
   {
      mProfile->setKeepAliveTimeForDatagram(30);
      mProfile->setKeepAliveTimeForStream(180);
   }

   // Supported Methods, etc.
   mProfile->validateContentEnabled() = false;
   mProfile->validateContentLanguageEnabled() = false;
   mProfile->validateAcceptEnabled() = false;

   mProfile->clearSupportedLanguages();
   mProfile->addSupportedLanguage(Token("en"));  

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
   mProfile->addSupportedMimeType(NOTIFY, Mime("message", "sipfrag"));  
   mProfile->addSupportedMimeType(NOTIFY, Mime("multipart", "mixed"));  
   mProfile->addSupportedMimeType(NOTIFY, Mime("multipart", "signed"));  
   mProfile->addSupportedMimeType(NOTIFY, Mime("multipart", "alternative"));  
   //profile->addSupportedMimeType(SUBSCRIBE, Mime("application", "simple-message-summary"));  

   mProfile->clearSupportedMethods();
   mProfile->addSupportedMethod(INVITE);
   mProfile->addSupportedMethod(ACK);
   mProfile->addSupportedMethod(CANCEL);
   mProfile->addSupportedMethod(OPTIONS);
   mProfile->addSupportedMethod(BYE);
   mProfile->addSupportedMethod(REFER);    
   mProfile->addSupportedMethod(NOTIFY);    
   mProfile->addSupportedMethod(SUBSCRIBE); 
   mProfile->addSupportedMethod(UPDATE);    
   mProfile->addSupportedMethod(PRACK);     
   mProfile->addSupportedMethod(INFO);    
   mProfile->addSupportedMethod(MESSAGE);

   mProfile->clearSupportedOptionTags();
   mProfile->addSupportedOptionTag(Token(Symbols::Replaces));      
   mProfile->addSupportedOptionTag(Token(Symbols::Timer)); 
   mProfile->addSupportedOptionTag(Token(Symbols::NoReferSub));
   //mProfile->addSupportedOptionTag(Token(Symbols::AnswerMode));
   //mProfile->addSupportedOptionTag(Token(Symbols::TargetDialog));
   //mProfile->addSupportedOptionTag(Token(Symbols::C100rel));  // Automatically added by calling setUacReliableProvisionalMode

   mProfile->setUacReliableProvisionalMode(MasterProfile::Supported);

   mProfile->clearSupportedSchemes();
   mProfile->addSupportedScheme("sip");  
#ifdef USE_SSL
   mProfile->addSupportedScheme("sips");
#endif

   // Have stack add Allow/Supported/Accept headers to INVITE dialog establishment messages
   mProfile->clearAdvertisedCapabilities(); // Remove Profile Defaults, then add our preferences
   mProfile->addAdvertisedCapability(Headers::Allow);  
   //mProfile->addAdvertisedCapability(Headers::AcceptEncoding);  // This can be misleading - it might specify what is expected in response
   mProfile->addAdvertisedCapability(Headers::AcceptLanguage);  
   mProfile->addAdvertisedCapability(Headers::Supported);  
   mProfile->setMethodsParamEnabled(true);

   mProfile->setDefaultFrom(mClickToCallIdentity);

   //profile->setOverrideHostAndPort(mContact);
   if(!mOutboundProxy.uri().host().empty())
   {
      mProfile->setOutboundProxy(mOutboundProxy.uri());
   }

   mProfile->setUserAgent("ClickToCall");

   // Install Handlers
   mDum.setMasterProfile(mProfile);
   //mDum.setClientAuthManager(std::auto_ptr<ClientAuthManager>(new ClientAuthManager));
   mDum.setKeepAliveManager(std::auto_ptr<KeepAliveManager>(new KeepAliveManager));

   // Install Sdp Message Decorator
   SharedPtr<MessageDecorator> outboundDecorator(new SdpMessageDecorator);
   mProfile->setOutboundDecorator(outboundDecorator);

   // Install this Server as handler
   mDum.setInviteSessionHandler(this); 
   mDum.setDialogSetHandler(this);
   mDum.addOutOfDialogHandler(OPTIONS, this);
   mDum.addOutOfDialogHandler(REFER, this);
   mDum.setRedirectHandler(this);
   mDum.addClientSubscriptionHandler("refer", this);
   mDum.addServerSubscriptionHandler("refer", this);

   // Set AppDialogSetFactory
   auto_ptr<AppDialogSetFactory> dsf(new ClickToCallDialogSetFactory(*this));
	mDum.setAppDialogSetFactory(dsf);

#if 0
   // Set UserAgentServerAuthManager
   SharedPtr<ServerAuthManager> uasAuth( new AppServerAuthManager(*this));
   mDum.setServerAuthManager(uasAuth);
#endif

   // Create Http Server
   if(mHttpPort != 0)
   {
      std::list<WebAdmin*> webAdminList;
      mWebAdminV4 = new WebAdmin(*this, !mHttpAuth /* web challenge */, Data::Empty, mHttpAuthPwd, mHttpPort, V4);
      webAdminList.push_back(mWebAdminV4);
      if(mIsV6Avail)
      {
         mWebAdminV6 = new WebAdmin(*this, !mHttpAuth /* web challenge */, Data::Empty, mHttpAuthPwd, mHttpPort, V6);
         webAdminList.push_back(mWebAdminV6);
      }
      mWebAdminThread = new WebAdminThread(webAdminList);
   }

   // Create Xml Rpc Server
   if(mXmlRpcPort != 0)
   {
      std::list<XmlRpcServer*> xmlRpcServerList;
      mXmlRpcServerV4 = new XmlRpcServer(*this, mXmlRpcPort, V4);  
      xmlRpcServerList.push_back(mXmlRpcServerV4);
      if(mIsV6Avail)
      {
         mXmlRpcServerV6 = new XmlRpcServer(*this, !mXmlRpcPort, V6);
         xmlRpcServerList.push_back(mXmlRpcServerV6);
      }
      mXmlRpcServerThread = new XmlRpcServerThread(xmlRpcServerList);
   }
}

Server::~Server()
{
   shutdown();

   if(mWebAdminThread) delete mWebAdminThread;
   if(mWebAdminV4) delete mWebAdminV4;
   if(mWebAdminV6) delete mWebAdminV6;

   if(mXmlRpcServerThread) delete mXmlRpcServerThread;
   if(mXmlRpcServerV4) delete mXmlRpcServerV4;
   if(mXmlRpcServerV6) delete mXmlRpcServerV6;
}

void
Server::startup()
{
   mStackThread.run(); 
   if(mWebAdminThread) mWebAdminThread->run();
   if(mXmlRpcServerThread) mXmlRpcServerThread->run();
}

void 
Server::process(int timeoutMs)
{
   mDum.process(timeoutMs);
}

void
Server::shutdown()
{
   ShutdownCmd* cmd = new ShutdownCmd(this);
   mDum.post(cmd);

   // Wait for Dum to shutdown
   while(!mDumShutdown) 
   {
      process(100);
   }

   mStackThread.shutdown();
   mStackThread.join();

   if(mWebAdminThread) mWebAdminThread->shutdown();
   if(mWebAdminThread) mWebAdminThread->join();
   
   if(mXmlRpcServerThread) mXmlRpcServerThread->shutdown();
   if(mXmlRpcServerThread) mXmlRpcServerThread->join();
}

void 
Server::post(ApplicationMessage& message, unsigned int ms)
{
   if(ms > 0)
   {
      mStack.postMS(message, ms, &mDum);
   }
   else
   {
      mDum.post(&message);
   }
}

void 
Server::setLogLevel(Log::Level level, LoggingSubsystem subsystem)
{
   switch(subsystem)
   {
   case SubsystemAll:
      Log::setLevel(level);
      break;
   case SubsystemContents:
      Log::setLevel(level, Subsystem::CONTENTS);
      break;
   case SubsystemDns:
      Log::setLevel(level, Subsystem::DNS);
      break;
   case SubsystemDum:
      Log::setLevel(level, Subsystem::DUM);
      break;
   case SubsystemSdp:
      Log::setLevel(level, Subsystem::SDP);
      break;
   case SubsystemSip:
      Log::setLevel(level, Subsystem::SIP);
      break;
   case SubsystemTransaction:
      Log::setLevel(level, Subsystem::TRANSACTION);
      break;
   case SubsystemTransport:
      Log::setLevel(level, Subsystem::TRANSPORT);
      break;
   case SubsystemStats:
      Log::setLevel(level, Subsystem::STATS);
      break;
   case SubsystemClickToCall:
      Log::setLevel(level, AppSubsystem::CLICKTOCALL);
      break;
   }
}

void
Server::clickToCall(const resip::Uri& initiator, const resip::Uri& destination, bool anchorCall, XmlRpcInfo* xmlRpcInfo)
{
   ClickToCallCmd* cmd = new ClickToCallCmd(*this, initiator, destination, anchorCall, xmlRpcInfo ? *xmlRpcInfo : XmlRpcInfo());
   mDum.post(cmd);
}

void
Server::clickToCallImpl(const resip::Uri& initiator, const resip::Uri& destination, bool anchorCall, const XmlRpcInfo& xmlRpcInfo)
{
   InfoLog(<< "Server::clickToCallImpl: initiating ClickToCall from " << initiator << " to " << destination << (anchorCall? " (anchored), connectionId=" : " (unanchored), connectionId=") << xmlRpcInfo.mConnectionId << ", requestId=" << xmlRpcInfo.mRequestId);

   // Create B2BCall
   B2BSession* initiatorSession = new B2BSession(*this);

   // Tag session as Click-To-Call
   initiatorSession->clickToCall(initiator, destination, anchorCall, xmlRpcInfo);
}

DialogUsageManager& 
Server::getDialogUsageManager()
{
   return mDum;
}

B2BSession* 
Server::getB2BSession(const B2BSessionHandle& handle) const
{
   B2BSessionMap::const_iterator i = mB2BSessions.find(handle);
   if(i != mB2BSessions.end())
   {
      return i->second;
   }
   else
   {
      return 0;
   }
}

B2BSessionHandle
Server::registerB2BSession(B2BSession *b2bSession)
{
   mB2BSessions[mCurrentB2BSessionHandle] = b2bSession;
   return mCurrentB2BSessionHandle++;
}
 
void 
Server::unregisterB2BSession(const B2BSessionHandle& handle)
{
   InfoLog(<< "unregisterB2BSession: B2B session unregistered, handle=" << handle);
   mB2BSessions.erase(handle);
}

void 
Server::onDumCanBeDeleted()
{
   mDumShutdown = true;
}

void 
Server::shutdownImpl()
{
   // End each B2BSession - keep track of last session handle to ended since ending a session may end up removing 1 or 2 items from the map
   B2BSessionHandle lastSessionHandleEnded=0;
   B2BSessionMap::iterator it = mB2BSessions.begin();
   bool sessionEnded = true;
   while(it != mB2BSessions.end() && sessionEnded)
   {
      sessionEnded=false;
      for(; it != mB2BSessions.end(); it++)
      {
         if(it->second->getB2BSessionHandle() > lastSessionHandleEnded)
         {
            sessionEnded=true;
            InfoLog(<< "Destroying B2BSession: " << it->second->getB2BSessionHandle());
            lastSessionHandleEnded = it->second->getB2BSessionHandle();
            it->second->end();
            it = mB2BSessions.begin();  // iterator can be invalidated after end() call, reset it here
            break;
         }
      }
   }

   mDum.shutdown(this);
}

bool 
Server::translateAddress(const Data& address, Data& translation, bool failIfNoRule)
{
   return mAddressTranslator.translate(address, translation, failIfNoRule);
}


////////////////////////////////////////////////////////////////////////////////
// InviteSessionHandler      ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void
Server::onNewSession(ClientInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onNewSession(h, oat, msg);
}

void
Server::onNewSession(ServerInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onNewSession(h, oat, msg);
}

void
Server::onFailure(ClientInviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onFailure(h, msg);
}
      
void
Server::onEarlyMedia(ClientInviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onEarlyMedia(h, msg, sdp);
}

void
Server::onProvisional(ClientInviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onProvisional(h, msg);
}

void
Server::onConnected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onConnected(h, msg);
}

void
Server::onConnected(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onConnected(h, msg);
}

void
Server::onStaleCallTimeout(ClientInviteSessionHandle h)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onStaleCallTimeout(h);
}

void
Server::onTerminated(InviteSessionHandle h, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onTerminated(h, reason, msg);
}

void
Server::onRedirected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onRedirected(h, msg);
}

void
Server::onAnswer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onAnswer(h, msg, sdp);
}

void
Server::onOffer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{         
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onOffer(h, msg, sdp);
}

void
Server::onOfferRequired(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onOfferRequired(h, msg);
}

void
Server::onOfferRejected(InviteSessionHandle h, const SipMessage* msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onOfferRejected(h, msg);
}

void
Server::onOfferRequestRejected(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onOfferRequestRejected(h, msg);
}

void
Server::onRemoteSdpChanged(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onRemoteSdpChanged(h, msg, sdp);
}

void
Server::onInfo(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onInfo(h, msg);
}

void
Server::onInfoSuccess(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onInfoSuccess(h, msg);
}

void
Server::onInfoFailure(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onInfoFailure(h, msg);
}

void
Server::onRefer(InviteSessionHandle h, ServerSubscriptionHandle ssh, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onRefer(h, ssh, msg);
}

void
Server::onReferAccepted(InviteSessionHandle h, ClientSubscriptionHandle csh, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onReferAccepted(h, csh, msg);
}

void
Server::onReferRejected(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onReferRejected(h, msg);
}

void
Server::onReferNoSub(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onReferNoSub(h, msg);
}

void
Server::onMessage(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onMessage(h, msg);
}

void
Server::onMessageSuccess(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onMessageSuccess(h, msg);
}

void
Server::onMessageFailure(InviteSessionHandle h, const SipMessage& msg)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onMessageFailure(h, msg);
}

void
Server::onForkDestroyed(ClientInviteSessionHandle h)
{
   dynamic_cast<B2BSession *>(h->getAppDialogSet().get())->onForkDestroyed(h);
}

////////////////////////////////////////////////////////////////////////////////
// DialogSetHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
Server::onTrying(AppDialogSetHandle h, const SipMessage& msg)
{
   B2BSession *B2BSessionDialogSet = dynamic_cast<B2BSession *>(h.get());
   if(B2BSessionDialogSet)
   {
      B2BSessionDialogSet->onTrying(h, msg);
   }
   else
   {
      InfoLog(<< "onTrying(AppDialogSetHandle): " << msg.brief());
   }
}

void 
Server::onNonDialogCreatingProvisional(AppDialogSetHandle h, const SipMessage& msg)
{
   B2BSession *B2BSessionDialogSet = dynamic_cast<B2BSession *>(h.get());
   if(B2BSessionDialogSet)
   {
      B2BSessionDialogSet->onNonDialogCreatingProvisional(h, msg);
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
Server::onUpdatePending(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   B2BSession* session = dynamic_cast<B2BSession *>(h->getAppDialogSet().get());
   if(session)
   {
      session->onUpdatePending(h, msg, outOfOrder);
   }
   else
   {
      InfoLog(<< "onUpdatePending(ClientSub): " << msg.brief());
      h->rejectUpdate(400, Data("Received notify out of context."));
   }
}

void
Server::onUpdateActive(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   B2BSession* session = dynamic_cast<B2BSession *>(h->getAppDialogSet().get());
   if(session)
   {
      session->onUpdateActive(h, msg, outOfOrder);
   }
   else
   {
      InfoLog(<< "onUpdateActive(ClientSub): " << msg.brief());
      h->rejectUpdate(400, Data("Received notify out of context."));
   }
}

void
Server::onUpdateExtension(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   B2BSession* session = dynamic_cast<B2BSession *>(h->getAppDialogSet().get());
   if(session)
   {
      session->onUpdateExtension(h, msg, outOfOrder);
   }
   else
   {
      InfoLog(<< "onUpdateExtension(ClientSub): " << msg.brief());
      h->rejectUpdate(400, Data("Received notify out of context."));
   }
}

void 
Server::onNotifyNotReceived(resip::ClientSubscriptionHandle h)
{
   B2BSession* session = dynamic_cast<B2BSession *>(h->getAppDialogSet().get());
   if(session)
   {
      session->onNotifyNotReceived(h);
   }
   else
   {
      InfoLog(<< "onNotifyNotReceived(ClientSub)");
   }
}

void
Server::onTerminated(ClientSubscriptionHandle h, const SipMessage* msg)
{
   B2BSession* session = dynamic_cast<B2BSession *>(h->getAppDialogSet().get());
   if(session)
   {
      session->onTerminated(h, msg);
   }
   else
   {
      if(msg)
      {
         InfoLog(<< "onTerminated(ClientSub): " << msg->brief());
      }
      else
      {
         InfoLog(<< "onTerminated(ClientSub)");
      }
   }
}

void
Server::onNewSubscription(ClientSubscriptionHandle h, const SipMessage& msg)
{
   B2BSession* session = dynamic_cast<B2BSession *>(h->getAppDialogSet().get());
   if(session)
   {
      session->onNewSubscription(h, msg);
   }
   else
   {
      InfoLog(<< "onNewSubscription(ClientSub): " << msg.brief());
   }
}

int 
Server::onRequestRetry(ClientSubscriptionHandle h, int retryMinimum, const SipMessage& msg)
{
   B2BSession* session = dynamic_cast<B2BSession *>(h->getAppDialogSet().get());
   if(session)
   {
      return session->onRequestRetry(h, retryMinimum, msg);
   }
   else
   {
      InfoLog(<< "onRequestRetry(ClientSubscriptionHandle): " << msg.brief());   
      return -1;
   }
}

////////////////////////////////////////////////////////////////////////////////
// ServerSubscriptionHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
Server::onNewSubscription(ServerSubscriptionHandle, const SipMessage& msg)
{
   InfoLog(<< "onNewSubscription(ServerSubscriptionHandle): " << msg.brief());
}

void 
Server::onNewSubscriptionFromRefer(ServerSubscriptionHandle ss, const SipMessage& msg)
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
            presult = mDum.findInviteSession(msg.header(h_TargetDialog));
            if(!(presult.first == InviteSessionHandle::NotValid())) 
            {         
               B2BSession* session = (B2BSession*)presult.first->getAppDialogSet().get();

               session->onRefer(presult.first, ss, msg);
               return;
            }
         }

         // We don't support this yet - reject request without a target dialog header
         WarningLog (<< "onNewSubscriptionFromRefer(ServerSubscriptionHandle): Received ood refer w/out a Target-Dialog: " << msg.brief());
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
      WarningLog(<< "onNewSubscriptionFromRefer(ServerSubscriptionHandle):  unknown exception");
   }
}

void 
Server::onRefresh(ServerSubscriptionHandle, const SipMessage& msg)
{
   InfoLog(<< "onRefresh(ServerSubscriptionHandle): " << msg.brief());
}

void 
Server::onTerminated(ServerSubscriptionHandle)
{
   InfoLog(<< "onTerminated(ServerSubscriptionHandle)");
}

void 
Server::onReadyToSend(ServerSubscriptionHandle, SipMessage&)
{
}

void 
Server::onNotifyRejected(ServerSubscriptionHandle, const SipMessage& msg)
{
   WarningLog(<< "onNotifyRejected(ServerSubscriptionHandle): " << msg.brief());
}

void 
Server::onError(ServerSubscriptionHandle, const SipMessage& msg)
{
   WarningLog(<< "onError(ServerSubscriptionHandle): " << msg.brief());
}

void 
Server::onExpiredByClient(ServerSubscriptionHandle, const SipMessage& sub, SipMessage& notify)
{
   InfoLog(<< "onExpiredByClient(ServerSubscriptionHandle): " << notify.brief());
}

void 
Server::onExpired(ServerSubscriptionHandle, SipMessage& msg)
{
   InfoLog(<< "onExpired(ServerSubscriptionHandle): " << msg.brief());
}

bool 
Server::hasDefaultExpires() const
{
   return true;
}

UInt32 
Server::getDefaultExpires() const
{
   return 60;
}

////////////////////////////////////////////////////////////////////////////////
// OutOfDialogHandler //////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
Server::onSuccess(ClientOutOfDialogReqHandle, const SipMessage& msg)
{
   InfoLog(<< "onSuccess(ClientOutOfDialogReqHandle): " << msg.brief());
}

void 
Server::onFailure(ClientOutOfDialogReqHandle, const SipMessage& msg)
{
   InfoLog(<< "onFailure(ClientOutOfDialogReqHandle): " << msg.brief());
}

void 
Server::onReceivedRequest(ServerOutOfDialogReqHandle ood, const SipMessage& msg)
{
   InfoLog(<< "onReceivedRequest(ServerOutOfDialogReqHandle): " << msg.brief());

   switch(msg.method())
   {
   case OPTIONS:
   {
      SharedPtr<SipMessage> optionsAnswer = ood->answerOptions();
      ood->send(optionsAnswer);
      break;
   }
   case REFER:
   {
      // Received an OOD refer request with no refer subscription
      try
      {
         if(msg.exists(h_ReferTo))
         {
            // Check if TargetDialog header is present
            if(msg.exists(h_TargetDialog))
            {
               pair<InviteSessionHandle, int> presult;
               presult = mDum.findInviteSession(msg.header(h_TargetDialog));
               if(!(presult.first == InviteSessionHandle::NotValid())) 
               {         
                  B2BSession* session = (B2BSession*)presult.first->getAppDialogSet().get();

                  if(session->doReferNoSub(msg))
                  {
                     // Accept the Refer
                     ood->send(ood->accept(202 /* Refer Accepted */));
                  }
                  else
                  {
                     ood->send(ood->reject(403));
                  }
                  return;
               }
            }

            // We don't support this yet - reject request without a target dialog header
            WarningLog (<< "onReceivedRequest(ServerOutOfDialogReqHandle): Received ood refer (noSub) w/out a Target-Dialog: " << msg.brief());
            ood->send(ood->reject(400));
         }
         else
         {
            WarningLog (<< "onReceivedRequest(ServerOutOfDialogReqHandle): Received refer w/out a Refer-To: " << msg.brief());
            ood->send(ood->reject(400));
         }
      }
      catch(BaseException &e)
      {
         WarningLog(<< "onReceivedRequest(ServerOutOfDialogReqHandle): exception " << e);
      }
      catch(...)
      {
         WarningLog(<< "onReceivedRequest(ServerOutOfDialogReqHandle):  unknown exception");
      }
      break;
   }
   default:
      break;
   }
}

////////////////////////////////////////////////////////////////////////////////
// RedirectHandler /////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
Server::onRedirectReceived(AppDialogSetHandle, const SipMessage& msg)
{
   InfoLog(<< "onRedirectReceived(AppDialogSetHandle): " << msg.brief());
}

bool 
Server::onTryingNextTarget(AppDialogSetHandle, const SipMessage& msg)
{
   InfoLog(<< "onTryingNextTarget(AppDialogSetHandle): " << msg.brief());
   // Always allow redirection for now
   return true;
}

}

/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
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

