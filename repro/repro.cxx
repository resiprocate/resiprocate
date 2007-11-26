#include <signal.h>
#include "resip/stack/MessageFilterRule.hxx"
#include "resip/stack/Security.hxx"
#include "resip/stack/Compression.hxx"
#include "resip/stack/ExtensionParameter.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/StackThread.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/dum/DumThread.hxx"
#include "resip/dum/InMemoryRegistrationDatabase.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/FileSystem.hxx"
#include "rutil/Inserter.hxx"

#ifdef WIN32
#include "rutil/Win32EventLog.hxx"
#endif

#include "repro/CommandLineParser.hxx"
#include "repro/Proxy.hxx"
#include "repro/Registrar.hxx"
#include "repro/ReproServerAuthManager.hxx"
#include "repro/ReproServerAuthManager.hxx"
#include "repro/ProcessorChain.hxx"
#include "repro/Store.hxx"
#include "repro/UserStore.hxx"
#include "repro/ConfigStore.hxx"
#include "repro/AclStore.hxx"
#include "repro/RouteStore.hxx"
#include "repro/AbstractDb.hxx"
#include "repro/BerkeleyDb.hxx"
#include "repro/WebAdmin.hxx"
#include "repro/WebAdminThread.hxx"
#include "repro/monkeys/IsTrustedNode.hxx"
#include "repro/monkeys/AmIResponsible.hxx"
#include "repro/monkeys/ConstantLocationMonkey.hxx"
#include "repro/monkeys/DigestAuthenticator.hxx"
#include "repro/monkeys/LocationServer.hxx"
#include "repro/monkeys/RecursiveRedirect.hxx"
#include "repro/monkeys/SimpleStaticRoute.hxx"
#include "repro/monkeys/StaticRoute.hxx"
#include "repro/monkeys/StrictRouteFixup.hxx"
#include "repro/monkeys/QValueTargetHandler.hxx"
#include "repro/monkeys/SimpleTargetHandler.hxx"
#include "repro/monkeys/SetTargetConnection.hxx"

#include "repro/Parameters.hxx"

#if defined(USE_SSL)
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

bool reproRestartServer = false;

#ifdef WIN32
static const char* ReproServiceName="ReproService";
static int ReproStateNum=1;
static volatile enum __ReproState { reproStarting, reproWorking, reproFinishing, reproEnd } ReproState = reproStarting; 
static bool ReproService=true;  
static SERVICE_STATUS_HANDLE svcH;
SERVICE_STATUS SvcStat={ SERVICE_WIN32, SERVICE_START_PENDING, 
      SERVICE_ACCEPT_STOP, 0, 0, 0, 5000 };  
#define SetSvcStat() \
   if ( ReproService ) \
   { \
      ReproStateNum++;\
      SvcStat.dwWaitHint = ReproStateNum;\
      switch ( ReproState )\
      {\
      case reproStarting:\
         SvcStat.dwCurrentState = SERVICE_START_PENDING;\
         break;\
      case reproWorking:\
         SvcStat.dwCurrentState = SERVICE_RUNNING;\
         break;\
      case reproFinishing:\
         SvcStat.dwCurrentState = SERVICE_STOP_PENDING;\
         break;\
      case reproEnd:\
         SvcStat.dwCurrentState = SERVICE_STOPPED;\
         break;\
      }\
      SetServiceStatus( svcH, &SvcStat);\
   }
