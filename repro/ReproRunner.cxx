#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <iostream>
#include <fstream>
#include <stdexcept>

#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/dns/DnsStub.hxx"
#include "rutil/GeneralCongestionManager.hxx"

#include "resip/stack/SipStack.hxx"
#include "resip/stack/Compression.hxx"
#include "resip/stack/EventStackThread.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "resip/stack/ConnectionManager.hxx"

#include "resip/dum/InMemorySyncRegDb.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/DumThread.hxx"
#include "resip/dum/TlsPeerAuthManager.hxx"

#include "repro/AsyncProcessorWorker.hxx"
#include "repro/ReproRunner.hxx"
#include "repro/Proxy.hxx"
#include "repro/ProxyConfig.hxx"
#include "repro/BerkeleyDb.hxx"
#include "repro/Dispatcher.hxx"
#include "repro/UserAuthGrabber.hxx"
#include "repro/ProcessorChain.hxx"
#include "repro/ReproVersion.hxx"
#include "repro/WebAdmin.hxx"
#include "repro/WebAdminThread.hxx"
#include "repro/Registrar.hxx"
#include "repro/ReproServerAuthManager.hxx"
#include "repro/RegSyncClient.hxx"
#include "repro/RegSyncServer.hxx"
#include "repro/RegSyncServerThread.hxx"
#include "repro/CommandServer.hxx"
#include "repro/CommandServerThread.hxx"
#include "repro/monkeys/IsTrustedNode.hxx"
#include "repro/monkeys/AmIResponsible.hxx"
#include "repro/monkeys/DigestAuthenticator.hxx"
#include "repro/monkeys/LocationServer.hxx"
#include "repro/monkeys/RecursiveRedirect.hxx"
#include "repro/monkeys/SimpleStaticRoute.hxx"
#include "repro/monkeys/StaticRoute.hxx"
#include "repro/monkeys/StrictRouteFixup.hxx"
#include "repro/monkeys/OutboundTargetHandler.hxx"
#include "repro/monkeys/QValueTargetHandler.hxx"
#include "repro/monkeys/SimpleTargetHandler.hxx"
#include "repro/monkeys/GeoProximityTargetSorter.hxx"
#include "repro/monkeys/RequestFilter.hxx"
#include "repro/monkeys/MessageSilo.hxx"
#include "repro/monkeys/CertificateAuthenticator.hxx"

#if defined(USE_SSL)
#include "repro/stateAgents/CertServer.hxx"
#include "resip/stack/ssl/Security.hxx"
#endif

#if defined(USE_MYSQL)
#include "repro/MySqlDb.hxx"
#endif

#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

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

ReproRunner::ReproRunner()
   : mRunning(false)
   , mRestarting(false)
   , mThreadedStack(false)
   , mSipAuthDisabled(false)
   , mUseV4(true)
   , mUseV6 (false)
   , mRegSyncPort(0)
   , mProxyConfig(0)
   , mFdPollGrp(0)
   , mAsyncProcessHandler(0)
   , mSipStack(0)
   , mStackThread(0)
   , mAbstractDb(0)
   , mRuntimeAbstractDb(0)
   , mRegistrationPersistenceManager(0)
   , mAuthRequestDispatcher(0)
   , mAsyncProcessorDispatcher(0)
   , mMonkeys(0)
   , mLemurs(0)
   , mBaboons(0)
   , mProxy(0)
   , mWebAdmin(0)
   , mWebAdminThread(0)
   , mRegistrar(0)
   , mDum(0)
   , mDumThread(0)
   , mCertServer(0)
   , mRegSyncClient(0)
   , mRegSyncServerV4(0)
   , mRegSyncServerV6(0)
   , mRegSyncServerThread(0)
   , mCommandServerV4(0)
   , mCommandServerV6(0)
   , mCommandServerThread(0)
   , mCongestionManager(0)
{
}

ReproRunner::~ReproRunner()
{
   if(mRunning) shutdown();
}

bool
ReproRunner::run(int argc, char** argv)
{
   if(mRunning) return false;

   if(!mRestarting)
   {
      // Store original arc and argv - so we can reuse them on restart request
      mArgc = argc;
      mArgv = argv;
   }

   // Parse command line and configuration file
   assert(!mProxyConfig);
   Data defaultConfigFilename("repro.config");
   try
   {
      mProxyConfig = new ProxyConfig();
      mProxyConfig->parseConfig(mArgc, mArgv, defaultConfigFilename);
   }
   catch(BaseException& ex)
   {
      std::cerr << "Error parsing configuration: " << ex << std::endl;
      return false;
   }

   // Non-Windows server process stuff
   if(!mRestarting)
   {
      setPidFile(mProxyConfig->getConfigData("PidFile", "", true));
      if(mProxyConfig->getConfigBool("Daemonize", false))
      {
         daemonize();
      }
   }

   // Initialize resip logger
   GenericLogImpl::MaxByteCount = mProxyConfig->getConfigUnsignedLong("LogFileMaxBytes", 5242880 /*5 Mb */);
   Data loggingType = mProxyConfig->getConfigData("LoggingType", "cout", true);
   Data mAppName = mProxyConfig->getConfigData("LoggingInstanceName", mArgv[0], true);
   Log::initialize(loggingType, 
                   mProxyConfig->getConfigData("LogLevel", "INFO", true), 
                   mAppName, 
                   mProxyConfig->getConfigData("LogFilename", "repro.log", true).c_str(),
                   isEqualNoCase(loggingType, "file") ? &g_ReproLogger : 0); // if logging to file then write WARNINGS, and Errors to console still

   InfoLog( << "Starting repro version " << VersionUtils::instance().releaseVersion() << "...");

   // Create SipStack and associated objects
   if(!createSipStack())
   {
      return false;
   }

   // Create datastore
   if(!createDatastore())
   {
      return false;
   }

   // Create DialogUsageManager that handles ServerRegistration,
   // and potentially certificate subscription server
   createDialogUsageManager();

   // Create the Proxy and associate objects
   if(!createProxy())
   {
      return false;
   }

   // Create HTTP WebAdmin and Thread
   if(!createWebAdmin())
   {
      return false;
   }

   // Create reg sync components if required
   createRegSync();

   // Create command server if required
   if(!mRestarting)
   {
      createCommandServer();
   }

   // Make it all go - startup all threads
   mThreadedStack = mProxyConfig->getConfigBool("ThreadedStack", true);
   if(mThreadedStack)
   {
      // If configured, then start the sub-threads within the stack
      mSipStack->run();
   }
   mStackThread->run();
   if(mDumThread)
   {
      mDumThread->run();
   }
   mProxy->run();
   if(mWebAdminThread)
   {
      mWebAdminThread->run();
   }
   if(!mRestarting && mCommandServerThread)
   {
      mCommandServerThread->run();
   }
   if(mRegSyncServerThread)
   {
      mRegSyncServerThread->run();
   }
   if(mRegSyncClient)
   {
      mRegSyncClient->run();
   }

   mRunning = true;

   return true;
}

