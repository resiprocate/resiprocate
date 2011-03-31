#include <signal.h>
#include "resip/stack/MessageFilterRule.hxx"
#include "resip/stack/Compression.hxx"
#include "resip/stack/ExtensionParameter.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/InterruptableStackThread.hxx"
#include "resip/stack/EventStackThread.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "resip/dum/DumThread.hxx"
#include "resip/dum/InMemoryRegistrationDatabase.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/FdPoll.hxx"

#include "repro/CommandLineParser.hxx"
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
addDomains(TransactionUser& tu, CommandLineParser& args, Store& store)
{
   Data realm;
   
   for (std::vector<Data>::const_iterator i=args.mDomains.begin(); 
        i != args.mDomains.end(); ++i)
   {
      InfoLog (<< "Adding domain " << *i << " from command line");
      tu.addDomain(*i);
      if ( realm.empty() )
      {
         realm = *i;
      }
   }

   const ConfigStore::ConfigData& dList = store.mConfigStore.getConfigs();
   for (  ConfigStore::ConfigData::const_iterator i=dList.begin(); 
           i != dList.end(); ++i)
   {
      InfoLog (<< "Adding domain " << i->second.mDomain << " from config");
      tu.addDomain( i->second.mDomain );
      if ( realm.empty() )
      {
         realm = i->second.mDomain;
      }
   }

   Data localhostname(DnsUtil::getLocalHostName());
   InfoLog (<< "Adding local hostname domain " << localhostname );
   tu.addDomain(localhostname);
   if ( realm.empty() )
   {
      realm = localhostname;
   }

   tu.addDomain("localhost");
   if ( realm.empty() )
   {
      realm = "localhost";
   }
   
   list<pair<Data,Data> > ips = DnsUtil::getInterfaces();
   for ( list<pair<Data,Data> >::const_iterator i=ips.begin(); i!=ips.end(); i++)
   {
      InfoLog( << "Adding domain for IP " << i->second << " from interface " << i->first  );
      tu.addDomain(i->second);
   }

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
   CommandLineParser args(argc, argv);
   if(args.mLogType.lowercase() == "file")
   {
      GenericLogImpl::MaxLineCount = 50000; // 50000 about 5M size - should make this configurable
      Log::initialize("file", args.mLogLevel, argv[0], "repro_log.txt");
   }
   else
   {
      Log::initialize(args.mLogType, args.mLogLevel, argv[0]);
   }

#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   { FindMemoryLeaks fml;
#endif

   Security* security = 0;
   Compression* compression = 0;

#ifdef USE_SSL
   security = new Security(args.mCertPath);
#endif

#ifdef USE_SIGCOMP
   compression = new Compression(Compression::DEFLATE);
#endif

   if ( args.mUseInternalEPoll ) {
#if defined(RESIP_SIPSTACK_HAVE_FDPOLL)
      SipStack::setDefaultUseInternalPoll(args.mUseInternalEPoll);
#else
      cerr << "Poll not supported by SipStack" << endl;
      exit(1);
#endif
   }

   std::auto_ptr<FdPollGrp> pollGrp(NULL);
   std::auto_ptr<SelectInterruptor> threadInterruptor(NULL);
#if defined(HAVE_EPOLL)
   if (args.mUseEventThread)
   {
      pollGrp.reset(FdPollGrp::create());
      threadInterruptor.reset(new EventThreadInterruptor(*pollGrp));
   }
   else
#endif
   threadInterruptor.reset(new SelectInterruptor());

   SipStack stack(security,DnsStub::EmptyNameserverList,
                  threadInterruptor.get(),
                  /*stateless*/false,
                  /*socketFunc*/0,
                  compression,
                  pollGrp.get());

   std::vector<Data> enumSuffixes;
   if (!args.mEnumSuffix.empty())
   {
      enumSuffixes.push_back(args.mEnumSuffix);
      stack.setEnumSuffixes(enumSuffixes);
   }

   bool allTransportsSpecifyRecordRoute=false;
   try
   {
      // An example of how to use this follows. This sets up 2 transports, 1 TLS
      // and 1 UDP. TLS domain is bound to example.com. 
      // repro -i "sip:192.168.1.200:5060;transport=tls;tls=example.com,sip:192.168.1.200:5060;transport=udp"
      // Note: If you specify interfaces the other transport arguments have no effect
      if (!args.mInterfaces.empty())
      {
         allTransportsSpecifyRecordRoute = true;
         for (std::vector<Data>::iterator i=args.mInterfaces.begin(); 
              i != args.mInterfaces.end(); ++i)
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
         if (args.mUseV4) InfoLog (<< "V4 enabled");
         if (args.mUseV6) InfoLog (<< "V6 enabled");         

         if (args.mUdpPort)
         {
            if (args.mUseV4) stack.addTransport(UDP, args.mUdpPort, V4, StunEnabled);
            if (args.mUseV6) stack.addTransport(UDP, args.mUdpPort, V6, StunEnabled);
         }
         if (args.mTcpPort)
         {
            if (args.mUseV4) stack.addTransport(TCP, args.mTcpPort, V4, StunEnabled);
            if (args.mUseV6) stack.addTransport(TCP, args.mTcpPort, V6, StunEnabled);
         }
         if (args.mTlsPort)
         {
            if (args.mUseV4) stack.addTransport(TLS, args.mTlsPort, V4, StunEnabled, Data::Empty, args.mTlsDomain);
            if (args.mUseV6) stack.addTransport(TLS, args.mTlsPort, V6, StunEnabled, Data::Empty, args.mTlsDomain);
         }
         if (args.mDtlsPort)
         {
            if (args.mUseV4) stack.addTransport(DTLS, args.mDtlsPort, V4, StunEnabled, Data::Empty, args.mTlsDomain);
            if (args.mUseV6) stack.addTransport(DTLS, args.mDtlsPort, V6, StunEnabled, Data::Empty, args.mTlsDomain);
         }
      }
   }
   catch (BaseException& e)
   {
      std::cerr << "Likely a port is already in use" << endl;
      InfoLog (<< "Caught: " << e);
      exit(-1);
   }
   
   if((InteropHelper::getOutboundSupported() 
         || InteropHelper::getRRTokenHackEnabled()
         || InteropHelper::getClientNATDetectionMode() != InteropHelper::ClientNATDetectionDisabled
         || args.mAssumePath
         || args.mForceRecordRoute
      )
      && !(allTransportsSpecifyRecordRoute || !args.mRecordRoute.host().empty()))
   {
      CritLog(<< "In order for outbound support, the Record-Route flow-token"
      " hack, or force-record-route to work, you MUST specify a Record-Route URI. Launching "
      "without...");
      InteropHelper::setOutboundSupported(false);
      InteropHelper::setRRTokenHackEnabled(false);
      InteropHelper::setClientNATDetectionMode(InteropHelper::ClientNATDetectionDisabled);
      args.mAssumePath = false;
      args.mForceRecordRoute=false;
   }

   std::auto_ptr<ThreadIf> stackThread(NULL);
#if defined(HAVE_EPOLL)
   if ( args.mUseEventThread )
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
   InMemorySyncRegDb regData(args.mXmlRpcPort ? 86400 /* 24 hours */ : 0 /* removeLingerSecs */);  // !slg! could make linger time a setting
   SharedPtr<MasterProfile> profile(new MasterProfile);
 

   AbstractDb* db=NULL;
#ifdef USE_MYSQL
   if ( !args.mMySqlServer.empty() )
   {
      db = new MySqlDb(args.mMySqlServer);
   }
#endif
   if (!db)
   {
      db = new BerkeleyDb(args.mDbPath);
      if (!static_cast<BerkeleyDb*>(db)->isSane())
      {
        CritLog(<<"Failed to open configuration database");
        exit(-1);
      }
   }
   assert( db );
   Store store(*db);

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
      requestProcessors.addProcessor(std::auto_ptr<Processor>(new IsTrustedNode(store.mAclStore)));
      if (!args.mNoChallenge)
      {
         DigestAuthenticator* da = new DigestAuthenticator(store.mUserStore,
                                                           &stack,args.mNoIdentityHeaders,
                                                           args.mHttpHostname,
                                                           args.mHttpPort,
                                                           !args.mNoAuthIntChallenge /*useAuthInt*/,
                                                           args.mRejectBadNonces);
         // Add digest authenticator monkey
         requestProcessors.addProcessor(std::auto_ptr<Processor>(da)); 
      }

      // Add am I responsible monkey
      requestProcessors.addProcessor(std::auto_ptr<Processor>(new AmIResponsible)); 
      
      // [TODO] support for GRUU is on roadmap.  When it is added the GruuMonkey will go here
      
      // [TODO] support for Manipulating Tel URIs is on the roadmap.
      //        When added, the telUriMonkey will go here 
     
      if (args.mRouteSet.empty())
      {
         StaticRoute* sr = new StaticRoute(store.mRouteStore, 
                                           args.mNoChallenge, 
                                           args.mParallelForkStaticRoutes, 
                                           !args.mNoAuthIntChallenge /*useAuthInt*/);
         // add static route monkey
         requestProcessors.addProcessor(std::auto_ptr<Processor>(sr)); 
      }
      else
      {
         resip::NameAddrs routes;
         for (std::vector<Data>::iterator i=args.mRouteSet.begin(); 
              i != args.mRouteSet.end(); ++i)
         {
            routes.push_back(NameAddr(*i));
         }

         // add simple static route monkey
         requestProcessors.addProcessor(std::auto_ptr<Processor>(new SimpleStaticRoute(routes))); 
      }
      
      // Add location server monkey
      requestProcessors.addProcessor(std::auto_ptr<Processor>(new LocationServer(regData, 
                                                                                 args.mParallelForkStaticRoutes))); 
   }

   // Build Lemur Chain
   ProcessorChain responseProcessors(Processor::RESPONSE_CHAIN); // Lemurs
   // Add outbound target handler lemur
   responseProcessors.addProcessor(std::auto_ptr<Processor>(new OutboundTargetHandler(regData))); 
   if (args.mRecursiveRedirect)
   {
      // Add recursive redirect lemur
      responseProcessors.addProcessor(std::auto_ptr<Processor>(new RecursiveRedirect)); 
   }

   // Build Baboons Chain
   ProcessorChain targetProcessors(Processor::TARGET_CHAIN);     // Baboons   
   if( args.mDoQValue)
   {
      QValueTargetHandler::ForkBehavior behavior=QValueTargetHandler::EQUAL_Q_PARALLEL;
      
      if(args.mForkBehavior=="FULL_SEQUENTIAL")
      {
         behavior=QValueTargetHandler::FULL_SEQUENTIAL;
      }
      else if(args.mForkBehavior=="FULL_PARALLEL")
      {
         behavior=QValueTargetHandler::FULL_PARALLEL;
      }
      
      QValueTargetHandler* qval = 
         new QValueTargetHandler(behavior,
                                 args.mCancelBetweenForkGroups, //Cancel btw fork groups?
                                 args.mWaitForTerminate, //Wait for termination btw fork groups?
                                 args.mMsBetweenForkGroups, //ms between fork groups, moot in this case
                                 args.mMsBeforeCancel //ms before cancel
                                 );
      // Add q value target handler baboon
      targetProcessors.addProcessor(std::auto_ptr<Processor>(qval)); 
   }
   
   // Add simple target handler baboon
   targetProcessors.addProcessor(std::auto_ptr<Processor>(new SimpleTargetHandler)); 

   Proxy proxy(stack, 
               args.mRecordRoute, 
               args.mForceRecordRoute,
               requestProcessors, 
               responseProcessors, 
               targetProcessors, 
               store.mUserStore,
               args.mTimerC );
   Data realm = addDomains(proxy, args, store);
   
   proxy.setAssumePath(args.mAssumePath);
   proxy.addSupportedOption("outbound");
   proxy.setServerText(args.mServerText);

   WebAdmin *admin = NULL;
   WebAdminThread *adminThread = NULL;
   if ( args.mHttpPort ) {
#ifdef USE_SSL
      admin = new WebAdmin( store, regData, security, args.mNoWebChallenge, realm, args.mAdminPassword, args.mHttpPort  );
#else
      admin = new WebAdmin( store, regData, NULL, args.mNoWebChallenge, realm, args.mAdminPassword, args.mHttpPort  );
#endif
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
   profile->addSupportedOptionTag(Token(Symbols::Outbound));
   profile->addSupportedOptionTag(Token(Symbols::Path));
   if(args.mAllowBadReg)
   {
       profile->allowBadRegistrationEnabled() = true;
   }
   
   DialogUsageManager* dum = 0;
   DumThread* dumThread = 0;

   resip::MessageFilterRuleList ruleList;
   if (!args.mNoRegistrar || args.mCertServer)
   {
      dum = new DialogUsageManager(stack);
      dum->setMasterProfile(profile);
      addDomains(*dum, args, store);
   }

   if (!args.mNoRegistrar)
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
   if (args.mCertServer)
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
      if (!args.mNoChallenge)
      {
         SharedPtr<ServerAuthManager> 
            uasAuth( new ReproServerAuthManager(*dum,
                                                store.mUserStore,
                                                store.mAclStore,
                                                !args.mNoAuthIntChallenge /*useAuthInt*/,
                                                args.mRejectBadNonces));
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
   if(args.mXmlRpcPort != 0)
   {
      std::list<RegSyncServer*> regSyncServerList;
      if(args.mUseV4) 
      {
         regSyncServerV4 = new RegSyncServer(&regData, args.mXmlRpcPort, V4);
         regSyncServerList.push_back(regSyncServerV4);
      }
      if(args.mUseV6) 
      {
         regSyncServerV6 = new RegSyncServer(&regData, args.mXmlRpcPort, V6);
          regSyncServerList.push_back(regSyncServerV6);
      }
      if(!regSyncServerList.empty())
      {
         regSyncServerThread = new RegSyncServerThread(regSyncServerList);
      }
      if(!args.mRegSyncPeerAddress.empty())
      {
          regSyncClient = new RegSyncClient(&regData, args.mRegSyncPeerAddress, args.mXmlRpcPort);
      }
   }

   /* Make it all go */
   stackThread->run();
   proxy.run();
   if ( adminThread )
      adminThread->run();
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