#endif

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

   const AbstractConfigStore::ConfigData& dList = store.mConfigStore->getConfigs();
   for (  AbstractConfigStore::ConfigData::const_iterator i=dList.begin(); 
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
      realm =DnsUtil::getLocalHostName();
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

void reproMain( CommandLineParser *args, Store &store)
{
   Security* security = 0;
   Compression* compression = 0;

#ifdef USE_SSL
   security = new Security(args->mCertPath);
#endif

#ifdef USE_SIGCOMP
   compression = new Compression(Compression::DEFLATE);
#endif

   SipStack stack(security,DnsStub::EmptyNameserverList,0,false,0,compression);

   std::vector<Data> enumSuffixes;
   if (!args->mEnumSuffix.empty())
   {
      enumSuffixes.push_back(args->mEnumSuffix);
      stack.setEnumSuffixes(enumSuffixes);
   }

#ifdef WIN32
   SetSvcStat();
#endif

   try
   {
      // An example of how to use this follows. This sets up 2 transports, 1 TLS
      // and 1 UDP. TLS domain is bound to example.com. 
      // repro -i "sip:192.168.1.200:5060;transport=tls;tls=example.com,sip:192.168.1.200:5060;transport=udp"
      // Note: If you specify interfaces the other transport arguments have no effect
      if (!args->mInterfaces.empty())
      {
         for (std::vector<Data>::iterator i=args->mInterfaces.begin(); 
              i != args->mInterfaces.end(); ++i)
         {
            Uri intf(*i);
            ExtensionParameter p_tls("tls"); // for specifying tls domain
            stack.addTransport(Tuple::toTransport(intf.param(p_transport)),
                               intf.port(), 
                               DnsUtil::isIpV6Address(intf.host()) ? V6 : V4,
                               StunEnabled, 
                               intf.host(), // interface to bind to
                               intf.param(p_tls));
         }
      }
      else
      {
         if (args->mUseV4) InfoLog (<< "V4 enabled");
         if (args->mUseV6) InfoLog (<< "V6 enabled");         

         if (args->mUdpPort)
         {
            if (args->mUseV4) stack.addTransport(UDP, args->mUdpPort, V4, StunEnabled);
            if (args->mUseV6) stack.addTransport(UDP, args->mUdpPort, V6, StunEnabled);
         }
         if (args->mTcpPort)
         {
            if (args->mUseV4) stack.addTransport(TCP, args->mTcpPort, V4, StunEnabled);
            if (args->mUseV6) stack.addTransport(TCP, args->mTcpPort, V6, StunEnabled);
         }
         if (args->mTlsPort)
         {
            if (args->mUseV4) stack.addTransport(TLS, args->mTlsPort, V4, StunEnabled, Data::Empty, args->mTlsDomain);
            if (args->mUseV6) stack.addTransport(TLS, args->mTlsPort, V6, StunEnabled, Data::Empty, args->mTlsDomain);
         }
         if (args->mDtlsPort)
         {
            if (args->mUseV4) stack.addTransport(DTLS, args->mTlsPort, V4, StunEnabled, Data::Empty, args->mTlsDomain);
            if (args->mUseV6) stack.addTransport(DTLS, args->mTlsPort, V6, StunEnabled, Data::Empty, args->mTlsDomain);
         }
      }
   }
   catch (Transport::Exception& e)
   {
      std::cerr << "Likely a port is already in use" << endl;
      InfoLog (<< "Caught: " << e);
#ifndef WIN32
      exit(-1);
#else 
      if ( ReproService )
         return;
      else
         exit(-1);
#endif
   }
   
#ifdef WIN32
   SetSvcStat();
#endif

   StackThread stackThread(stack);

   Registrar registrar;
   InMemoryRegistrationDatabase regData;
   SharedPtr<MasterProfile> profile(new MasterProfile);
 

#ifdef WIN32
   SetSvcStat();
#endif


#ifdef WIN32
   SetSvcStat();
#endif

   /* Initialize a proxy */
   
   /* Explanation:  "Monkeys" are processors which operate on incoming requests
                    "Lemurs"  are processors which operate on incoming responses
                    "Baboons" are processors which operate on a request for each target  
                              as the request is about to be forwarded to that target */
      
   ProcessorChain requestProcessors(Processor::REQUEST_CHAIN);   // Monkeys
   ProcessorChain responseProcessors(Processor::RESPONSE_CHAIN); // Lemurs
   ProcessorChain targetProcessors(Processor::TARGET_CHAIN);     // Baboons

#if 0
   if (args->mRequestProcessorChainName=="StaticTest")
   {
      ConstantLocationMonkey* testMonkey = new ConstantLocationMonkey();
      requestProcessors.addProcessor(std::auto_ptr<Processor>(testMonkey));
   }
   else
#endif
   {
      // Either the chainName is default or we don't know about it
      // Use default if we don't recognize the name
      // Should log about it.
      ProcessorChain* locators = new ProcessorChain();
      
      StrictRouteFixup* srf = new StrictRouteFixup;
      locators->addProcessor(std::auto_ptr<Processor>(srf));

      SetTargetConnection* stc = new SetTargetConnection;   
      locators->addProcessor(std::auto_ptr<Processor>(stc)); 
      
      IsTrustedNode* isTrusted = new IsTrustedNode(*store.mAclStore);
      locators->addProcessor(std::auto_ptr<Processor>(isTrusted));

#ifdef WIN32
   SetSvcStat();
#endif

      if (!args->mNoChallenge)
      {
         DigestAuthenticator* da = new DigestAuthenticator(*store.mUserStore,
                                                           &stack,args->mNoIdentityHeaders,
                                                           args->mHttpPort,
                                                           !args->mNoAuthIntChallenge /*useAuthInt*/);
         locators->addProcessor(std::auto_ptr<Processor>(da)); 
      }

      AmIResponsible* isme = new AmIResponsible;
      locators->addProcessor(std::auto_ptr<Processor>(isme));
      
      // [TODO] support for GRUU is on roadmap.  When it is added the GruuMonkey will go here
      
      // [TODO] support for Manipulating Tel URIs is on the roadmap.
      //        When added, the telUriMonkey will go here 
     
      if (args->mRouteSet.empty())
      {
         StaticRoute* sr = new StaticRoute(*store.mRouteStore, args->mNoChallenge, args->mParallelForkStaticRoutes, !args->mNoAuthIntChallenge /*useAuthInt*/);
         locators->addProcessor(std::auto_ptr<Processor>(sr));
      }
      else
      {
         resip::NameAddrs routes;
         for (std::vector<Data>::iterator i=args->mRouteSet.begin(); 
              i != args->mRouteSet.end(); ++i)
         {
            routes.push_back(NameAddr(*i));
         }
         SimpleStaticRoute* sr = new SimpleStaticRoute(routes);
         locators->addProcessor(std::auto_ptr<Processor>(sr));
      }
      
      LocationServer* ls = new LocationServer(regData, args->mParallelForkStaticRoutes);
      locators->addProcessor(std::auto_ptr<Processor>(ls));
 
      requestProcessors.addProcessor(auto_ptr<Processor>(locators));      

   }

   if (args->mRecursiveRedirect)
   {
      ProcessorChain* lemurs = new ProcessorChain;
      RecursiveRedirect* red = new RecursiveRedirect;
      lemurs->addProcessor(std::auto_ptr<Processor>(red));
      responseProcessors.addProcessor(auto_ptr<Processor>(lemurs));      
   }
   
#ifdef WIN32
   SetSvcStat();
#endif

   ProcessorChain* baboons = new ProcessorChain;

   if( args->mDoQValue)
   {
      QValueTargetHandler::ForkBehavior behavior=QValueTargetHandler::EQUAL_Q_PARALLEL;
      
      if(args->mForkBehavior=="FULL_SEQUENTIAL")
      {
         behavior=QValueTargetHandler::FULL_SEQUENTIAL;
      }
      else if(args->mForkBehavior=="FULL_PARALLEL")
      {
         behavior=QValueTargetHandler::FULL_PARALLEL;
      }
      
      QValueTargetHandler* qval = 
         new QValueTargetHandler(behavior,
                                 args->mCancelBetweenForkGroups, //Cancel btw fork groups?
                                 args->mWaitForTerminate, //Wait for termination btw fork groups?
                                 args->mMsBetweenForkGroups, //ms between fork groups, moot in this case
                                 args->mMsBeforeCancel //ms before cancel
                                 );
      baboons->addProcessor(std::auto_ptr<Processor>(qval));
   }
   
   SimpleTargetHandler* smpl = new SimpleTargetHandler;
   baboons->addProcessor(std::auto_ptr<Processor>(smpl));
   
   targetProcessors.addProcessor(auto_ptr<Processor>(baboons));

#ifdef WIN32
   SetSvcStat();
#endif

   Proxy proxy(stack, 
               args->mRecordRoute, 
               requestProcessors, 
               responseProcessors, 
               targetProcessors, 
               *store.mUserStore,
               args->mTimerC );
   Data realm = addDomains(proxy, *args, store);
   
#ifdef USE_SSL
   WebAdmin admin( store, regData, security, args->mNoWebChallenge, realm, args->mAdminPassword, args->mHttpPort  );
#else
   WebAdmin admin( store, regData, NULL, args->mNoWebChallenge, realm, args->mAdminPassword, args->mHttpPort  );
#endif
   if (!admin.isSane())
   {
     CritLog(<<"Failed to start the WebAdmin - exiting");
#ifndef WIN32
      exit(-1);
#else 
      if ( ReproService )
         return;
      else
         exit(-1);
#endif
   }
   WebAdminThread adminThread(admin);

   profile->clearSupportedMethods();
   profile->addSupportedMethod(resip::REGISTER);
#ifdef USE_SSL
   profile->addSupportedScheme(Symbols::Sips);
#endif
   if(args->mAllowBadReg)
   {
       profile->allowBadRegistrationEnabled() = true;
   }
   
#ifdef WIN32
   SetSvcStat();
#endif

   DialogUsageManager* dum = 0;
   DumThread* dumThread = 0;

   resip::MessageFilterRuleList ruleList;
   if (!args->mNoRegistrar || args->mCertServer)
   {
      dum = new DialogUsageManager(stack);
      dum->setMasterProfile(profile);
      addDomains(*dum, *args, store);
   }

   if (!args->mNoRegistrar)
   {   
      assert(dum);
      dum->setServerRegistrationHandler(&registrar);
      dum->setRegistrationPersistenceManager(&regData);

      // Install rules so that the registrar only gets REGISTERs
      resip::MessageFilterRule::MethodList methodList;
      methodList.push_back(resip::REGISTER);
      ruleList.push_back(MessageFilterRule(resip::MessageFilterRule::SchemeList(),
                                           resip::MessageFilterRule::Any,
                                           methodList) );
   }
   
#ifdef WIN32
   SetSvcStat();
#endif

#if defined(USE_SSL)
   CertServer* certServer = 0;
#endif
   if (args->mCertServer)
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
                                           resip::MessageFilterRule::Any,
                                           methodList,
                                           eventList));
