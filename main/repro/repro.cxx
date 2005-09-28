
#ifdef WIN32
#include <db_cxx.h>
#elif HAVE_CONFIG_H
#include "config.hxx"
#include DB_HEADER
#else 
#include <db4/db_cxx.h>
#include <sys/ipc.h>
#endif

#include <signal.h>
#include "resip/stack/MessageFilterRule.hxx"
#include "resip/stack/Security.hxx"
#include "resip/stack/ExtensionParameter.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/StackThread.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/dum/DumThread.hxx"
#include "resip/dum/InMemoryRegistrationDatabase.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"

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
#include "repro/monkeys/IsTrustedNode.hxx"
#include "repro/monkeys/AmIResponsible.hxx"
#include "repro/monkeys/ConstantLocationMonkey.hxx"
#include "repro/monkeys/DigestAuthenticator.hxx"
#include "repro/monkeys/LocationServer.hxx"
#include "repro/monkeys/RecursiveRedirect.hxx"
#include "repro/monkeys/SimpleStaticRoute.hxx"
#include "repro/monkeys/StaticRoute.hxx"
#include "repro/monkeys/StrictRouteFixup.hxx"

#if defined(USE_SSL)
#include "repro/stateAgents/CertServer.hxx"
#endif

#if defined(USE_MYSQL)
#include "repro/MySqlDb.hxx"
#endif

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

   ConfigStore::DataList dList = store.mConfigStore.getDomains();
   for (  ConfigStore::DataList::const_iterator i=dList.begin(); 
           i != dList.end(); ++i)
   {
      InfoLog (<< "Adding domain " << *i << " from config");
      tu.addDomain( *i );
      if ( realm.empty() )
      {
         realm = *i;
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
   CommandLineParser args(argc, argv);
   if(args.mLogType.lowercase() == "file")
   {
      Log::initialize("file", args.mLogLevel, argv[0], "repro_log.txt");
   }
   else
   {
      Log::initialize(args.mLogType, args.mLogLevel, argv[0]);
   }

#ifdef USE_SSL
   Security security(args.mCertPath);
   SipStack stack(&security);
#else
   SipStack stack;
#endif

   std::vector<Data> enumSuffixes;
   if (!args.mEnumSuffix.empty())
   {
      enumSuffixes.push_back(args.mEnumSuffix);
      stack.setEnumSuffixes(enumSuffixes);
   }

   try
   {
      // An example of how to use this follows. This sets up 2 transports, 1 TLS
      // and 1 UDP. TLS domain is bound to example.com. 
      // repro -i "sip:192.168.1.200:5060;transport=tls;tls=example.com,sip:192.168.1.200:5060;transport=udp"
      // Note: If you specify interfaces the other transport arguments have no effect
      if (!args.mInterfaces.empty())
      {
         for (std::vector<Data>::iterator i=args.mInterfaces.begin(); 
              i != args.mInterfaces.end(); ++i)
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
            if (args.mUseV4) stack.addTransport(DTLS, args.mTlsPort, V4, StunEnabled, Data::Empty, args.mTlsDomain);
            if (args.mUseV6) stack.addTransport(DTLS, args.mTlsPort, V6, StunEnabled, Data::Empty, args.mTlsDomain);
         }
      }
   }
   catch (Transport::Exception& e)
   {
      std::cerr << "Likely a port is already in use" << endl;
      InfoLog (<< "Caught: " << e);
      exit(-1);
   }
   
   StackThread stackThread(stack);

   Registrar registrar;
   InMemoryRegistrationDatabase regData;
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
      db = new BerkeleyDb;
   }
   assert( db );
   Store store(*db);

   /* Initialize a proxy */
   ProcessorChain requestProcessors;
   ProcessorChain responseProcessors;
   ProcessorChain targetProcessors;
   
   if (args.mRequestProcessorChainName=="StaticTest")
   {
      ConstantLocationMonkey* testMonkey = new ConstantLocationMonkey();
      requestProcessors.addProcessor(std::auto_ptr<Processor>(testMonkey));
   }
   else
   {
      // Either the chainName is default or we don't know about it
      // Use default if we don't recognize the name
      // Should log about it.
      ProcessorChain* locators = new ProcessorChain();
      
      StrictRouteFixup* srf = new StrictRouteFixup;
      locators->addProcessor(std::auto_ptr<Processor>(srf));
      
#if 0  // this is for request uri manipulation
      ManipulationMonkey* manip = new ManipulationMonkey
      locators->addProcessor(std::auto_ptr<Processor>(manip));
#endif

      IsTrustedNode* isTrusted = new IsTrustedNode;
      locators->addProcessor(std::auto_ptr<Processor>(isTrusted));

      if (!args.mNoChallenge)
      {
         DigestAuthenticator* da = new DigestAuthenticator;
         locators->addProcessor(std::auto_ptr<Processor>(da)); 
      }

      AmIResponsible* isme = new AmIResponsible;
      locators->addProcessor(std::auto_ptr<Processor>(isme));
      
      // [TODO] !rwm! put Gruu monkey here
      
      // [TODO] !rwm! put Tel URI monkey here 
      
      // TODO - remove next forwards all to 

#if 0 // static routes here
      ConstantLocationMonkey* cls = new ConstantLocationMonkey;
      locators->addProcessor(std::auto_ptr<Processor>(cls));
#endif
     
      if (args.mRouteSet.empty())
      {
         StaticRoute* sr = new StaticRoute(store.mRouteStore);
         locators->addProcessor(std::auto_ptr<Processor>(sr));
      }
      else
      {
         resip::NameAddrs routes;
         for (std::vector<Data>::iterator i=args.mRouteSet.begin(); 
              i != args.mRouteSet.end(); ++i)
         {
            routes.push_back(NameAddr(*i));
         }
         SimpleStaticRoute* sr = new SimpleStaticRoute(routes);
         locators->addProcessor(std::auto_ptr<Processor>(sr));
      }
      
      LocationServer* ls = new LocationServer(regData);
      locators->addProcessor(std::auto_ptr<Processor>(ls));
 
      requestProcessors.addProcessor(auto_ptr<Processor>(locators));      

   }

   if (args.mRecursiveRedirect)
   {
      ProcessorChain* lemurs = new ProcessorChain;
      RecursiveRedirect* red = new RecursiveRedirect;
      lemurs->addProcessor(std::auto_ptr<Processor>(red));
      responseProcessors.addProcessor(auto_ptr<Processor>(lemurs));      
   }
   
   Proxy proxy(stack, 
               args.mRecordRoute, 
               requestProcessors, 
               responseProcessors, 
               targetProcessors, 
               store.mUserStore );
   Data realm = addDomains(proxy, args, store);
   