void 
ReproRunner::shutdown()
{
   if(!mRunning) return;

   // Tell all threads to shutdown
   if(mWebAdminThread)
   {
      mWebAdminThread->shutdown();
   }
   if(mDumThread)
   {
      mDumThread->shutdown();
   }
   mProxy->shutdown();
   mStackThread->shutdown();
   if(!mRestarting && mCommandServerThread)  // leave command server running if we are restarting
   {
      mCommandServerThread->shutdown();
   }
   if(mRegSyncServerThread)
   {
      mRegSyncServerThread->shutdown();
   }
   if(mRegSyncClient)
   {
      mRegSyncClient->shutdown();
   }

   // Wait for all threads to shutdown, and destroy objects
   mProxy->join();
   if(mThreadedStack)
   {
      mSipStack->shutdownAndJoinThreads();
   }
   mStackThread->join();
   if(mWebAdminThread) 
   {
      mWebAdminThread->join();
   }
   if(mDumThread)
   {
      mDumThread->join();
   }
   if(mAuthRequestDispatcher)
   {
      // Both proxy and dum threads are down at this point, we can 
      // destroy the authRequest dispatcher and associated threads now
      delete mAuthRequestDispatcher;
      mAuthRequestDispatcher = 0;
   }
   if(mAsyncProcessorDispatcher)
   {
      // Both proxy and dum threads are down at this point, we can 
      // destroy the async processor dispatcher and associated threads now
      delete mAsyncProcessorDispatcher;
      mAsyncProcessorDispatcher = 0;
   }
   if(!mRestarting && mCommandServerThread)  // we leave command server running during restart
   {
      mCommandServerThread->join();
   }
   if(mRegSyncServerThread)
   {
      mRegSyncServerThread->join();
   }
   if(mRegSyncClient)
   {
      mRegSyncClient->join();
   }

   mSipStack->setCongestionManager(0);

   cleanupObjects();
   mRunning = false;
}

void
ReproRunner::restart()
{
   if(!mRunning) return;
   mRestarting = true;
   shutdown();
   run(0, 0);
   mRestarting = false;
}

void
ReproRunner::cleanupObjects()
{
   delete mCongestionManager; mCongestionManager = 0;
   if(!mRestarting)
   {
      // We leave command server running during restart
      delete mCommandServerThread; mCommandServerThread = 0;
      delete mCommandServerV6; mCommandServerV6 = 0;
      delete mCommandServerV4; mCommandServerV4 = 0;
   }
   delete mRegSyncServerThread; mRegSyncServerThread = 0;
   delete mRegSyncServerV6; mRegSyncServerV6 = 0;
   delete mRegSyncServerV4; mRegSyncServerV4 = 0;
   delete mRegSyncClient; mRegSyncClient = 0;
#if defined(USE_SSL)
   delete mCertServer; mCertServer = 0;
#endif
   delete mDumThread; mDumThread = 0;
   delete mDum; mDum = 0;
   delete mRegistrar; mRegistrar = 0;
   delete mWebAdminThread; mWebAdminThread = 0;
   delete mWebAdmin; mWebAdmin = 0;
   delete mProxy; mProxy = 0;
   delete mBaboons; mBaboons = 0;
   delete mLemurs; mLemurs = 0;
   delete mMonkeys; mMonkeys = 0;
   delete mAuthRequestDispatcher; mAuthRequestDispatcher = 0;
   delete mAsyncProcessorDispatcher; mAsyncProcessorDispatcher = 0;
   if(!mRestarting) 
   {
      // If we are restarting then leave the In Memory Registration database intact
      delete mRegistrationPersistenceManager; mRegistrationPersistenceManager = 0;
   }
   delete mAbstractDb; mAbstractDb = 0;
   delete mRuntimeAbstractDb; mRuntimeAbstractDb = 0;
   delete mStackThread; mStackThread = 0;
   delete mSipStack; mSipStack = 0;
   delete mAsyncProcessHandler; mAsyncProcessHandler = 0;
   delete mFdPollGrp; mFdPollGrp = 0;
   delete mProxyConfig; mProxyConfig = 0;
}