#endif
   }

   if (dum)
   {
      if (!args->mNoChallenge)
      {
         SharedPtr<ServerAuthManager> 
            uasAuth( new ReproServerAuthManager(*dum,
                                                *store.mUserStore,
                                                *store.mAclStore,
                                                !args->mNoAuthIntChallenge /*useAuthInt*/));
         dum->setServerAuthManager(uasAuth);
      }
      dum->setMessageFilterRuleList(ruleList);
      dumThread = new DumThread(*dum);
   }

   stack.registerTransactionUser(proxy);

   /* Make it all go */
   stackThread.run();
   proxy.run();
   adminThread.run();
   if (dumThread)
   {
      dumThread->run();
      Sleep(10000);
   }
   
#ifdef WIN32
   ReproState = reproWorking;
   SetSvcStat();
#endif

   while (!finished && !reproRestartServer)
   {
#ifdef WIN32
   Sleep(1000);
#else
   usleep(100000);
#endif
   }

#ifdef WIN32
   ReproState = reproFinishing;
   SetSvcStat();
#endif


   proxy.shutdown();
   stackThread.shutdown();
   adminThread.shutdown();

   if (dumThread)
   {
       dumThread->shutdown();
   }

   proxy.join();
   stackThread.join();
   adminThread.join();

#ifdef WIN32
   SetSvcStat();
