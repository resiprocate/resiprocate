#include <signal.h>
#include "resip/stack/MessageFilterRule.hxx"
#include "resip/stack/Compression.hxx"
#include "resip/stack/ExtensionParameter.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/InterruptableStackThread.hxx"
#include "resip/stack/EventStackThread.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "resip/stack/ConnectionManager.hxx"
#include "resip/dum/DumThread.hxx"
#include "resip/dum/InMemoryRegistrationDatabase.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/GeneralCongestionManager.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/FdPoll.hxx"

#include "repro/ProxyConfig.hxx"
#include "repro/Proxy.hxx"
#include "repro/Registrar.hxx"
#include "repro/ReproServerAuthManager.hxx"
#include "repro/ReproServerAuthManager.hxx"
#include "repro/ProcessorChain.hxx"
#include "repro/Store.hxx"
#include "repro/UserStore.hxx"
#include "repro/RouteStore.hxx"
#include "repro/AbstractDb.hxx"
#include "repro/BerkeleyDb.hxx"
#include "repro/WebAdmin.hxx"
#include "repro/WebAdminThread.hxx"
#include "repro/CommandServer.hxx"
#include "repro/CommandServerThread.hxx"
#include "repro/RegSyncClient.hxx"
#include "repro/RegSyncServer.hxx"
#include "repro/RegSyncServerThread.hxx"
#include "repro/monkeys/IsTrustedNode.hxx"
#include "repro/monkeys/AmIResponsible.hxx"
#include "repro/monkeys/ConstantLocationMonkey.hxx"
#include "repro/monkeys/DigestAuthenticator.hxx"
#include "repro/monkeys/LocationServer.hxx"
#include "repro/monkeys/RecursiveRedirect.hxx"
#include "repro/monkeys/SimpleStaticRoute.hxx"
#include "repro/monkeys/StaticRoute.hxx"
#include "repro/monkeys/StrictRouteFixup.hxx"
#include "repro/monkeys/OutboundTargetHandler.hxx"
#include "repro/monkeys/QValueTargetHandler.hxx"
#include "repro/monkeys/SimpleTargetHandler.hxx"

#if defined(USE_SSL)
#include "resip/stack/ssl/Security.hxx"
#include "repro/stateAgents/CertServer.hxx"
#endif

#if defined(USE_MYSQL)
#include "repro/MySqlDb.hxx"
#endif
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

using namespace repro;
using namespace resip;
using namespace std;

static bool finished = false;

static void
signalHandler(int signo)
{
   std::cerr << "Shutting down" << endl;
   finished = true;
}

class ReproLogger : public ExternalLogger
{
public:
   virtual ~ReproLogger() {}
   /** return true to also do default logging, false to supress default logging. */
   virtual bool operator()(Log::Level level,
                           const Subsystem& subsystem, 
                           const Data& appName,
                           const char* file,
                           int line,
                           const Data& message,
                           const Data& messageWithHeaders)
   {
      // Log any errors to the screen 
      if(level <= Log::Err)
      {
         resipCout << messageWithHeaders << endl;
      }
      return true;
   }
};
ReproLogger g_ReproLogger;