bool
ReproRunner::createSipStack()
{
   // Override T1 timer if configured to do so
   unsigned long overrideT1 = mProxyConfig->getConfigInt("TimerT1", 0);
   if(overrideT1)
   {
      WarningLog(<< "Overriding T1! (new value is " << 
               overrideT1 << ")");
      resip::Timer::resetT1(overrideT1);
   }

   // Create Security (TLS / Certificates) and Compression (SigComp) objects if
   // pre-precessor defines are enabled
   Security* security = 0;
   Compression* compression = 0;
#ifdef USE_SSL
#ifdef WIN32
   Data certPath("C:\\sipCerts");
#else 
   Data certPath(getenv("HOME"));
   certPath += "/.sipCerts";
#endif
   mProxyConfig->getConfigValue("CertificatePath", certPath);
   security = new Security(certPath);
   Data caDir;
   mProxyConfig->getConfigValue("CADirectory", caDir);
   if(!caDir.empty())
   {
      security->addCADirectory(caDir);
   }
   Data caFile;
   mProxyConfig->getConfigValue("CAFile", caFile);
   if(!caFile.empty())
   {
      security->addCAFile(caFile);
   }
#endif

#ifdef USE_SIGCOMP
   compression = new Compression(Compression::DEFLATE);
#endif

   // Create EventThreadInterruptor used to wake up the stack for 
   // for reasons other than an Fd signalling
   assert(!mFdPollGrp);
   mFdPollGrp = FdPollGrp::create();
   assert(!mAsyncProcessHandler);
   mAsyncProcessHandler = new EventThreadInterruptor(*mFdPollGrp);

   // Set Flags that will enable/disable IPv4 and/or IPv6, based on 
   // configuration and pre-processor flags
   mUseV4 = !mProxyConfig->getConfigBool("DisableIPv4", false);
#ifdef USE_IPV6
   mUseV6 = mProxyConfig->getConfigBool("EnableIPv6", true);
#else
   bool useV6 = false;
#endif
   if (mUseV4) InfoLog (<< "V4 enabled");
   if (mUseV6) InfoLog (<< "V6 enabled");

   // Build DNS Server list from config
   DnsStub::NameserverList dnsServers;
   std::vector<resip::Data> dnsServersConfig;
   mProxyConfig->getConfigValue("DNSServers", dnsServersConfig);
   for(std::vector<resip::Data>::iterator it = dnsServersConfig.begin(); it != dnsServersConfig.end(); it++)
   {
      if((mUseV4 && DnsUtil::isIpV4Address(*it)) || (mUseV6 && DnsUtil::isIpV6Address(*it)))
      {
         InfoLog(<< "Using DNS Server from config: " << *it);
         dnsServers.push_back(Tuple(*it, 0, UNKNOWN_TRANSPORT).toGenericIPAddress());
      }
   }

   // Create the SipStack Object
   assert(!mSipStack);
   mSipStack = new SipStack(security,
                            dnsServers,
                            mAsyncProcessHandler,
                            /*stateless*/false,
                            /*socketFunc*/0,
                            compression,
                            mFdPollGrp);

   // Set any enum suffixes from configuration
   std::vector<Data> enumSuffixes;
   mProxyConfig->getConfigValue("EnumSuffixes", enumSuffixes);
   if (enumSuffixes.size() > 0)
   {
      mSipStack->setEnumSuffixes(enumSuffixes);
   }

   // Set any enum domains from configuration
   std::map<Data,Data> enumDomains;
   std::vector<Data> _enumDomains;
   mProxyConfig->getConfigValue("EnumDomains", _enumDomains);
   if (enumSuffixes.size() > 0)
   {
      for(std::vector<Data>::iterator it = _enumDomains.begin(); it != _enumDomains.end(); it++)
      {
         enumDomains[*it] = *it;
      }
      mSipStack->setEnumDomains(enumDomains);
   }

   // Add stack transports
   bool allTransportsSpecifyRecordRoute=false;
   if(!addTransports(allTransportsSpecifyRecordRoute))
   {
      cleanupObjects();
      return false;
   }

   // Enable and configure RFC5626 Outbound support
   InteropHelper::setOutboundVersion(mProxyConfig->getConfigInt("OutboundVersion", 5626));
   InteropHelper::setOutboundSupported(mProxyConfig->getConfigBool("DisableOutbound", false) ? false : true);
   InteropHelper::setRRTokenHackEnabled(mProxyConfig->getConfigBool("EnableFlowTokens", false));
   InteropHelper::setAssumeFirstHopSupportsOutboundEnabled(mProxyConfig->getConfigBool("AssumeFirstHopSupportsOutbound", false));
   Data clientNATDetectionMode = mProxyConfig->getConfigData("ClientNatDetectionMode", "DISABLED");
   if(isEqualNoCase(clientNATDetectionMode, "ENABLED"))
   {
      InteropHelper::setClientNATDetectionMode(InteropHelper::ClientNATDetectionEnabled);
   }
   else if(isEqualNoCase(clientNATDetectionMode, "PRIVATE_TO_PUBLIC"))
   {
      InteropHelper::setClientNATDetectionMode(InteropHelper::ClientNATDetectionPrivateToPublicOnly);
   }
   unsigned long outboundFlowTimer = mProxyConfig->getConfigUnsignedLong("FlowTimer", 0);
   if(outboundFlowTimer > 0)
   {
      InteropHelper::setFlowTimerSeconds(outboundFlowTimer);
      ConnectionManager::MinimumGcAge = 7200000; // Timeout connections not related to a flow timer after 2 hours - TODO make configurable
      ConnectionManager::EnableAgressiveGc = true;
   }

   // Check Path and RecordRoute settings, print warning if features are enabled that
   // require record-routing and record-route uri(s) is not configured
   bool assumePath = mProxyConfig->getConfigBool("AssumePath", false);
   bool forceRecordRoute = mProxyConfig->getConfigBool("ForceRecordRouting", false);
   Uri recordRouteUri;
   mProxyConfig->getConfigValue("RecordRouteUri", recordRouteUri);
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

   // Configure misc. stack settings
   mSipStack->setFixBadDialogIdentifiers(false);
   mSipStack->setFixBadCSeqNumbers(false);
   int statsLogInterval = mProxyConfig->getConfigInt("StatisticsLogInterval", 60);
   if(statsLogInterval > 0)
   {
      mSipStack->setStatisticsInterval(statsLogInterval);
      mSipStack->statisticsManagerEnabled() = true;
   }
   else
   {
      mSipStack->statisticsManagerEnabled() = false;
   }

   // Create Congestion Manager, if required
   assert(!mCongestionManager);
   if(mProxyConfig->getConfigBool("CongestionManagement", true))
   {
      Data metricData = mProxyConfig->getConfigData("CongestionManagementMetric", "WAIT_TIME", true);
      GeneralCongestionManager::MetricType metric = GeneralCongestionManager::WAIT_TIME;
      if(isEqualNoCase(metricData, "TIME_DEPTH"))
      {
         metric = GeneralCongestionManager::TIME_DEPTH;
      }
      else if(isEqualNoCase(metricData, "SIZE"))
      {
         metric = GeneralCongestionManager::SIZE;
      }
      else if(!isEqualNoCase(metricData, "WAIT_TIME"))
      {
         WarningLog( << "CongestionManagementMetric specified as an unknown value (" << metricData << "), defaulting to WAIT_TIME.");
      }
      mCongestionManager = new GeneralCongestionManager(
                                          metric, 
                                          mProxyConfig->getConfigUnsignedLong("CongestionManagementTolerance", 200));
      mSipStack->setCongestionManager(mCongestionManager);
   }

   // Create base thread to run stack in (note:  stack may use other sub-threads, depending on configuration)
   assert(!mStackThread);
   mStackThread = new EventStackThread(*mSipStack,
                                       *dynamic_cast<EventThreadInterruptor*>(mAsyncProcessHandler),
                                       *mFdPollGrp);
   return true;
}