#endif

   if (dumThread)
   {
      dumThread->join();
      delete dumThread;
   }

#ifdef WIN32
   SetSvcStat();
#endif

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

#ifdef WIN32
   SetSvcStat();
#endif

}

#ifdef WIN32

DWORD WINAPI ReproSvcHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
   SvcStat.dwWaitHint = ReproStateNum;
   if ( dwControl == SERVICE_CONTROL_INTERROGATE )
   {
      switch ( ReproState )
      {
      case reproStarting:
         SvcStat.dwCurrentState = SERVICE_START_PENDING;
         break;
      case reproWorking:
         SvcStat.dwCurrentState = SERVICE_RUNNING;
         break;
      case reproFinishing:
         SvcStat.dwCurrentState = SERVICE_STOP_PENDING;
         break;
      case reproEnd:
         SvcStat.dwCurrentState = SERVICE_STOPPED;
         break;
      }
      SetServiceStatus( svcH, &SvcStat);
      return NO_ERROR;
   }
   else if ( dwControl == SERVICE_CONTROL_STOP )
   {
      finished = true;
      while ( ReproState != reproEnd )
         Sleep( 1000 );
//      ReproState = reproFinishing;
      SvcStat.dwCurrentState = SERVICE_STOPPED;
      SetServiceStatus( svcH, &SvcStat);
      return NO_ERROR;
   }
   switch ( ReproState )
   {
   case reproStarting:
      SvcStat.dwCurrentState = SERVICE_START_PENDING;
      break;
   case reproWorking:
      SvcStat.dwCurrentState = SERVICE_RUNNING;
      break;
   case reproFinishing:
      SvcStat.dwCurrentState = SERVICE_STOP_PENDING;
      break;
   case reproEnd:
      SvcStat.dwCurrentState = SERVICE_STOPPED;
      break;
   }
   SetServiceStatus( svcH, &SvcStat);
   return ERROR_CALL_NOT_IMPLEMENTED;
}