Data
addDomains(TransactionUser& tu, ProxyConfig& config, bool log)
{
   Data realm;
   
   std::vector<Data> configDomains;
   if(config.getConfigValue("Domains", configDomains))
   {
      for (std::vector<Data>::const_iterator i=configDomains.begin(); 
         i != configDomains.end(); ++i)
      {
         if(log) InfoLog (<< "Adding domain " << *i << " from command line");
         tu.addDomain(*i);
         if ( realm.empty() )
         {
            realm = *i;
         }
      }
   }

   const ConfigStore::ConfigData& dList = config.getDataStore()->mConfigStore.getConfigs();
   for (  ConfigStore::ConfigData::const_iterator i=dList.begin(); 
           i != dList.end(); ++i)
   {
      if(log) InfoLog (<< "Adding domain " << i->second.mDomain << " from config");
      tu.addDomain( i->second.mDomain );
      if ( realm.empty() )
      {
         realm = i->second.mDomain;
      }
   }

   Data localhostname(DnsUtil::getLocalHostName());
   if(log) InfoLog (<< "Adding local hostname domain " << localhostname );
   tu.addDomain(localhostname);
   if ( realm.empty() )
   {
      realm = localhostname;
   }

   if(log) InfoLog (<< "Adding localhost domain.");
   tu.addDomain("localhost");
   if ( realm.empty() )
   {
      realm = "localhost";
   }
   
   list<pair<Data,Data> > ips = DnsUtil::getInterfaces();
   for ( list<pair<Data,Data> >::const_iterator i=ips.begin(); i!=ips.end(); i++)
   {
      if(log) InfoLog( << "Adding domain for IP " << i->second << " from interface " << i->first  );
      tu.addDomain(i->second);
   }

   if(log) InfoLog (<< "Adding 127.0.0.1 domain.");
   tu.addDomain("127.0.0.1");

   return realm;
}