bool 
ReproRunner::createDatastore()
{
   // Create Database access objects
   assert(!mAbstractDb);
   assert(!mRuntimeAbstractDb);
#ifdef USE_MYSQL
   Data mySQLServer;
   mProxyConfig->getConfigValue("MySQLServer", mySQLServer);
   if(!mySQLServer.empty())
   {
      mAbstractDb = new MySqlDb(mySQLServer, 
                       mProxyConfig->getConfigData("MySQLUser", ""), 
                       mProxyConfig->getConfigData("MySQLPassword", ""),
                       mProxyConfig->getConfigData("MySQLDatabaseName", ""),
                       mProxyConfig->getConfigUnsignedLong("MySQLPort", 0),
                       mProxyConfig->getConfigData("MySQLCustomUserAuthQuery", ""));
   }
   Data runtimeMySQLServer;
   mProxyConfig->getConfigValue("RuntimeMySQLServer", runtimeMySQLServer);
   if(!runtimeMySQLServer.empty())
   {
      mRuntimeAbstractDb = new MySqlDb(runtimeMySQLServer,
                       mProxyConfig->getConfigData("RuntimeMySQLUser", ""), 
                       mProxyConfig->getConfigData("RuntimeMySQLPassword", ""),
                       mProxyConfig->getConfigData("RuntimeMySQLDatabaseName", ""),
                       mProxyConfig->getConfigUnsignedLong("RuntimeMySQLPort", 0),
                       mProxyConfig->getConfigData("MySQLCustomUserAuthQuery", ""));
   }
#endif
   if (!mAbstractDb)
   {
      mAbstractDb = new BerkeleyDb(mProxyConfig->getConfigData("DatabasePath", "./", true));
   }
   assert(mAbstractDb);
   if(!mAbstractDb->isSane())
   {
      CritLog(<<"Failed to open configuration database");
      cleanupObjects();
      return false;
   }
   if(mRuntimeAbstractDb && !mRuntimeAbstractDb->isSane())
   {
      CritLog(<<"Failed to open runtime configuration database");
      cleanupObjects();
      return false;
   }
   mProxyConfig->createDataStore(mAbstractDb, mRuntimeAbstractDb);

   // Create ImMemory Registration Database
   mRegSyncPort = mProxyConfig->getConfigInt("RegSyncPort", 0);
   // We only need removed records to linger if we have reg sync enabled
   if(!mRestarting)  // If we are restarting then we left the InMemoryRegistrationDb intact at shutdown - don't recreate
   {
      assert(!mRegistrationPersistenceManager);
      mRegistrationPersistenceManager = new InMemorySyncRegDb(mRegSyncPort ? 86400 /* 24 hours */ : 0 /* removeLingerSecs */);  // !slg! could make linger time a setting
   }
   assert(mRegistrationPersistenceManager);

   // Copy contacts from the StaticRegStore to the RegistrationPersistanceManager
   populateRegistrations();

   return true;
}

