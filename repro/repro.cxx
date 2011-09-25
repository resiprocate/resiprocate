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
   Log::initialize(config.getConfigData("LoggingType", "cout", true), 
                   config.getConfigData("LogLevel", "INFO", true), 
                   argv[0], 
                   config.getConfigData("LogFilename", "repro.log", true).c_str());

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

#if defined(HAVE_EPOLL)
   if(config.getConfigBool("InternalEpoll", true)) 
   {
#if defined(RESIP_SIPSTACK_HAVE_FDPOLL)
      SipStack::setDefaultUseInternalPoll(true);
#else
      cerr << "Poll not supported by SipStack" << endl;
      exit(1);
#endif
   }
#endif

   std::auto_ptr<FdPollGrp> pollGrp(NULL);
   std::auto_ptr<SelectInterruptor> threadInterruptor(NULL);
#if defined(HAVE_EPOLL)
   if (config.getConfigBool("EventThread", true))
   {
      pollGrp.reset(FdPollGrp::create());
      threadInterruptor.reset(new EventThreadInterruptor(*pollGrp));
   }
   else
#endif
   threadInterruptor.reset(new SelectInterruptor());

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
      // An example of how to use this follows. This sets up 2 transports, 1 TLS
      // and 1 UDP. TLS domain is bound to example.com. 
      // repro -i "sip:192.168.1.200:5060;transport=tls;tls=example.com,sip:192.168.1.200:5060;transport=udp"
      // Note: If you specify interfaces the other transport arguments have no effect
      std::vector<Data> interfaces;
      config.getConfigValue("Interfaces", interfaces);
      if (!interfaces.empty())
      {
         allTransportsSpecifyRecordRoute = true;
         for (std::vector<Data>::iterator i=interfaces.begin(); 
              i != interfaces.end(); ++i)
         {
            try
            {
               Uri intf(*i);
               if(!DnsUtil::isIpAddress(intf.host()))
               {
                  CritLog(<< "Malformed IP-address found in the --interfaces (-i) command-line option: " << intf.host());
               }
               TransportType tt = Tuple::toTransport(intf.param(p_transport));
               ExtensionParameter p_rcvbuf("rcvbuf"); // SO_RCVBUF
               int rcvBufLen = 0;
               if (intf.exists(p_rcvbuf) && !intf.param(p_rcvbuf).empty())
               {
                  rcvBufLen = intf.param(p_rcvbuf).convertInt();
               }
               // maybe do transport-type dependent processing of buf?

               ExtensionParameter p_tls("tls"); // for specifying tls domain
               Transport *t = stack.addTransport(tt,
                                 intf.port(),
                                 DnsUtil::isIpV6Address(intf.host()) ? V6 : V4,
                                 StunEnabled, 
                                 intf.host(), // interface to bind to
                                 intf.param(p_tls));
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
               ExtensionParameter p_rr("rr"); // for specifying transport specific record route
               if(intf.exists(p_rr))
               {
                  // Note:  Any ';' in record-route must be escaped as %3b
                  // repro -i "sip:192.168.1.200:5060;transport=tcp;rr=sip:example.com%3btransport=tcp,sip:192.168.1.200:5060;transport=udp;rr=sip:example.com%3btransport=udp"
                  try
                  {
                     if(intf.param(p_rr).empty())
                     {
                        if(tt == TLS || tt == DTLS)
                        {
                           NameAddr rr;
                           rr.uri().host()=intf.param(p_tls);
                           rr.uri().port()=intf.port();
                           rr.uri().param(resip::p_transport)=resip::Tuple::toDataLower(tt);
                           t->setRecordRoute(rr);
                           InfoLog (<< "Transport specific record-route enabled (generated): " << rr);
                        }
                        else
                        {
                           NameAddr rr;
                           rr.uri().host()=intf.host();
                           rr.uri().port()=intf.port();
                           rr.uri().param(resip::p_transport)=resip::Tuple::toDataLower(tt);
                           t->setRecordRoute(rr);
                           InfoLog (<< "Transport specific record-route enabled (generated): " << rr);
                        }
                     }
                     else
                     {
                        NameAddr rr(intf.param(p_rr).charUnencoded());
                        t->setRecordRoute(rr);
                        InfoLog (<< "Transport specific record-route enabled: " << rr);
                     }
                  }
                  catch(BaseException& e)
                  {
                     ErrLog (<< "Invalid transport record-route uri provided (ignoring): " << e);
                     allTransportsSpecifyRecordRoute = false;
                  }
               }
               else 
               {
                  allTransportsSpecifyRecordRoute = false;
               }
            }
            catch(ParseException& e)
            {
               ErrLog (<< "Invalid transport uri provided (ignoring): " << e);
            }
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
   
   InteropHelper::setOutboundVersion(config.getConfigInt("OutboundVersion", 11));
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
#if defined(HAVE_EPOLL)
   if(config.getConfigBool("EventThread", true))
   {
      stackThread.reset(new EventStackThread(stack,
               *dynamic_cast<EventThreadInterruptor*>(threadInterruptor.get()),
               *pollGrp));
   }
   else
#endif

   stackThread.reset(new InterruptableStackThread(stack, *threadInterruptor));

   Registrar registrar;
   // We only need removed records to linger if we have reg sync enabled
   int xmlRpcPort = config.getConfigInt("XmlRpcPort", 0);
   InMemorySyncRegDb regData(xmlRpcPort ? 86400 /* 24 hours */ : 0 /* removeLingerSecs */);  // !slg! could make linger time a setting
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
   if(xmlRpcPort != 0)
   {
      std::list<RegSyncServer*> regSyncServerList;
      if(useV4) 
      {
         regSyncServerV4 = new RegSyncServer(&regData, xmlRpcPort, V4);
         regSyncServerList.push_back(regSyncServerV4);
      }
      if(useV6) 
      {
         regSyncServerV6 = new RegSyncServer(&regData, xmlRpcPort, V6);
          regSyncServerList.push_back(regSyncServerV6);
      }
      if(!regSyncServerList.empty())
      {
         regSyncServerThread = new RegSyncServerThread(regSyncServerList);
      }
      Data regSyncPeerAddress(config.getConfigData("RegSyncPeer", ""));
      if(!regSyncPeerAddress.empty())
      {
         regSyncClient = new RegSyncClient(&regData, regSyncPeerAddress, xmlRpcPort);
      }
   }

   /* Make it all go */
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
       adminThread->shutdown();
   if (dumThread)
   {
       dumThread->shutdown();
   }
   if(regSyncServerThread)
   {
      regSyncServerThread->shutdown();
   }
   if(regSyncClient)
   {
      regSyncClient->shutdown();
   }

   proxy.join();
   stackThread->join();
   if ( adminThread ) {
      adminThread->join();
      delete adminThread; adminThread = NULL;
   }
   if ( admin ) {
      delete admin; admin = NULL;
   }
   if (dumThread)
   {
      dumThread->join();
      delete dumThread;
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