int
main(int argc, char** argv)
{
#ifndef _WIN32
   if ( signal( SIGPIPE, SIG_IGN) == SIG_ERR)
   {
      cerr << "Couldn't install signal handler for SIGPIPE" << endl;
      exit(-1);
   }
#endif

   if ( signal( SIGINT, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGINT" << endl;
      exit( -1 );
   }

   if ( signal( SIGTERM, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGTERM" << endl;
      exit( -1 );
   }

   /* Initialize a stack */
   initNetwork();

   Data configFilename("repro.config");
   ProxyConfig config(argc, argv, configFilename);

   GenericLogImpl::MaxByteCount = config.getConfigUnsignedLong("LogFileMaxBytes", 5242880 /*5 Mb */);
   Data loggingType = config.getConfigData("LoggingType", "cout", true);
   Log::initialize(loggingType, 
                   config.getConfigData("LogLevel", "INFO", true), 
                   argv[0], 
                   config.getConfigData("LogFilename", "repro.log", true).c_str(),
                   isEqualNoCase(loggingType, "file") ? &g_ReproLogger : 0); // if logging to file then write WARNINGS, and Errors to console still

   unsigned long overrideT1 = config.getConfigInt("TimerT1", 0);
   if(overrideT1)
   {
      WarningLog(<< "Overriding T1! (new value is " << 
               overrideT1 << ")");
      resip::Timer::resetT1(overrideT1);
   }

#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   { FindMemoryLeaks fml;
#endif

   Security* security = 0;
   Compression* compression = 0;

#ifdef USE_SSL
#ifdef WIN32
   Data certPath("C:\\sipCerts");
#else 
   Data certPath(getenv("HOME"));
   certPath += "/.sipCerts";
#endif
   config.getConfigValue("CertificatePath", certPath);
   security = new Security(certPath);
#endif

#ifdef USE_SIGCOMP
   compression = new Compression(Compression::DEFLATE);
#endif

   std::auto_ptr<FdPollGrp> pollGrp(NULL);
   std::auto_ptr<SelectInterruptor> threadInterruptor(NULL);
   pollGrp.reset(FdPollGrp::create());
   threadInterruptor.reset(new EventThreadInterruptor(*pollGrp));
   //threadInterruptor.reset(new SelectInterruptor());

   bool useV4 = !config.getConfigBool("DisableIPv4", false);
#ifdef USE_IPV6
   bool useV6 = config.getConfigBool("EnableIPv6", true);
#else
   bool useV6 = false;
#endif
   if (useV4) InfoLog (<< "V4 enabled");
   if (useV6) InfoLog (<< "V6 enabled");

   // Build DNS Server list from config
   DnsStub::NameserverList dnsServers;
   std::vector<resip::Data> dnsServersConfig;
   config.getConfigValue("DNSServers", dnsServersConfig);
   for(std::vector<resip::Data>::iterator it = dnsServersConfig.begin(); it != dnsServersConfig.end(); it++)
   {
      if((useV4 && DnsUtil::isIpV4Address(*it)) || (useV6 && DnsUtil::isIpV6Address(*it)))
      {
         InfoLog(<< "Using DNS Server from config: " << *it);
         dnsServers.push_back(Tuple(*it, 0, UNKNOWN_TRANSPORT).toGenericIPAddress());
      }
   }

   SipStack stack(security,
                  dnsServers,
                  threadInterruptor.get(),
                  /*stateless*/false,
                  /*socketFunc*/0,
                  compression,
                  pollGrp.get());

   std::vector<Data> enumSuffixes;
   config.getConfigValue("EnumSuffixes", enumSuffixes);
   if (enumSuffixes.size() > 0)
   {
      stack.setEnumSuffixes(enumSuffixes);
   }

   bool allTransportsSpecifyRecordRoute=false;
   try
   {
      // Check if advanced transport settings are provided
      unsigned int transportNum = 1;
      Data settingKeyBase("Transport" + Data(transportNum));
      Data interfaceSettingKey(settingKeyBase + "Interface");
      Data interfaceSettings = config.getConfigData(interfaceSettingKey, "", true);
      if(!interfaceSettings.empty())
      {
         // Sample config file format for advanced transport settings
         // Transport1Interface = 192.168.1.106:5061
         // Transport1Type = TLS
         // Transprot1TlsDomain = sipdomain.com
         // Transport1RecordRouteUri = sip:sipdomain.com;transport=TLS
         // Transport1RcvBufLen = 2000

         allTransportsSpecifyRecordRoute = true;

         const char *anchor;
         while(!interfaceSettings.empty())
         {
            Data typeSettingKey(settingKeyBase + "Type");
            Data tlsDomainSettingKey(settingKeyBase + "TlsDomain");
            Data recordRouteUriSettingKey(settingKeyBase + "RecordRouteUri");
            Data rcvBufSettingKey(settingKeyBase + "RcvBufLen");

            // Parse out interface settings
            ParseBuffer pb(interfaceSettings);
            anchor = pb.position();
            pb.skipToChar(':');
            if(!pb.eof())
            {
               Data ipAddr;
               Data portData;
               pb.data(ipAddr, anchor);
               pb.skipChar();
               anchor = pb.position();
               pb.skipToEnd();
               pb.data(portData, anchor);
               if(!DnsUtil::isIpAddress(ipAddr))
               {
                  CritLog(<< "Malformed IP-address found in " << interfaceSettingKey << " setting: " << ipAddr);
               }
               int port = portData.convertInt();
               if(port == 0)
               {
                  CritLog(<< "Invalid port found in " << interfaceSettingKey << " setting: " << port);
               }
               TransportType tt = Tuple::toTransport(config.getConfigData(typeSettingKey, "UDP"));
               if(tt == UNKNOWN_TRANSPORT)
               {
                  CritLog(<< "Unknown transport type found in " << typeSettingKey << " setting: " << config.getConfigData(typeSettingKey, "UDP"));
               }
               Data tlsDomain = config.getConfigData(tlsDomainSettingKey, "");
               int rcvBufLen = config.getConfigInt(rcvBufSettingKey, 0);
               Transport *t = stack.addTransport(tt,
                                 port,
                                 DnsUtil::isIpV6Address(ipAddr) ? V6 : V4,
                                 StunEnabled, 
                                 ipAddr, // interface to bind to
                                 tlsDomain);

               if (t && rcvBufLen>0 )
               {
#if defined(RESIP_SIPSTACK_HAVE_FDPOLL)
                  // this new method is part of the epoll changeset,
                  // which isn't commited yet.
                  t->setRcvBufLen(rcvBufLen);
#else
                   assert(0);
#endif
               }

               Data recordRouteUri = config.getConfigData(recordRouteUriSettingKey, "");
               if(!recordRouteUri.empty())
               {
                  try
                  {
                     if(isEqualNoCase(recordRouteUri, "auto")) // auto generated record route uri
                     {
                        if(tt == TLS || tt == DTLS)
                        {
                           NameAddr rr;
                           rr.uri().host()=tlsDomain;
                           rr.uri().port()=port;
                           rr.uri().param(resip::p_transport)=resip::Tuple::toDataLower(tt);
                           t->setRecordRoute(rr);
                           InfoLog (<< "Transport specific record-route enabled (generated): " << rr);
                        }
                        else
                        {
                           NameAddr rr;
                           rr.uri().host()=ipAddr;
                           rr.uri().port()=port;
                           rr.uri().param(resip::p_transport)=resip::Tuple::toDataLower(tt);
                           t->setRecordRoute(rr);
                           InfoLog (<< "Transport specific record-route enabled (generated): " << rr);
                        }
                     }
                     else
                     {
                        NameAddr rr(recordRouteUri);
                        t->setRecordRoute(rr);
                        InfoLog (<< "Transport specific record-route enabled: " << rr);
                     }
                  }
                  catch(BaseException& e)
                  {
                     ErrLog (<< "Invalid uri provided in " << recordRouteUriSettingKey << " setting (ignoring): " << e);
                     allTransportsSpecifyRecordRoute = false;
                  }
               }
               else 
               {
                  allTransportsSpecifyRecordRoute = false;
               }
            }
            else
            {
               CritLog(<< "Port not specified in " << interfaceSettingKey << " setting: expected format is <IPAddress>:<Port>");
               exit(-1);
            }

            // Check if there is another transport
            transportNum++;
            settingKeyBase = Data("Transport" + Data(transportNum));
            interfaceSettingKey = Data(settingKeyBase + "Interface");
            interfaceSettings = config.getConfigData(interfaceSettingKey, "", true);
         }
      }
      else
      {
         int udpPort = config.getConfigInt("UDPPort", 5060);
         int tcpPort = config.getConfigInt("TCPPort", 5060);
         int tlsPort = config.getConfigInt("TLSPort", 5061);
         int dtlsPort = config.getConfigInt("DTLSPort", 0);
         Data tlsDomain = config.getConfigData("TLSDomainName", "");

         if (udpPort)
         {
            if (useV4) stack.addTransport(UDP, udpPort, V4, StunEnabled);
            if (useV6) stack.addTransport(UDP, udpPort, V6, StunEnabled);
         }
         if (tcpPort)
         {
            if (useV4) stack.addTransport(TCP, tcpPort, V4, StunEnabled);
            if (useV6) stack.addTransport(TCP, tcpPort, V6, StunEnabled);
         }
         if (tlsPort)
         {
            if (useV4) stack.addTransport(TLS, tlsPort, V4, StunEnabled, Data::Empty, tlsDomain);
            if (useV6) stack.addTransport(TLS, tlsPort, V6, StunEnabled, Data::Empty, tlsDomain);
         }
         if (dtlsPort)
         {
            if (useV4) stack.addTransport(DTLS, dtlsPort, V4, StunEnabled, Data::Empty, tlsDomain);
            if (useV6) stack.addTransport(DTLS, dtlsPort, V6, StunEnabled, Data::Empty, tlsDomain);
         }
      }
   }
   catch (BaseException& e)
   {
      std::cerr << "Likely a port is already in use" << endl;
      InfoLog (<< "Caught: " << e);
      exit(-1);
   }
   
   InteropHelper::setOutboundVersion(config.getConfigInt("OutboundVersion", 5626));
   InteropHelper::setOutboundSupported(config.getConfigBool("DisableOutbound", false) ? false : true);
   InteropHelper::setRRTokenHackEnabled(config.getConfigBool("EnableFlowTokens", false));
   Data clientNATDetectionMode = config.getConfigData("ClientNatDetectionMode", "DISABLED");
   if(isEqualNoCase(clientNATDetectionMode, "ENABLED"))
   {
      InteropHelper::setClientNATDetectionMode(InteropHelper::ClientNATDetectionEnabled);
   }
   else if(isEqualNoCase(clientNATDetectionMode, "PRIVATE_TO_PUBLIC"))
   {
      InteropHelper::setClientNATDetectionMode(InteropHelper::ClientNATDetectionPrivateToPublicOnly);
   }
   unsigned long outboundFlowTimer = config.getConfigUnsignedLong("FlowTimer", 0);
   if(outboundFlowTimer > 0)
   {
      InteropHelper::setFlowTimerSeconds(outboundFlowTimer);
      ConnectionManager::MinimumGcAge = 7200000; // Timeout connections not related to a flow timer after 2 hours - TODO make configurable
      ConnectionManager::EnableAgressiveGc = true;
   }

   bool assumePath = config.getConfigBool("AssumePath", false);
   bool forceRecordRoute = config.getConfigBool("ForceRecordRouting", false);
   Uri recordRouteUri;
   config.getConfigValue("RecordRouteUri", recordRouteUri);
   if((InteropHelper::getOutboundSupported() 
         || InteropHelper::getRRTokenHackEnabled()
         || InteropHelper::getClientNATDetectionMode() != InteropHelper::ClientNATDetectionDisabled
         || assumePath
         || forceRecordRoute
      )
      && !(allTransportsSpecifyRecordRoute || !recordRouteUri.host().empty()))
   {
      CritLog(<< "In order for outbound support, the Record-Route flow-token"
      " hack, or force-record-route to work, you MUST specify a Record-Route URI. Launching "
      "without...");
      InteropHelper::setOutboundSupported(false);
      InteropHelper::setRRTokenHackEnabled(false);
      InteropHelper::setClientNATDetectionMode(InteropHelper::ClientNATDetectionDisabled);
      assumePath = false;
      forceRecordRoute=false;
   }

   std::auto_ptr<ThreadIf> stackThread(NULL);
   stackThread.reset(new EventStackThread(stack,
            *dynamic_cast<EventThreadInterruptor*>(threadInterruptor.get()),
            *pollGrp));

   Registrar registrar;
   // We only need removed records to linger if we have reg sync enabled
   int regSyncPort = config.getConfigInt("RegSyncPort", 0);
   InMemorySyncRegDb regData(regSyncPort ? 86400 /* 24 hours */ : 0 /* removeLingerSecs */);  // !slg! could make linger time a setting
   SharedPtr<MasterProfile> profile(new MasterProfile);

   AbstractDb* db=NULL;
#ifdef USE_MYSQL
   Data mySQLServer;
   config.getConfigValue("MySQLServer", mySQLServer);
   if ( !mySQLServer.empty() )
   {
      db = new MySqlDb(mySQLServer);
   }
#endif
   if (!db)
   {
      db = new BerkeleyDb(config.getConfigData("DatabasePath", "./", true));
      if (!static_cast<BerkeleyDb*>(db)->isSane())
      {
        CritLog(<<"Failed to open configuration database");
        exit(-1);
      }
   }
   assert( db );
   config.createDataStore(db);

   /* Initialize a proxy */
   
   /* Explanation:  "Monkeys" are processors which operate on incoming requests
                    "Lemurs"  are processors which operate on incoming responses
                    "Baboons" are processors which operate on a request for each target  
                              as the request is about to be forwarded to that target */
      
   // Build Monkey Chain
   ProcessorChain requestProcessors(Processor::REQUEST_CHAIN);   // Monkeys
#if 0
   if (args.mRequestProcessorChainName=="StaticTest")
   {
      // Add contact location monkey
      requestProcessors.addProcessor(std::auto_ptr<Processor>(new ContactLocationMonkey));
   }
   else
#endif
   {
      // Either the chainName is default or we don't know about it
      // Use default if we don't recognize the name
      // Should log about it.

      // Add strict route fixup monkey
      requestProcessors.addProcessor(std::auto_ptr<Processor>(new StrictRouteFixup));      

      // Add is trusted node monkey
      requestProcessors.addProcessor(std::auto_ptr<Processor>(new IsTrustedNode(config)));
      if (!config.getConfigBool("DisableAuth", false))
      {
         DigestAuthenticator* da = new DigestAuthenticator(config, &stack);

         // Add digest authenticator monkey
         requestProcessors.addProcessor(std::auto_ptr<Processor>(da)); 
      }

      // Add am I responsible monkey
      requestProcessors.addProcessor(std::auto_ptr<Processor>(new AmIResponsible)); 
      
      // [TODO] support for GRUU is on roadmap.  When it is added the GruuMonkey will go here
      
      // [TODO] support for Manipulating Tel URIs is on the roadmap.
      //        When added, the telUriMonkey will go here 
     
      std::vector<Data> routeSet;
      config.getConfigValue("Routes", routeSet);
      if (routeSet.empty())
      {
         // add static route monkey
         requestProcessors.addProcessor(std::auto_ptr<Processor>(new StaticRoute(config))); 
      }
      else
      {
         // add simple static route monkey
         requestProcessors.addProcessor(std::auto_ptr<Processor>(new SimpleStaticRoute(config))); 
      }
      
      // Add location server monkey
      requestProcessors.addProcessor(std::auto_ptr<Processor>(new LocationServer(config, regData)));
   }

   // Build Lemur Chain
   ProcessorChain responseProcessors(Processor::RESPONSE_CHAIN); // Lemurs

   // Add outbound target handler lemur
   responseProcessors.addProcessor(std::auto_ptr<Processor>(new OutboundTargetHandler(regData))); 

   if (config.getConfigBool("RecursiveRedirect", false))
   {
      // Add recursive redirect lemur
      responseProcessors.addProcessor(std::auto_ptr<Processor>(new RecursiveRedirect)); 
   }

   // Build Baboons Chain
   ProcessorChain targetProcessors(Processor::TARGET_CHAIN);     // Baboons   
   if(config.getConfigBool("QValue", false))
   {
      // Add q value target handler baboon
      targetProcessors.addProcessor(std::auto_ptr<Processor>(new QValueTargetHandler(config))); 
   }
   
   // Add simple target handler baboon
   targetProcessors.addProcessor(std::auto_ptr<Processor>(new SimpleTargetHandler)); 

   // Create main Proxy class
   Proxy proxy(stack, 
               config, 
               requestProcessors, 
               responseProcessors, 
               targetProcessors);
   Data realm = addDomains(proxy, config, true);
   
   WebAdmin *admin = NULL;
   WebAdminThread *adminThread = NULL;
   int httpPort = config.getConfigInt("HttpPort", 5080);
   if (httpPort) 
   {
      admin = new WebAdmin(*config.getDataStore(), 
                           regData, 
#ifdef USE_SSL
                           security, 
#else
                           0, 
#endif
                           config.getConfigBool("DisableHttpAuth", false), 
                           realm, 
                           config.getConfigData("HttpAdminPassword", "admin"), 
                           httpPort);
      if (!admin->isSane())
      {
         CritLog(<<"Failed to start the WebAdmin - exiting");
         exit(-1);
      }
      adminThread = new WebAdminThread(*admin);
   }

   profile->clearSupportedMethods();
   profile->addSupportedMethod(resip::REGISTER);
#ifdef USE_SSL
   profile->addSupportedScheme(Symbols::Sips);
#endif
   if(InteropHelper::getOutboundSupported())
   {
      profile->addSupportedOptionTag(Token(Symbols::Outbound));
   }
   profile->addSupportedOptionTag(Token(Symbols::Path));
   if(config.getConfigBool("AllowBadReg", false))
   {
       profile->allowBadRegistrationEnabled() = true;
   }
   
   DialogUsageManager* dum = 0;
   DumThread* dumThread = 0;

   resip::MessageFilterRuleList ruleList;
   bool registrarEnabled = !config.getConfigBool("DisableRegistrar", false);
   bool certServerEnabled = config.getConfigBool("EnableCertServer", false);
   if (registrarEnabled || certServerEnabled)
   {
      dum = new DialogUsageManager(stack);
      dum->setMasterProfile(profile);
      addDomains(*dum, config, false /* log? already logged when adding to Proxy - no need to log again*/);
   }

   if (registrarEnabled)
   {   
      assert(dum);
      dum->setServerRegistrationHandler(&registrar);
      dum->setRegistrationPersistenceManager(&regData);

      // Install rules so that the registrar only gets REGISTERs
      resip::MessageFilterRule::MethodList methodList;
      methodList.push_back(resip::REGISTER);
      ruleList.push_back(MessageFilterRule(resip::MessageFilterRule::SchemeList(),
                                           resip::MessageFilterRule::DomainIsMe,
                                           methodList) );
   }
   
#if defined(USE_SSL)
   CertServer* certServer = 0;
#endif
   if (certServerEnabled)
   {
#if defined(USE_SSL)
      certServer = new CertServer(*dum);

      // Install rules so that the cert server receives SUBSCRIBEs and PUBLISHs
      resip::MessageFilterRule::MethodList methodList;
      resip::MessageFilterRule::EventList eventList;
      methodList.push_back(resip::SUBSCRIBE);
      methodList.push_back(resip::PUBLISH);
      eventList.push_back(resip::Symbols::Credential);
      eventList.push_back(resip::Symbols::Certificate);
      ruleList.push_back(MessageFilterRule(resip::MessageFilterRule::SchemeList(),
                                           resip::MessageFilterRule::DomainIsMe,
                                           methodList,
                                           eventList));
#endif
   }

   if (dum)
   {
      if (!config.getConfigBool("DisableAuth", false))
      {
         SharedPtr<ServerAuthManager> 
            uasAuth( new ReproServerAuthManager(*dum,
                                                config.getDataStore()->mUserStore,
                                                config.getDataStore()->mAclStore,
                                                !config.getConfigBool("DisableAuthInt", false) /*useAuthInt*/,
                                                config.getConfigBool("RejectBadNonces", false)));
         dum->setServerAuthManager(uasAuth);
      }
      dum->setMessageFilterRuleList(ruleList);
      dumThread = new DumThread(*dum);
   }

   stack.registerTransactionUser(proxy);
   // !bwc! If, in the future, we do anything client-side using DUM with this 
   // stack, we'll need to rework things (maybe use two stacks).
   stack.setFixBadDialogIdentifiers(false);
   stack.setFixBadCSeqNumbers(false);

   // Create reg sync components if required
   RegSyncClient* regSyncClient = 0;
   RegSyncServer* regSyncServerV4 = 0;
   RegSyncServer* regSyncServerV6 = 0;
   RegSyncServerThread* regSyncServerThread = 0;
   if(regSyncPort != 0)
   {
      std::list<RegSyncServer*> regSyncServerList;
      if(useV4) 
      {
         regSyncServerV4 = new RegSyncServer(&regData, regSyncPort, V4);
         regSyncServerList.push_back(regSyncServerV4);
      }
      if(useV6) 
      {
         regSyncServerV6 = new RegSyncServer(&regData, regSyncPort, V6);
          regSyncServerList.push_back(regSyncServerV6);
      }
      if(!regSyncServerList.empty())
      {
         regSyncServerThread = new RegSyncServerThread(regSyncServerList);
      }
      Data regSyncPeerAddress(config.getConfigData("RegSyncPeer", ""));
      if(!regSyncPeerAddress.empty())
      {
         regSyncClient = new RegSyncClient(&regData, regSyncPeerAddress, regSyncPort);
      }
   }

   // Create command server if required
   CommandServer* commandServerV4 = 0;
   CommandServer* commandServerV6 = 0;
   CommandServerThread* commandServerThread = 0;
   int commandPort = config.getConfigInt("CommandPort", 5081);
   if(commandPort != 0)
   {
      std::list<CommandServer*> commandServerList;
      if(useV4) 
      {
         commandServerV4 = new CommandServer(stack, commandPort, V4);
         commandServerList.push_back(commandServerV4);
      }
      if(useV6) 
      {
         commandServerV6 = new CommandServer(stack, commandPort, V6);
         commandServerList.push_back(commandServerV6);
      }
      if(!commandServerList.empty())
      {
         commandServerThread = new CommandServerThread(commandServerList);
      }
   }

   std::auto_ptr<CongestionManager> congestionManager;
   if(config.getConfigBool("CongestionManagement", true))
   {
      Data metricData = config.getConfigData("CongestionManagementMetric", "WAIT_TIME", true);
      GeneralCongestionManager::MetricType metric = GeneralCongestionManager::WAIT_TIME;
      if(isEqualNoCase(metricData, "TIME_DEPTH"))
      {
         metric = GeneralCongestionManager::TIME_DEPTH;
      }
      else if(isEqualNoCase(metricData, "SIZE"))
      {
         metric = GeneralCongestionManager::SIZE;
      }
      else if(!isEqualNoCase(metricData, "TIME_DEPTH"))
      {
         WarningLog( << "CongestionManagementMetric specified as an unknown value (" << metricData << "), defaulting to TIME_DEPTH.");
      }
      congestionManager.reset(new GeneralCongestionManager(
                                          metric, 
                                          config.getConfigUnsignedLong("CongestionManagementTolerance", 200)));
      stack.setCongestionManager(congestionManager.get());
   }

   /* Make it all go */
   bool threadedStack = config.getConfigBool("ThreadedStack", true);
   if(threadedStack)
   {
      stack.run();
   }
   stackThread->run();
   proxy.run();
   if(adminThread)
   {
      adminThread->run();
   }
   if(dumThread)
   {
      dumThread->run();
   }
   if(commandServerThread)
   {
      commandServerThread->run();
   }
   if(regSyncServerThread)
   {
      regSyncServerThread->run();
   }
   if(regSyncClient)
   {
      regSyncClient->run();
   }
   
   while (!finished)
   {
#ifdef WIN32
   Sleep(1000);
#else
   usleep(100000);
#endif
   }

   proxy.shutdown();
   stackThread->shutdown();
   if ( adminThread )
   {
       adminThread->shutdown();
   }
   if (dumThread)
   {
       dumThread->shutdown();
   }
   if(commandServerThread)
   {
      commandServerThread->shutdown();
   }
   if(regSyncServerThread)
   {
      regSyncServerThread->shutdown();
   }
   if(regSyncClient)
   {
      regSyncClient->shutdown();
   }
   if(threadedStack)
   {
      stack.shutdownAndJoinThreads();
   }

   proxy.join();
   stackThread->join();
   if (adminThread) 
   {
      adminThread->join();
      delete adminThread; 
      adminThread = NULL;
   }
   if (admin) 
   {
      delete admin; 
      admin = NULL;
   }
   if (dumThread)
   {
      dumThread->join();
      delete dumThread;
   }
   if(commandServerThread)
   {
      commandServerThread->join();
      delete commandServerThread;
   }
   if(regSyncServerThread)
   {
      regSyncServerThread->join();
      delete regSyncServerThread;
   }
   if(regSyncClient)
   {
      regSyncClient->join();
      delete regSyncClient;
   }

   stack.setCongestionManager(0);

   if(commandServerV4)
   {
      delete commandServerV4;
   }
   if(commandServerV6)
   {
      delete commandServerV6;
   }

   if(regSyncServerV4)
   {
      delete regSyncServerV4;
   }
   if(regSyncServerV6)
   {
      delete regSyncServerV6;
   }

#if defined(USE_SSL)
   if(certServer)
   {
       delete certServer;
   }
#endif

   if(dum) 
   {
       delete dum;
   }

   delete db; db=0;
#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   }
#endif
}

/*
 * vi: set shiftwidth=3 expandtab:
 */