void
ReproRunner::createDialogUsageManager()
{
   // Create Profile settings for DUM Instance that handles ServerRegistration,
   // and potentially certificate subscription server
   SharedPtr<MasterProfile> profile(new MasterProfile);
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
   if(mProxyConfig->getConfigBool("AllowBadReg", false))
   {
       profile->allowBadRegistrationEnabled() = true;
   }
   
   // Create DialogeUsageManager if Registrar or Certificate Server are enabled
   assert(!mRegistrar);
   assert(!mDum);
   assert(!mDumThread);
   mRegistrar = new Registrar;
   resip::MessageFilterRuleList ruleList;
   bool registrarEnabled = !mProxyConfig->getConfigBool("DisableRegistrar", false);
   bool certServerEnabled = mProxyConfig->getConfigBool("EnableCertServer", false);
   if (registrarEnabled || certServerEnabled)
   {
      mDum = new DialogUsageManager(*mSipStack);
      mDum->setMasterProfile(profile);
      addDomains(*mDum, false /* log? already logged when adding to Proxy - no need to log again*/);
   }

   // If registrar is enabled, configure DUM to handle REGISTER requests
   if (registrarEnabled)
   {   
      assert(mDum);
      assert(mRegistrationPersistenceManager);
      mDum->setServerRegistrationHandler(mRegistrar);
      mDum->setRegistrationPersistenceManager(mRegistrationPersistenceManager);

      // Install rules so that the registrar only gets REGISTERs
      resip::MessageFilterRule::MethodList methodList;
      methodList.push_back(resip::REGISTER);
      ruleList.push_back(MessageFilterRule(resip::MessageFilterRule::SchemeList(),
                                           resip::MessageFilterRule::DomainIsMe,
                                           methodList) );
   }
   
   // If Certificate Server is enabled, configure DUM to handle SUBSCRIBE and 
   // PUBLISH requests for events: credential and certificate
   assert(!mCertServer);
   if (certServerEnabled)
   {
#if defined(USE_SSL)
      mCertServer = new CertServer(*mDum);

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

   if (mDum)
   {
      bool enableCertAuth = mProxyConfig->getConfigBool("EnableCertificateAuthenticator", false);
      // Maintains existing behavior for non-TLS cert auth users
      bool digestChallengeThirdParties = !enableCertAuth;

      if(enableCertAuth)
      {
         // TODO: perhaps this should be initialised from the trusted node
         // monkey?  Or should the list of trusted TLS peers be independent
         // from the trusted node list?
         std::set<Data> trustedPeers;
         loadCommonNameMappings();
         SharedPtr<TlsPeerAuthManager> certAuth(new TlsPeerAuthManager(*mDum, mDum->dumIncomingTarget(), trustedPeers, true, mCommonNameMappings));
         mDum->addIncomingFeature(certAuth);
      }

      mSipAuthDisabled = mProxyConfig->getConfigBool("DisableAuth", false);

      // If Authentication is enabled, then configure DUM to authenticate requests
      if (!mSipAuthDisabled)
      {
         // Create UserAuthGrabber Worker Thread Pool if auth is enabled
         assert(!mAuthRequestDispatcher);
         int numAuthGrabberWorkerThreads = mProxyConfig->getConfigInt("NumAuthGrabberWorkerThreads", 2);
         if(numAuthGrabberWorkerThreads < 1) numAuthGrabberWorkerThreads = 1; // must have at least one thread
         std::auto_ptr<Worker> grabber(new UserAuthGrabber(mProxyConfig->getDataStore()->mUserStore));
         mAuthRequestDispatcher = new Dispatcher(grabber, mSipStack, numAuthGrabberWorkerThreads);

         SharedPtr<ServerAuthManager> 
            uasAuth( new ReproServerAuthManager(*mDum,
                                                mAuthRequestDispatcher,
                                                mProxyConfig->getDataStore()->mAclStore,
                                                !mProxyConfig->getConfigBool("DisableAuthInt", false) /*useAuthInt*/,
                                                mProxyConfig->getConfigBool("RejectBadNonces", false),
                                                digestChallengeThirdParties));
         mDum->setServerAuthManager(uasAuth);
      }

      // Set the MessageFilterRuleList on DUM and create a thread to run DUM in
      mDum->setMessageFilterRuleList(ruleList);
      mDumThread = new DumThread(*mDum);
   }   
}

bool
ReproRunner::createProxy()
{
   // Create AsyncProcessorDispatcher thread pool that is shared by the processsors for
   // any asyncronous tasks (ie: RequestFilter and MessageSilo processors)
   int numAsyncProcessorWorkerThreads = mProxyConfig->getConfigInt("NumAsyncProcessorWorkerThreads", 2);
   if(numAsyncProcessorWorkerThreads > 0)
   {
      assert(!mAsyncProcessorDispatcher);
      mAsyncProcessorDispatcher = new Dispatcher(std::auto_ptr<Worker>(new AsyncProcessorWorker), 
                                                 mSipStack, 
                                                 numAsyncProcessorWorkerThreads);
   }

   // Create proxy processor chains
   /* Explanation:  "Monkeys" are processors which operate on incoming requests
                    "Lemurs"  are processors which operate on incoming responses
                    "Baboons" are processors which operate on a request for each target  
                              as the request is about to be forwarded to that target */
   // Make Monkeys
   assert(!mMonkeys);
   mMonkeys = new ProcessorChain(Processor::REQUEST_CHAIN);
   makeRequestProcessorChain(*mMonkeys);
   InfoLog(<< *mMonkeys);

   // Make Lemurs
   assert(!mLemurs);
   mLemurs = new ProcessorChain(Processor::RESPONSE_CHAIN);
   makeResponseProcessorChain(*mLemurs);
   InfoLog(<< *mLemurs);

   // Make Baboons
   assert(!mBaboons);
   mBaboons = new ProcessorChain(Processor::TARGET_CHAIN);
   makeTargetProcessorChain(*mBaboons);
   InfoLog(<< *mBaboons);

   // Create main Proxy class
   assert(!mProxy);
   mProxy = new Proxy(*mSipStack, 
                      *mProxyConfig, 
                      *mMonkeys, 
                      *mLemurs, 
                      *mBaboons);
   mHttpRealm = addDomains(*mProxy, true);

   // Register the Proxy class a stack transaction user
   // Note:  This is done after creating the DialogUsageManager so that it acts 
   // like a catchall and will handle all requests the DUM does not
   mSipStack->registerTransactionUser(*mProxy);

   return true;
}

void 
ReproRunner::populateRegistrations()
{
   assert(mRegistrationPersistenceManager);
   assert(mProxyConfig);
   assert(mProxyConfig->getDataStore());

   // Copy contacts from the StaticRegStore to the RegistrationPersistanceManager
   StaticRegStore::StaticRegRecordMap& staticRegList = mProxyConfig->getDataStore()->mStaticRegStore.getStaticRegList();
   StaticRegStore::StaticRegRecordMap::iterator it = staticRegList.begin();
   for(; it != staticRegList.end(); it++)
   {
      try
      {
         Uri aor(it->second.mAor);

         ContactInstanceRecord rec;
         rec.mContact = NameAddr(it->second.mContact);
         rec.mSipPath = NameAddrs(it->second.mPath);
         rec.mRegExpires = NeverExpire;
         rec.mSyncContact = true;  // Tag this permanent contact as being a syncronized contact so that it will
                                    // be syncronized to a paired server (this is actually configuration information)
         mRegistrationPersistenceManager->updateContact(aor, rec);
      }
      catch(resip::ParseBuffer::Exception& e)  
      {
         // This should never happen, since the format should be verified before writing to DB
         ErrLog(<<"Failed to apply a static registration due to parse error: " << e);
      }
   }
}

bool
ReproRunner::createWebAdmin()
{
   assert(!mWebAdmin);
   assert(!mWebAdminThread);
   int httpPort = mProxyConfig->getConfigInt("HttpPort", 5080);
   if (httpPort) 
   {
      mWebAdmin = new WebAdmin(*mProxy,
                               *mRegistrationPersistenceManager, 
                               mHttpRealm, 
                               httpPort);
      if (!mWebAdmin->isSane())
      {
         CritLog(<<"Failed to start the WebAdmin");
         cleanupObjects();
         return false;
      }
      mWebAdminThread = new WebAdminThread(*mWebAdmin);
   }
   return true;
}

void
ReproRunner::createRegSync()
{
   assert(!mRegSyncClient);
   assert(!mRegSyncServerV4);
   assert(!mRegSyncServerV6);
   assert(!mRegSyncServerThread);
   if(mRegSyncPort != 0)
   {
      std::list<RegSyncServer*> regSyncServerList;
      if(mUseV4) 
      {
         mRegSyncServerV4 = new RegSyncServer(dynamic_cast<InMemorySyncRegDb*>(mRegistrationPersistenceManager), mRegSyncPort, V4);
         regSyncServerList.push_back(mRegSyncServerV4);
      }
      if(mUseV6) 
      {
         mRegSyncServerV6 = new RegSyncServer(dynamic_cast<InMemorySyncRegDb*>(mRegistrationPersistenceManager), mRegSyncPort, V6);
         regSyncServerList.push_back(mRegSyncServerV6);
      }
      if(!regSyncServerList.empty())
      {
         mRegSyncServerThread = new RegSyncServerThread(regSyncServerList);
      }
      Data regSyncPeerAddress(mProxyConfig->getConfigData("RegSyncPeer", ""));
      if(!regSyncPeerAddress.empty())
      {
         mRegSyncClient = new RegSyncClient(dynamic_cast<InMemorySyncRegDb*>(mRegistrationPersistenceManager), regSyncPeerAddress, mRegSyncPort);
      }
   }
}

void
ReproRunner::createCommandServer()
{
   assert(!mCommandServerV4);
   assert(!mCommandServerV6);
   assert(!mCommandServerThread);
   int commandPort = mProxyConfig->getConfigInt("CommandPort", 5081);
   if(commandPort != 0)
   {
      std::list<CommandServer*> commandServerList;
      if(mUseV4) 
      {
         mCommandServerV4 = new CommandServer(*this, commandPort, V4);
         commandServerList.push_back(mCommandServerV4);
      }
      if(mUseV6) 
      {
         mCommandServerV6 = new CommandServer(*this, commandPort, V6);
         commandServerList.push_back(mCommandServerV6);
      }
      if(!commandServerList.empty())
      {
         mCommandServerThread = new CommandServerThread(commandServerList);
      }
   }
}

Data
ReproRunner::addDomains(TransactionUser& tu, bool log)
{
   assert(mProxyConfig);
   Data realm;
   
   std::vector<Data> configDomains;
   if(mProxyConfig->getConfigValue("Domains", configDomains))
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

   const ConfigStore::ConfigData& dList = mProxyConfig->getDataStore()->mConfigStore.getConfigs();
   for (ConfigStore::ConfigData::const_iterator i=dList.begin(); 
           i != dList.end(); ++i)
   {
      if(log) InfoLog (<< "Adding domain " << i->second.mDomain << " from config");
      tu.addDomain( i->second.mDomain );
      if ( realm.empty() )
      {
         realm = i->second.mDomain;
      }
   }

   /* All of this logic has been commented out - the sysadmin must explicitly
      add any of the items below to the Domains config option in repro.config

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
   tu.addDomain("127.0.0.1"); */

   if( realm.empty() )
      realm = "Unconfigured";

   return realm;
}

bool
ReproRunner::addTransports(bool& allTransportsSpecifyRecordRoute)
{
   assert(mProxyConfig);
   assert(mSipStack);
   allTransportsSpecifyRecordRoute=false;
   bool useEmailAsSIP = mProxyConfig->getConfigBool("TLSUseEmailAsSIP", false);
   try
   {
      // Check if advanced transport settings are provided
      unsigned int transportNum = 1;
      Data settingKeyBase("Transport" + Data(transportNum));
      Data interfaceSettingKey(settingKeyBase + "Interface");
      Data interfaceSettings = mProxyConfig->getConfigData(interfaceSettingKey, "", true);
      if(!interfaceSettings.empty())
      {
         // Sample config file format for advanced transport settings
         // Transport1Interface = 192.168.1.106:5061
         // Transport1Type = TLS
         // Transport1TlsDomain = sipdomain.com
         // Transport1TlsClientVerification = None
         // Transport1RecordRouteUri = sip:sipdomain.com;transport=TLS
         // Transport1RcvBufLen = 2000

         allTransportsSpecifyRecordRoute = true;

         const char *anchor;
         while(!interfaceSettings.empty())
         {
            Data typeSettingKey(settingKeyBase + "Type");
            Data tlsDomainSettingKey(settingKeyBase + "TlsDomain");
            Data tlsCVMSettingKey(settingKeyBase + "TlsClientVerification");
            Data recordRouteUriSettingKey(settingKeyBase + "RecordRouteUri");
            Data rcvBufSettingKey(settingKeyBase + "RcvBufLen");

            // Parse out interface settings
            ParseBuffer pb(interfaceSettings);
            anchor = pb.position();
            pb.skipToEnd();
            pb.skipBackToChar(':');  // For IPv6 the last : should be the port
            pb.skipBackChar();
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
               TransportType tt = Tuple::toTransport(mProxyConfig->getConfigData(typeSettingKey, "UDP"));
               if(tt == UNKNOWN_TRANSPORT)
               {
                  CritLog(<< "Unknown transport type found in " << typeSettingKey << " setting: " << mProxyConfig->getConfigData(typeSettingKey, "UDP"));
               }
               Data tlsDomain = mProxyConfig->getConfigData(tlsDomainSettingKey, "");
               Data tlsCVMValue = mProxyConfig->getConfigData(tlsCVMSettingKey, "NONE");
               SecurityTypes::TlsClientVerificationMode cvm = SecurityTypes::None;
               if(isEqualNoCase(tlsCVMValue, "Optional"))
               {
                  cvm = SecurityTypes::Optional;
               }
               else if(isEqualNoCase(tlsCVMValue, "Mandatory"))
               {
                  cvm = SecurityTypes::Mandatory;
               }
               else if(!isEqualNoCase(tlsCVMValue, "None"))
               {
                  CritLog(<< "Unknown TLS client verification mode found in " << tlsCVMSettingKey << " setting: " << tlsCVMValue);
               }
               int rcvBufLen = mProxyConfig->getConfigInt(rcvBufSettingKey, 0);
               Transport *t = mSipStack->addTransport(tt,
                                 port,
                                 DnsUtil::isIpV6Address(ipAddr) ? V6 : V4,
                                 StunEnabled, 
                                 ipAddr,       // interface to bind to
                                 tlsDomain,
                                 Data::Empty,  // private key passphrase - not currently used
                                 SecurityTypes::TLSv1, // sslType
                                 0,            // transport flags
                                 cvm,          // tls client verification mode
                                 useEmailAsSIP);

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

               Data recordRouteUri = mProxyConfig->getConfigData(recordRouteUriSettingKey, "");
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
               return false;
            }

            // Check if there is another transport
            transportNum++;
            settingKeyBase = Data("Transport" + Data(transportNum));
            interfaceSettingKey = Data(settingKeyBase + "Interface");
            interfaceSettings = mProxyConfig->getConfigData(interfaceSettingKey, "", true);
         }
      }
      else
      {
         int udpPort = mProxyConfig->getConfigInt("UDPPort", 5060);
         int tcpPort = mProxyConfig->getConfigInt("TCPPort", 5060);
         int tlsPort = mProxyConfig->getConfigInt("TLSPort", 5061);
         int dtlsPort = mProxyConfig->getConfigInt("DTLSPort", 0);
         Data tlsDomain = mProxyConfig->getConfigData("TLSDomainName", "");
         Data tlsCVMValue = mProxyConfig->getConfigData("TLSClientVerification", "NONE");
         SecurityTypes::TlsClientVerificationMode cvm = SecurityTypes::None;
         if(isEqualNoCase(tlsCVMValue, "Optional"))
         {
            cvm = SecurityTypes::Optional;
         }
         else if(isEqualNoCase(tlsCVMValue, "Mandatory"))
         {
            cvm = SecurityTypes::Mandatory;
         }
         else if(!isEqualNoCase(tlsCVMValue, "None"))
         {
            CritLog(<< "Unknown TLS client verification mode found in TLSClientVerification setting: " << tlsCVMValue);
         }

         if (udpPort)
         {
            if (mUseV4) mSipStack->addTransport(UDP, udpPort, V4, StunEnabled);
            if (mUseV6) mSipStack->addTransport(UDP, udpPort, V6, StunEnabled);
         }
         if (tcpPort)
         {
            if (mUseV4) mSipStack->addTransport(TCP, tcpPort, V4, StunEnabled);
            if (mUseV6) mSipStack->addTransport(TCP, tcpPort, V6, StunEnabled);
         }
         if (tlsPort)
         {
            if (mUseV4) mSipStack->addTransport(TLS, tlsPort, V4, StunEnabled, Data::Empty, tlsDomain, Data::Empty, SecurityTypes::TLSv1, 0, cvm, useEmailAsSIP);
            if (mUseV6) mSipStack->addTransport(TLS, tlsPort, V6, StunEnabled, Data::Empty, tlsDomain, Data::Empty, SecurityTypes::TLSv1, 0, cvm, useEmailAsSIP);
         }
         if (dtlsPort)
         {
            if (mUseV4) mSipStack->addTransport(DTLS, dtlsPort, V4, StunEnabled, Data::Empty, tlsDomain);
            if (mUseV6) mSipStack->addTransport(DTLS, dtlsPort, V6, StunEnabled, Data::Empty, tlsDomain);
         }
      }
   }
   catch (BaseException& e)
   {
      std::cerr << "Likely a port is already in use" << endl;
      InfoLog (<< "Caught: " << e);
      return false;
   }
   return true;
}