#ifdef USE_SSL
   WebAdmin admin( store, regData, &security, args.mNoWebChallenge, realm, args.mHttpPort  );
#else
   WebAdmin admin( store, regData, NULL, args.mNoWebChallenge, realm, args.mHttpPort  );
#endif
   WebAdminThread adminThread(admin);

   profile->clearSupportedMethods();
   profile->addSupportedMethod(resip::REGISTER);
   profile->addSupportedScheme(Symbols::Sips);
   
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
                                           resip::MessageFilterRule::Any,
                                           methodList) );
   }
   
   if (args.mCertServer)
   {
#if defined(USE_SSL)
      CertServer* certServer = 0;
      certServer = new CertServer(*dum);

      // Install rules so that the cert server receives SUBSCRIBEs and PUBLISHs
      resip::MessageFilterRule::MethodList methodList;
      methodList.push_back(resip::SUBSCRIBE);
      methodList.push_back(resip::PUBLISH);
      ruleList.push_back(MessageFilterRule(resip::MessageFilterRule::SchemeList(),
                                           resip::MessageFilterRule::Any,
                                           methodList) );
#endif
   }

   if (dum)
   {
      if (!args.mNoChallenge)
      {
         SharedPtr<ServerAuthManager> 
            uasAuth( new ReproServerAuthManager(*dum,
                                                store.mUserStore ));
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
   }
   
   while (!finished)
   {
#ifdef WIN32
   Sleep(1000);
#else
   usleep(100000);
#endif
   }
   exit(0);

   proxy.join();
   stackThread.join();
   adminThread.join();
   if (dumThread)
   {
      dumThread->join();
   }

   delete db; db=0;
}
