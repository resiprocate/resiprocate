
#ifdef WIN32
#include <db_cxx.h>
#elif HAVE_CONFIG_H
#include "config.hxx"
#include DB_HEADER
#else 
#include <db4/db_cxx.h>
#endif

#include "resiprocate/MessageFilterRule.hxx"
#include "resiprocate/Security.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/StackThread.hxx"
#include "resiprocate/dum/DumThread.hxx"
#include "resiprocate/dum/InMemoryRegistrationDatabase.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/Log.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Inserter.hxx"

#include "repro/CommandLineParser.hxx"
#include "repro/Proxy.hxx"
#include "repro/Registrar.hxx"
#include "repro/ReproServerAuthManager.hxx"
#include "repro/ReproServerAuthManager.hxx"
#include "repro/RequestProcessorChain.hxx"
#include "repro/Store.hxx"
#include "repro/UserStore.hxx"
#include "repro/RouteStore.hxx"
#include "repro/AbstractDb.hxx"
#include "repro/BerkeleyDb.hxx"
#include "repro/WebAdmin.hxx"
#include "repro/WebAdminThread.hxx"
#include "repro/monkeys/AmIResponsible.hxx"
#include "repro/monkeys/ConstantLocationMonkey.hxx"
#include "repro/monkeys/DigestAuthenticator.hxx"
#include "repro/monkeys/LocationServer.hxx"
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

   tu.addDomain(DnsUtil::getLocalHostName());
   if ( realm.empty() )
   {
      realm =DnsUtil::getLocalHostName();
   }

   tu.addDomain("localhost");
   if ( realm.empty() )
   {
      realm = "localhost";
   }
   
#ifndef WIN32 // !cj! TODO 
   list<pair<Data,Data> > ips = DnsUtil::getInterfaces();
   for ( list<pair<Data,Data> >::const_iterator i=ips.begin(); i!=ips.end(); i++)
   {
      DebugLog( << "Adding domain for IP " << i->second  );
      tu.addDomain(i->second);
   }
#endif 

   tu.addDomain("127.0.0.1");

   return realm;
}


int
main(int argc, char** argv)
{
   /* Initialize a stack */
   CommandLineParser args(argc, argv);
   Log::initialize(args.mLogType, args.mLogLevel, argv[0]);

#ifdef USE_SSL
   Security security(args.mCertPath);
   SipStack stack(&security);
#else
    SipStack stack;
#endif

   try
   {
      if (args.mUseV4) InfoLog (<< "V4 enabled");
      if (args.mUseV6) InfoLog (<< "V6 enabled");         

      if (args.mUdpPort)
      {
         if (args.mUseV4) stack.addTransport(UDP, args.mUdpPort, V4);
#ifdef USE_IPV6
         if (args.mUseV6) stack.addTransport(UDP, args.mUdpPort, V6);
#endif
      }
      if (args.mTcpPort)
      {
         if (args.mUseV4) stack.addTransport(TCP, args.mUdpPort, V4);
#ifdef USE_IPV6
         if (args.mUseV6) stack.addTransport(TCP, args.mUdpPort, V6);
#endif
      }
#ifdef USE_SSL
      if (args.mTlsPort)
      {
         if (args.mUseV4) stack.addTransport(TLS, args.mTlsPort, V4, Data::Empty, args.mTlsDomain);
#ifdef USE_IPV6
         if (args.mUseV6) stack.addTransport(TLS, args.mTlsPort, V6, Data::Empty, args.mTlsDomain);
#endif
      }
      if (args.mDtlsPort)
      {
         if (args.mUseV4) stack.addTransport(DTLS, args.mTlsPort, V4, Data::Empty, args.mTlsDomain);
#ifdef USE_IPV6
         if (args.mUseV6) stack.addTransport(DTLS, args.mTlsPort, V6, Data::Empty, args.mTlsDomain);
#endif
      }
#endif
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
   MasterProfile profile;
 

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
   RequestProcessorChain requestProcessors;
   
   if (args.mRequestProcessorChainName=="StaticTest")
   {
      ConstantLocationMonkey* testMonkey = new ConstantLocationMonkey();
      requestProcessors.addProcessor(std::auto_ptr<RequestProcessor>(testMonkey));
   }
   else
   {
      // Either the chainName is default or we don't know about it
      // Use default if we don't recognize the name
      // Should log about it.
      RequestProcessorChain* locators = new RequestProcessorChain();
      
      StrictRouteFixup* srf = new StrictRouteFixup;
      locators->addProcessor(std::auto_ptr<RequestProcessor>(srf));
      
#if 0  // this is for request uri manipulation
      ManipulationMonkey* manip = new ManipulationMonkey
      locators->addProcessor(std::auto_ptr<RequestProcessor>(manip));
#endif

      if (!args.mNoChallenge)
      {
         DigestAuthenticator* da = new DigestAuthenticator;
         requestProcessors.addProcessor(std::auto_ptr<RequestProcessor>(da)); 
      }

      AmIResponsible* isme = new AmIResponsible;
      locators->addProcessor(std::auto_ptr<RequestProcessor>(isme));
      
      // [TODO] !rwm! put Gruu monkey here
      
      // [TODO] !rwm! put Tel URI monkey here 
      
      // TODO - remove next forwards all to 

#if 0 // static routes here
      ConstantLocationMonkey* cls = new ConstantLocationMonkey;
      locators->addProcessor(std::auto_ptr<RequestProcessor>(cls));
#endif
     
      StaticRoute* sr = new StaticRoute(store.mRouteStore);
      locators->addProcessor(std::auto_ptr<RequestProcessor>(sr));
 
      LocationServer* ls = new LocationServer(regData);
      locators->addProcessor(std::auto_ptr<RequestProcessor>(ls));
 
      requestProcessors.addProcessor(auto_ptr<RequestProcessor>(locators));      
   }
   
   Proxy proxy(stack, requestProcessors, store.mUserStore );
   Data realm = addDomains(proxy, args, store);
   
#ifdef USE_SSL
   WebAdmin admin( store, regData, &security, args.mNoWebChallenge, realm, args.mHttpPort  );
#else
   WebAdmin admin( store, regData, NULL, args.mNoWebChallenge, realm, args.mHttpPort  );
#endif
   WebAdminThread adminThread(admin);

   profile.clearSupportedMethods();
   profile.addSupportedMethod(resip::REGISTER);
   profile.addSupportedScheme(Symbols::Sips);
   
   DialogUsageManager* dum = 0;
   DumThread* dumThread = 0;


   resip::MessageFilterRuleList ruleList;
   if (!args.mNoRegistrar || args.mCertServer)
   {
      dum = new DialogUsageManager(stack);
      dum->setMasterProfile(&profile);
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
         auto_ptr<ServerAuthManager> 
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
   
   proxy.join();
   stackThread.join();
   adminThread.join();
   if (dumThread)
   {
      dumThread->join();
   }

   delete db; db=0;
}