void 
ReproRunner::addProcessor(repro::ProcessorChain& chain, std::auto_ptr<Processor> processor)
{
   chain.addProcessor(processor);
}

void
ReproRunner::loadCommonNameMappings()
{
   // Already loaded?
   if(!mCommonNameMappings.empty())
      return;

   Data mappingsFileName = mProxyConfig->getConfigData("CommonNameMappings", "");
   if(mappingsFileName.empty())
      return;

   InfoLog(<< "trying to load common name mappings from file: " << mappingsFileName);

   ifstream mappingsFile(mappingsFileName.c_str());
   if(!mappingsFile)
   {
      throw std::runtime_error("Error opening/reading mappings file");
   }

   string sline;
   while(getline(mappingsFile, sline))
   {
      Data line(sline);
      Data cn;
      PermittedFromAddresses permitted;
      ParseBuffer pb(line);

      pb.skipWhitespace();
      const char * anchor = pb.position();
      if(pb.eof() || *anchor == '#') continue;  // if line is a comment or blank then skip it

      // Look for end of name
      pb.skipToOneOf("\t");
      pb.data(cn, anchor);
      pb.skipChar('\t');

      while(!pb.eof())
      {
         pb.skipWhitespace();
         if(pb.eof())
            continue;

         Data value;
         anchor = pb.position();
         pb.skipToOneOf(",\r\n ");
         pb.data(value, anchor);
         if(!value.empty())
         {
            StackLog(<< "Loading CN '" << cn << "', found mapping '" << value << "'");
            permitted.insert(value);
         }
         if(!pb.eof())
            pb.skipChar();
      }

      DebugLog(<< "Loaded mapping for CN '" << cn << "', " << permitted.size() << " mapping(s)");
      mCommonNameMappings[cn] = permitted;
   }
}

