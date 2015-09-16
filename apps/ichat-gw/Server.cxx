#ifdef WIN32
#include <process.h>
#else
#include <spawn.h>
#endif

#include "Server.hxx"
#include "B2BSession.hxx"
#include "IChatGatewayCmds.hxx"
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
#include <resip/dum/ClientRegistration.hxx>
#include <resip/dum/AppDialogSetFactory.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace gateway;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::GATEWAY

namespace gateway
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
         sdp->session().origin().user() == SDP_ICHATGW_ORIGIN_USER)
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

class GatewayDialogSetFactory : public resip::AppDialogSetFactory
{
public:
   GatewayDialogSetFactory(Server& server) : mServer(server) {}
   resip::AppDialogSet* createAppDialogSet(resip::DialogUsageManager& dum,
                                           const resip::SipMessage& msg)
   {
      switch(msg.method())
      {
      case INVITE:
         if(msg.exists(h_UserAgent))
         {
            if(msg.header(h_UserAgent).value().prefix("Viceroy"))
            {
               B2BSession* session = mServer.findMatchingIChatB2BSession(msg);
               if(session)
               {
                  InfoLog( << "GatewayDialogSetFactory found existing session for new IChat call, handle=" << session->getB2BSessionHandle());
                  return session;
               }
            }
         }
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

class GatewayLogger : public ExternalLogger
{
public:
   virtual ~GatewayLogger() {}
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
      if(level <= Log::Warning || subsystem.getSubsystem() == AppSubsystem::GATEWAY.getSubsystem())
      {
         resipCout << messageWithHeaders << endl;
      }
      return true;
   }
};
GatewayLogger g_GatewayLogger;

class IPCMutexResip : public IPCMutex
{
public:
   IPCMutexResip() {}
   virtual ~IPCMutexResip() {}
   virtual void lock() { mMutex.lock(); }
   virtual void unlock() { mMutex.unlock(); }
private:
   resip::Mutex mMutex;
};
IPCMutexResip g_IPCMutexResip;

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
   mMediaRelay(0),
   mIPCThread(0)
{
   GenericLogImpl::MaxLineCount = mLogFileMaxLines; 
   Log::initialize("file", mLogLevel, "", mLogFilename.c_str(), (ExternalLogger*)&g_GatewayLogger);
   //UserAgent::setLogLevel(Log::Warning, UserAgent::SubsystemAll);
   //UserAgent::setLogLevel(Log::Info, UserAgent::SubsystemGateway);

   InfoLog( << "ichat-gw settings:");
   InfoLog( << "  Local IP Address = " << mAddress);
   Data dnsServersString;
   for(unsigned int i = 0; i != (unsigned int)mDnsServers.size(); i++)
   {
      if(i!=0) dnsServersString += ", ";
      dnsServersString += Tuple(mDnsServers[i]).presentationFormat();
   }
   InfoLog( << "  Override DNS Servers = " << dnsServersString);
   InfoLog( << "  Gateway Identity = " << mGatewayIdentity);
   InfoLog( << "  SIP Port = " << mSipPort);
   InfoLog( << "  TLS Port = " << mTlsPort);
   InfoLog( << "  TLS Domain = " << mTlsDomain);
   InfoLog( << "  Keepalives = " << (mKeepAlives ? "enabled" : "disabled"));
   InfoLog( << "  Outbound Proxy = " << mOutboundProxy);
   InfoLog( << "  Registration Time = " << mRegistrationTime);
   InfoLog( << "  Registration Retry Time = " << mRegistrationRetryTime);
   InfoLog( << "  Log Level = " << mLogLevel);
   InfoLog( << "  Gateway IPC Port = " << mGatewayIPCPort);
   InfoLog( << "  Jabber Connector IPC Port = " << mJabberConnectorIPCPort);
   InfoLog( << "  HttpPort = " << mHttpPort);
   InfoLog( << "  HttpAuth = " << (mHttpAuth ? "enabled" : "disabled"));
   InfoLog( << "  IChatProceedingTimeout = " << mIChatProceedingTimeout);
   InfoLog( << "  AlwaysRelayIChatMedia = " << (mAlwaysRelayIChatMedia ? "enabled" : "disabled"));
   InfoLog( << "  PreferIPv6 = " << (mPreferIPv6 ? "enabled" : "disabled"));
   Data codecIdFilterListString;
   for(CodecIdList::iterator it = mCodecIdFilterList.begin(); it != mCodecIdFilterList.end(); it++)
   {
      if(it!=mCodecIdFilterList.begin()) codecIdFilterListString += ", ";
      codecIdFilterListString += Data(*it);
   }
   InfoLog( << "  CodecIdFilterList = " << codecIdFilterListString);
   InfoLog( << "  MediaRelayPortRange = " << mMediaRelayPortRangeMin << "-" << mMediaRelayPortRangeMax);
   InfoLog( << "  JabberServer = " << mJabberServer);
   InfoLog( << "  JabberComponentName = " << mJabberComponentName);
   //InfoLog( << "  JabberComponentPassword = " << mJabberComponentPassword);  Don't output password
   InfoLog( << "  JabberComponentPort = " << mJabberComponentPort);
   InfoLog( << "  JabberServerPingDuration = " << mJabberServerPingDuration);
   InfoLog( << "  JabberControlUsername = " << mJabberControlUsername);

   // Install Translation Rules
   TranslationList::iterator itTrans = mAddressTranslations.begin();
   for(;itTrans!=mAddressTranslations.end();itTrans++)
   {
      mAddressTranslator.addTranslation(itTrans->first, itTrans->second);
   }

   // Populate local IP Port Data from external network interfaces
   if(!mAddress.empty())
   {
      // If address is specified in config file, then just use this address only
      Tuple myTuple(mAddress, mSipPort, UDP);
      mLocalIPPortData.addIPPortData(Data("eth0"), myTuple);
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
      unsigned int resNum = 0;
      for(;itInt != interfaces.end(); itInt++)
      {
         Tuple myTuple(itInt->second, mSipPort, UDP);
         if(itInt->second != "127.0.0.1" &&      // not a IPv4 Loopback address
            itInt->second != "::1" &&            // not a IPv6 Loopback address
            !itInt->second.prefix("169.254") &&  // not a IPv4 link local address
            !itInt->second.prefix("fe8") &&      // not a IPv6 link local address
            !itInt->second.prefix("fe9") &&
            !itInt->second.prefix("fea") &&
            !itInt->second.prefix("feb"))  
         {
            InfoLog(<<"adding discovered external interface " << itInt->first << " - " << itInt->second);
            mLocalIPPortData.addIPPortData(Data("eth"+Data(resNum++)), myTuple);
         }
         else
         {
            InfoLog(<<"ignoring discovered loopback/link local interface " << itInt->first << " - " << itInt->second);
         }
         if(myTuple.ipVersion() == V6)
         {
            mIsV6Avail = true;
         }
      }
   }

   if(!mIsV6Avail && mPreferIPv6)
   {
      // If we prefer ipv6 but it is not available, then flip setting
      mPreferIPv6 = false;
   }

   // Create IPCThread
   mIPCThread = new IPCThread(mGatewayIPCPort, mJabberConnectorIPCPort, this, &g_IPCMutexResip);

   // Start the Jabber Connector Process
#ifdef _WIN32
   intptr_t result = _spawnl(_P_NOWAIT, 
                             "ichat-gw-jc.exe", 
                             "ichat-gw",                               // argv[0]   Jabber connector uses this to check if launched from here or not
                             Data(mJabberConnectorIPCPort).c_str(),    // argv[1]   JabberConnector IPC Port
                             Data(mGatewayIPCPort).c_str(),            // argv[2]   Gateway IPC Port
                             mJabberServer.c_str(),                    // argv[3]
                             mJabberComponentName.c_str(),             // argv[4]
                             mJabberComponentPassword.c_str(),         // argv[5]
                             Data(mJabberComponentPort).c_str(),       // argv[6]
                             Data(mJabberServerPingDuration).c_str(),  // argv[7]
                             mJabberControlUsername.c_str(),           // argv[8]
                             mLocalIPPortData.hexBlob().c_str(),       // argv[9]
                             mLogLevel.c_str(),                        // argv[10]
                             NULL);
   InfoLog(<< "JabberConnector spawn returned: " << result);
   if(result == -1)
   {
      exit((int)result);
   }
#else
   pid_t pid;
   const char* args[12];
   Data program("ichat-gw");
   Data jabberConnectorIPCPort(mJabberConnectorIPCPort);
   Data gatewayIPCPort(mGatewayIPCPort);
   Data componentPort(mJabberComponentPort);
   Data pingDuration(mJabberServerPingDuration);
   args[0] = program.c_str();
   args[1] = jabberConnectorIPCPort.c_str();
   args[2] = gatewayIPCPort.c_str();
   args[3] = mJabberServer.c_str();
   args[4] = mJabberComponentName.c_str();
   args[5] = mJabberComponentPassword.c_str();
   args[6] = componentPort.c_str();
   args[7] = pingDuration.c_str();
   args[8] = mJabberControlUsername.c_str();
   args[9] = mLocalIPPortData.hexBlob().c_str();
   args[10] = mLogLevel.c_str();
   args[11] = 0;
   int result = posix_spawn(&pid,
                            "ichat-gw-jc",
                            NULL /* file actions */,
                            NULL /* attr pointer */,
                            (char* const*)args /* argv */,
                            NULL /* envp */);
   InfoLog(<< "JabberConnector spawn returned: " << result);
   if(result != 0)
   {
      exit(result);
   }
#endif

   // Start Media Relay
   mMediaRelay = new MediaRelay(mIsV6Avail, mMediaRelayPortRangeMin, mMediaRelayPortRangeMax);

   // Add transports
   try
   {
      UdpTransport* udpTransport = (UdpTransport*)mStack.addTransport(UDP, mSipPort, DnsUtil::isIpV6Address(mAddress) ? V6 : V4, StunEnabled, mAddress);
      udpTransport->setExternalUnknownDatagramHandler(this);  // Install handler to catch iChat pinhole messages
      mStack.addTransport(TCP, mSipPort, DnsUtil::isIpV6Address(mAddress) ? V6 : V4, StunEnabled, mAddress);
      mStack.addTransport(TLS, mTlsPort, DnsUtil::isIpV6Address(mAddress) ? V6 : V4, StunEnabled, mAddress, mTlsDomain);
      if(mAddress.empty() && mIsV6Avail)
      {
         // if address is empty (ie. all interfaces), then create V6 transports too
         udpTransport = (UdpTransport*)mStack.addTransport(UDP, mSipPort, V6, StunEnabled, mAddress);
         udpTransport->setExternalUnknownDatagramHandler(this);  // Install handler to catch iChat pinhole messages
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

   mProfile->setDefaultFrom(mGatewayIdentity);

   mProfile->setDefaultRegistrationTime(mRegistrationTime);
   mProfile->setDefaultRegistrationRetryTime(mRegistrationRetryTime);

   //profile->setOverrideHostAndPort(mContact);
   if(!mOutboundProxy.uri().host().empty())
   {
      mProfile->setOutboundProxy(mOutboundProxy.uri());
   }

   mProfile->setUserAgent("ichat-gw");

   // Install Handlers
   mDum.setMasterProfile(mProfile);
   mDum.setClientRegistrationHandler(this);
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
   auto_ptr<AppDialogSetFactory> dsf(new GatewayDialogSetFactory(*this));
	mDum.setAppDialogSetFactory(dsf);

#if 0
   // Set UserAgentServerAuthManager
   SharedPtr<ServerAuthManager> uasAuth( new AppServerAuthManager(*this));
   mDum.setServerAuthManager(uasAuth);
#endif
}

Server::~Server()
{
   shutdown();

   delete mMediaRelay;
   delete mIPCThread;
}

void
Server::startup()
{
   mIPCThread->run();
   mStackThread.run(); 
   mMediaRelay->run();
}

void 
Server::process(int timeoutMs)
{
   mDum.process(timeoutMs);
}

void
Server::shutdown()
{
   // Unregister all registrations
   RegistrationMap tempRegs = mRegistrations;  // Create copy for safety, since ending Registrations can immediately remove themselves from map
   RegistrationMap::iterator j;
   for(j = tempRegs.begin(); j != tempRegs.end(); j++)
   {
      j->second->end();
   }

   ShutdownCmd* cmd = new ShutdownCmd(this);
   mDum.post(cmd);

   // Wait for Dum to shutdown
   while(!mDumShutdown) 
   {
      process(100);
   }

   mMediaRelay->shutdown();
   mMediaRelay->join();

   mStackThread.shutdown();
   mStackThread.join();

   mIPCThread->shutdown();
   mIPCThread->join();
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
   case SubsystemGateway:
      Log::setLevel(level, AppSubsystem::GATEWAY);
      break;
   }
}

void 
Server::notifyIChatCallRequest(const std::string& to, const std::string& from)
{
   NotifyIChatCallRequestCmd* cmd = new NotifyIChatCallRequestCmd(*this, to, from);
   mDum.post(cmd);
}

void 
Server::notifyIChatCallRequestImpl(const std::string& to, const std::string& from)
{
   InfoLog(<< "notifyIChatCallRequestImpl: call request from " << from << " to " << to);
   B2BSession* session = new B2BSession(*this, false /* hasDialogSet */);   
   session->initiateIChatCallRequest(to, from);
}

void 
Server::notifyIChatCallCancelled(const B2BSessionHandle& handle)
{
   NotifyIChatCallCancelledCmd* cmd = new NotifyIChatCallCancelledCmd(*this, handle);
   mDum.post(cmd);
}

void 
Server::notifyIChatCallCancelledImpl(const B2BSessionHandle& handle)
{
   B2BSession* session = getB2BSession(handle);
   if(session)
   {
      session->notifyIChatCallCancelled();
   }
}

void 
Server::notifyIChatCallProceeding(const B2BSessionHandle& handle, const std::string& to)
{
   NotifyIChatCallProceedingCmd* cmd = new NotifyIChatCallProceedingCmd(*this, handle, to);
   mDum.post(cmd);
}

void 
Server::notifyIChatCallProceedingImpl(const B2BSessionHandle& handle, const std::string& to)
{
   B2BSession* session = getB2BSession(handle);
   if(session)
   {
      session->notifyIChatCallProceeding(to);
   }
}

void 
Server::notifyIChatCallFailed(const B2BSessionHandle& handle, unsigned int statusCode)
{
   NotifyIChatCallFailedCmd* cmd = new NotifyIChatCallFailedCmd(*this, handle, statusCode);
   mDum.post(cmd);
}

void 
Server::notifyIChatCallFailedImpl(const B2BSessionHandle& handle, unsigned int statusCode)
{
   B2BSession* session = getB2BSession(handle);
   if(session)
   {
      session->notifyIChatCallFailed(statusCode);
   }
}

void 
Server::continueIChatCall(const B2BSessionHandle& handle, const std::string& remoteIPPortListBlob)
{
   ContinueIChatCallCmd* cmd = new ContinueIChatCallCmd(*this, handle, remoteIPPortListBlob);
   mDum.post(cmd);
}

void 
Server::continueIChatCallImpl(const B2BSessionHandle& handle, const std::string& remoteIPPortListBlob)
{
   B2BSession* session = getB2BSession(handle);
   if(session)
   {
      session->continueIChatCall(remoteIPPortListBlob);
   }
}

void 
Server::sipRegisterJabberUser(const std::string& jidToRegister)
{
   if(mRegistrationTime == 0) return;

   SipRegisterJabberUserCmd* cmd = new SipRegisterJabberUserCmd(*this, jidToRegister);
   mDum.post(cmd);
}

void 
Server::sipRegisterJabberUserImpl(const std::string& jidToRegister)
{
   // Transform JID to SIP Uri
   resip::Data sipUriData;
   if(translateAddress(Data("xmpp:") + Data(jidToRegister.c_str()), sipUriData, true /* failIfNoRule */))
   {
      try
      {
         resip::NameAddr sipNameAddr(sipUriData);

         RegistrationMap::iterator it = mRegistrations.find(sipNameAddr.uri());
         if(it == mRegistrations.end())
         {
            SipRegistration *registration = new SipRegistration(*this, mDum, sipNameAddr.uri());

            // Create new UserProfile
            SharedPtr<UserProfile> userProfile(new UserProfile(mProfile));
            userProfile->setDefaultFrom(sipNameAddr);

            mDum.send(mDum.makeRegistration(sipNameAddr, userProfile, registration));
         }
      }
      catch(resip::BaseException& e)
      {
         ErrLog(<< "sipRegisterJabberUserImpl: Translation of " << jidToRegister << " resulted in invalid uri format: " << e);
      }
   }
   else
   {
      WarningLog(<< "sipRegisterJabberUserImpl: No translation of " << jidToRegister << " to a SIP uri format.");
   }
}

void 
Server::sipUnregisterJabberUser(const std::string& jidToUnregister)
{
   if(mRegistrationTime == 0) return;

   SipUnregisterJabberUserCmd* cmd = new SipUnregisterJabberUserCmd(*this, jidToUnregister);
   mDum.post(cmd);
}

void 
Server::sipUnregisterJabberUserImpl(const std::string& jidToUnregister)
{
   // Transform JID to SIP Uri
   resip::Data sipUriData;
   if(translateAddress(Data("xmpp:") + Data(jidToUnregister.c_str()), sipUriData, true /* failIfNoRule */))
   {
      try
      {
         resip::NameAddr sipNameAddr(sipUriData);

         RegistrationMap::iterator it = mRegistrations.find(sipNameAddr.uri());
         if(it != mRegistrations.end())
         {
            it->second->end();
         }
      }
      catch(resip::BaseException& e)
      {
         ErrLog(<< "sipUnregisterJabberUserImpl: Translation of " << jidToUnregister << " resulted in invalid uri format: " << e);
      }
   }
   else
   {
      WarningLog(<< "sipRegisterJabberUserImpl: No translation of " << jidToUnregister << " to a SIP uri format.");
   }
}

void 
Server::checkSubscription(const std::string& to, const std::string& from)
{
   Data temp;
   bool success = mAddressTranslator.translate(Data("xmpp:") + Data(to.c_str()), temp, true /* failIfNoRule */);
   IPCMsg msg;
   msg.addArg("sendSubscriptionResponse");
   msg.addArg(from.c_str());
   msg.addArg(to.c_str());   
   msg.addArg(Data((unsigned long)success).c_str());   
   mIPCThread->sendIPCMsg(msg);
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
Server::registerRegistration(SipRegistration *registration)
{
   mRegistrations[registration->getAor()] = registration;
}

void 
Server::unregisterRegistration(SipRegistration *registration)
{
   mRegistrations.erase(registration->getAor());
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
            InfoLog(<< "Destroying B2BSession: " << it->second->getB2BSessionHandle());
            sessionEnded = true;
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

B2BSession* 
Server::findMatchingIChatB2BSession(const resip::SipMessage& msg)
{
   B2BSessionMap::const_iterator i = mB2BSessions.begin();
   for(; i != mB2BSessions.end(); i++)
   {
      if(i->second->checkIChatCallMatch(msg))
      {
         return i->second;
      }
   }
   return 0;
}

void 
Server::onNewIPCMsg(const IPCMsg& msg)
{
   const std::vector<std::string>& args = msg.getArgs();
   resip_assert(args.size() >= 1);
   if(args.at(0) == "notifyIChatCallRequest")
   {
      InfoLog(<< "Server::onNewIPCMsg - notifyIChatCallRequest");
      resip_assert(args.size() == 3);
      notifyIChatCallRequest(args.at(1).c_str(), args.at(2).c_str());
   }
   else if(args.at(0) == "notifyIChatCallCancelled")
   {
      InfoLog(<< "Server::onNewIPCMsg - notifyIChatCallCancelled");
      resip_assert(args.size() == 2);
      notifyIChatCallCancelled(atoi(args.at(1).c_str()));
   }
   else if(args.at(0) == "notifyIChatCallProceeding")
   {
      InfoLog(<< "Server::onNewIPCMsg - notifyIChatCallProceeding");
      resip_assert(args.size() == 3);
      notifyIChatCallProceeding(atoi(args.at(1).c_str()), args.at(2).c_str());
   }
   else if(args.at(0) == "notifyIChatCallFailed")
   {
      InfoLog(<< "Server::onNewIPCMsg - notifyIChatCallFailed");      
      resip_assert(args.size() == 3);
      notifyIChatCallFailed(atoi(args.at(1).c_str()), atoi(args.at(2).c_str()));
   }
   else if(args.at(0) == "continueIChatCall")
   {
      InfoLog(<< "Server::onNewIPCMsg - continueIChatCall");      
      resip_assert(args.size() == 3);
      continueIChatCall(atoi(args.at(1).c_str()), args.at(2).c_str());
   }
   else if(args.at(0) == "sipRegisterJabberUser")
   {
      InfoLog(<< "Server::onNewIPCMsg - sipRegisterJabberUser");      
      resip_assert(args.size() == 2);
      sipRegisterJabberUser(args.at(1).c_str());
   }
   else if(args.at(0) == "sipUnregisterJabberUser")
   {
      InfoLog(<< "Server::onNewIPCMsg - sipUnregisterJabberUser");      
      resip_assert(args.size() == 2);
      sipUnregisterJabberUser(args.at(1).c_str());
   }
   else if(args.at(0) == "checkSubscription")
   {
      InfoLog(<< "Server::onNewIPCMsg - checkSubscription");      
      resip_assert(args.size() == 3);
      checkSubscription(args.at(1).c_str(), args.at(2).c_str());
   }
   else if(args.at(0) == "log")
   {
      resip_assert(args.size() == 3);
      if(args.at(1) == "warning")
      {
         WarningLog(<< args.at(2));
      }
      else if(args.at(1) == "error")
      {
         ErrLog(<< args.at(2));
      }
      else
      {
         InfoLog(<< args.at(2));
      }
   }
   else
   {
      resip_assert(false);
   }
}

void 
Server::operator()(UdpTransport* transport, const Tuple& source, std::auto_ptr<resip::Data> unknownPacket)
{
   InfoLog(<< "Received an unknown packet of size=" << unknownPacket->size() << ", from " << source);
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
         WarningLog (<< "Received ood refer w/out a Target-Dialog: " << msg.brief());
         ss->send(ss->reject(400));
      }
      else
      {
         WarningLog (<< "Received refer w/out a Refer-To: " << msg.brief());
         ss->send(ss->reject(400));
      }
   }
   catch(BaseException &e)
   {
      WarningLog(<< "onNewSubscriptionFromRefer exception: " << e);
   }
   catch(...)
   {
      WarningLog(<< "onNewSubscriptionFromRefer unknown exception");
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
         WarningLog(<< "onReceivedRequest(ServerOutOfDialogReqHandle): unknown exception");
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

////////////////////////////////////////////////////////////////////////////////
// Registration Handler ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
Server::onSuccess(ClientRegistrationHandle h, const SipMessage& msg)
{
   dynamic_cast<SipRegistration *>(h->getAppDialogSet().get())->onSuccess(h, msg);
}

void
Server::onFailure(ClientRegistrationHandle h, const SipMessage& msg)
{
   dynamic_cast<SipRegistration *>(h->getAppDialogSet().get())->onFailure(h, msg);
}

void
Server::onRemoved(ClientRegistrationHandle h, const SipMessage&msg)
{
   dynamic_cast<SipRegistration *>(h->getAppDialogSet().get())->onRemoved(h, msg);
}

int 
Server::onRequestRetry(ClientRegistrationHandle h, int retryMinimum, const SipMessage& msg)
{
   return dynamic_cast<SipRegistration *>(h->getAppDialogSet().get())->onRequestRetry(h, retryMinimum, msg);
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