CommandLineParser *args;
Store *store;

void WINAPI ReproServiceMain(DWORD dwArgc,LPTSTR* lpszArgv)
{
   ReproState = reproStarting; 
   svcH = RegisterServiceCtrlHandlerEx( ReproServiceName, &ReproSvcHandlerEx, &SvcStat);
   Win32EventLog *EventLog = new Win32EventLog( ReproServiceName );
   auto_ptr<Win32EventLog> EventLogpPtr( EventLog );
   if ( svcH ) 
   {
      SetServiceStatus(svcH,&SvcStat);
      NoticeLog (<< "Starting service " << ReproServiceName );
      do
      {
         reproRestartServer = false;
         if ( !args->mNoUseParameters )
            Parameters::StoreParametersInArgs( args );
         if(args->mLogType.lowercase() == "file")
         {
            Log::initialize("file", args->mLogLevel, ReproServiceName, (args->mLogFilePath+FileSystem::PathSeparator+"repro_log.txt").c_str() , EventLog );
         }
         else
         {
            Log::initialize(args->mLogType, args->mLogLevel, ReproServiceName, NULL, EventLog );
         }
         reproMain( args, *store );
      }
      while ( reproRestartServer );
      NoticeLog (<< "Service " << ReproServiceName << " stopped" );
   }
   if(args->mLogType.lowercase() == "file")
   {
      Log::initialize("file", args->mLogLevel, ReproServiceName, (args->mLogFilePath+FileSystem::PathSeparator+"repro_log.txt").c_str() , NULL );
   }
   else
   {
      Log::initialize(args->mLogType, args->mLogLevel, ReproServiceName, NULL, NULL );
   }
   ReproState = reproEnd;
   SetSvcStat();
}

bool installService(int argc, char** argv)
{
   SC_HANDLE sch = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS );
   if ( !sch )
   {
      return false;
   }
   size_t CmdLineSize = 0;
   for ( int i=0; i < argc; i++)
      CmdLineSize+=strlen( argv[i])+1;
   char *CmdLine = (char *)malloc( CmdLineSize + 1);
   *CmdLine=0;
   if ( strchr(argv[0] , (int) " " ) )
   {
      strcpy(CmdLine,"\"");
      strcat(CmdLine,argv[0]);
      strcat(CmdLine,"\"");
   }
   else
   {
      strcpy(CmdLine,argv[0]);
   }
   for ( int i=1; i < argc; i++)
      if ( stricmp( argv[i], "--install-service") )
      {
         strcat( CmdLine, " " );
         strcat( CmdLine, argv[i] );
      }
   SC_HANDLE svch=CreateService( sch, ReproServiceName, "Repro server", SERVICE_ALL_ACCESS, 
           SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_IGNORE, CmdLine,
           NULL, NULL, NULL, NULL, NULL);
   if ( !svch )
   {
      CloseServiceHandle(sch);
      return false;
   }
   CloseServiceHandle(svch);
   CloseServiceHandle(sch);

   HKEY hk; 
   DWORD dwData; 
   char *BaseKey="SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";
   char *Key=(char *)malloc( strlen( BaseKey ) + strlen( ReproServiceName ) + 1 );
   strcpy( Key, BaseKey );
   strcat( Key, ReproServiceName );
   RegCreateKey(HKEY_LOCAL_MACHINE, Key, &hk);
   RegSetValueEx(hk, "EventMessageFile", 0, REG_EXPAND_SZ, (LPBYTE) argv[0], strlen( argv[0] ) + 1);
   dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE; 
   RegSetValueEx(hk,"TypesSupported",0,REG_DWORD,(LPBYTE) &dwData,sizeof(DWORD));
   RegCloseKey(hk); 

   return true;
}