void  // Monkeys
ReproRunner::makeRequestProcessorChain(ProcessorChain& chain)
{
   assert(mProxyConfig);
   assert(mRegistrationPersistenceManager);

   // Add strict route fixup monkey
   addProcessor(chain, std::auto_ptr<Processor>(new StrictRouteFixup));

   // Add is trusted node monkey
   addProcessor(chain, std::auto_ptr<Processor>(new IsTrustedNode(*mProxyConfig)));

   // Add Certificate Authenticator - if required
   if(mProxyConfig->getConfigBool("EnableCertificateAuthenticator", false))
   {
      // TODO: perhaps this should be initialised from the trusted node
      // monkey?  Or should the list of trusted TLS peers be independent
      // from the trusted node list?
      // Should we used the same trustedPeers object that was
      // passed to TlsPeerAuthManager perhaps?
      std::set<Data> trustedPeers;
      loadCommonNameMappings();
      addProcessor(chain, std::auto_ptr<Processor>(new CertificateAuthenticator(*mProxyConfig, mSipStack, trustedPeers, true, mCommonNameMappings)));
   }

   // Add digest authenticator monkey - if required
   if (!mSipAuthDisabled)
   {
      assert(mAuthRequestDispatcher);
      DigestAuthenticator* da = new DigestAuthenticator(*mProxyConfig, mAuthRequestDispatcher);

      addProcessor(chain, std::auto_ptr<Processor>(da)); 
   }

   // Add am I responsible monkey
   addProcessor(chain, std::auto_ptr<Processor>(new AmIResponsible)); 

   // Add RequestFilter monkey
   if(!mProxyConfig->getConfigBool("DisableRequestFilterProcessor", false))
   {
      if(mAsyncProcessorDispatcher)
      {
         addProcessor(chain, std::auto_ptr<Processor>(new RequestFilter(*mProxyConfig, mAsyncProcessorDispatcher)));
      }
      else
      {
         WarningLog(<< "Could not start RequestFilter Processor due to no worker thread pool (NumAsyncProcessorWorkerThreads=0)");
      }
   }

   // [TODO] support for GRUU is on roadmap.  When it is added the GruuMonkey will go here
      
   // [TODO] support for Manipulating Tel URIs is on the roadmap.
   //        When added, the telUriMonkey will go here 

   std::vector<Data> routeSet;
   mProxyConfig->getConfigValue("Routes", routeSet);
   if (routeSet.empty())
   {
      // add static route monkey
      addProcessor(chain, std::auto_ptr<Processor>(new StaticRoute(*mProxyConfig))); 
   }
   else
   {
      // add simple static route monkey
      addProcessor(chain, std::auto_ptr<Processor>(new SimpleStaticRoute(*mProxyConfig))); 
   }

   // Add location server monkey
   addProcessor(chain, std::auto_ptr<Processor>(new LocationServer(*mProxyConfig, *mRegistrationPersistenceManager, mAuthRequestDispatcher)));

   // Add message silo monkey
   if(mProxyConfig->getConfigBool("MessageSiloEnabled", false))
   {
      if(mAsyncProcessorDispatcher && mRegistrar)
      {
         MessageSilo* silo = new MessageSilo(*mProxyConfig, mAsyncProcessorDispatcher);
         mRegistrar->addRegistrarHandler(silo);
         addProcessor(chain, std::auto_ptr<Processor>(silo));
      }
      else
      {
         WarningLog(<< "Could not start MessageSilo Processor due to no worker thread pool (NumAsyncProcessorWorkerThreads=0) or Registrar");
      }
   }
}