bool removeService()
{
   SC_HANDLE sch = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS );
   if ( !sch )
   {
      return false;
   }
   SC_HANDLE svch = OpenService( sch, ReproServiceName, SERVICE_ALL_ACCESS);
   if ( !svch )
   {
      return false;
   }
   SERVICE_STATUS svcstat;
   if ( ControlService( svch, SERVICE_CONTROL_STOP, &svcstat ) )
   {
      int i=0;
      while ( i<=20 && svcstat.dwCurrentState != SERVICE_STOPPED )
      {
         Sleep(1000);
         QueryServiceStatus( svch, &svcstat );
      }
      if ( svcstat.dwCurrentState != SERVICE_STOPPED )
         cerr << "Unable to stop service, service will be deleted after reboot\n";
   }   
   DeleteService( svch );
   CloseServiceHandle( svch );
   CloseServiceHandle( sch );
   return true;
}

#endif

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
   args = new CommandLineParser(argc, argv);
   std::auto_ptr<CommandLineParser> args_ptr(args);

#ifdef WIN32
//   assert(args->mInstallService && !args->mRemoveService || !args->mInstallService && args->mRemoveService);
   if ( args->mInstallService )
   {
      if ( installService( argc, argv ) )
      {
         cerr << "Service installed\n";
         exit(0);
      }
      else
      {
         cerr << "Failed to install service\n";
         exit(-1);
      }
   }
   if ( args->mRemoveService )
   {
      if ( removeService() )
      {
         cerr << "Service removed\n";
         exit(0);
      }
      else
      {
         cerr << "Failed to remove service\n";
         exit(-1);
      }
   }
#endif



   AbstractDb *db=NULL;
#ifdef USE_MYSQL
   if ( !args->mMySqlServer.empty() )
   {
      db = new MySqlDb(args->mMySqlServer);
   }
#endif
   if (!db)
   {
      try // at least under Windows BerkleyDb throw exception when can't open database
      {      
         db = new BerkeleyDb(args->mDbPath);
      }
      catch(...)
      {
         exit(-1);
      }
      if (!static_cast<BerkeleyDb*>(db)->isSane())
      {
        cerr <<"Failed to open configuration database";
        exit(-1);
      }
   }
   assert( db );

   Parameters::SetDb( db );
   store = new Store(new UserStore(*db), new RouteStore(*db), new AclStore(*db), new ConfigStore(*db) );
   if ( !args->mNoUseParameters )
      Parameters::StoreParametersInArgs( args );


#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   { FindMemoryLeaks fml;
#endif

#ifndef WIN32
   do
   {
      reproRestartServer = false;
      if ( !args->mNoUseParameters )
         Parameters::StoreParametersInArgs( args );
      if(args->mLogType.lowercase() == "file")
      {
         Log::initialize("file", args->mLogLevel, argv[0], (args->mLogFilePath+FileSystem::PathSeparator+"repro_log.txt").c_str() );
      }
      else
      {
         Log::initialize(args->mLogType, args->mLogLevel, argv[0]);
      }
      reproMain( args, *store );
   }
   while ( reproRestartServer );
#else
   SERVICE_TABLE_ENTRY SvcTE[]=
   {
      { (LPSTR)ReproServiceName, ReproServiceMain },
      {NULL,                     NULL}
   };
   if ( !StartServiceCtrlDispatcher( SvcTE ) )
   {
      if ( GetLastError() == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT ) //Ok we run repro in console
      {
         ReproService = false;
         do
         {
            reproRestartServer = false;
            if ( !args->mNoUseParameters )
               Parameters::StoreParametersInArgs( args );
            if(args->mLogType.lowercase() == "file")
            {
               Log::initialize("file", args->mLogLevel, argv[0], (args->mLogFilePath+FileSystem::PathSeparator+"repro_log.txt").c_str() );
            }
            else
            {
               Log::initialize(args->mLogType, args->mLogLevel, argv[0]);
            }
            reproMain( args, *store );
         }
         while ( reproRestartServer );
      }
      else
         exit(-1);
   }

#endif

   delete store;

   delete db; db=0;

#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   }
#endif
}