void  // Lemurs
ReproRunner::makeResponseProcessorChain(ProcessorChain& chain)
{
   assert(mProxyConfig);
   assert(mRegistrationPersistenceManager);

   // Add outbound target handler lemur
   addProcessor(chain, std::auto_ptr<Processor>(new OutboundTargetHandler(*mRegistrationPersistenceManager))); 

   if (mProxyConfig->getConfigBool("RecursiveRedirect", false))
   {
      // Add recursive redirect lemur
      addProcessor(chain, std::auto_ptr<Processor>(new RecursiveRedirect)); 
   }
}

void  // Baboons
ReproRunner::makeTargetProcessorChain(ProcessorChain& chain)
{
   assert(mProxyConfig);

#ifndef RESIP_FIXED_POINT
   if(mProxyConfig->getConfigBool("GeoProximityTargetSorting", false))
   {
      addProcessor(chain, std::auto_ptr<Processor>(new GeoProximityTargetSorter(*mProxyConfig)));
   }
#endif

   if(mProxyConfig->getConfigBool("QValue", true))
   {
      // Add q value target handler baboon
      addProcessor(chain, std::auto_ptr<Processor>(new QValueTargetHandler(*mProxyConfig))); 
   }
   
   // Add simple target handler baboon
   addProcessor(chain, std::auto_ptr<Processor>(new SimpleTargetHandler)); 
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
 */
