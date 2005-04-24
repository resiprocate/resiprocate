
#ifdef WIN32
#include <db_cxx.h>
#else 
#include <db4/db_185.h>
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
#include "repro/RouteDbMemory.hxx"
#include "repro/UserDb.hxx"
#include "repro/WebAdmin.hxx"
#include "repro/WebAdminThread.hxx"
#include "repro/monkeys/AmIResponsible.hxx"
#include "repro/monkeys/ConstantLocationMonkey.hxx"
#include "repro/monkeys/DigestAuthenticator.hxx"
#include "repro/monkeys/LocationServer.hxx"
#include "repro/monkeys/RouteMonkey.hxx"
#include "repro/monkeys/RouteProcessor.hxx"

#if defined(USE_SSL)
#include "repro/stateAgents/CertServer.hxx"
#endif

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

using namespace repro;
using namespace resip;
using namespace std;


static void
addDomains(TransactionUser& tu, CommandLineParser& args)
{
   for (std::vector<Uri>::const_iterator i=args.mDomains.begin(); 
        i != args.mDomains.end(); ++i)
   {
      InfoLog (<< "Adding domain " << i->host() );
      tu.addDomain(i->host());
   }

   tu.addDomain(DnsUtil::getLocalHostName());

   tu.addDomain("localhost");

#ifndef WIN32 // !cj! TODO 
   list<pair<Data,Data> > ips = DnsUtil::getInterfaces();
   for ( list<pair<Data,Data> >::const_iterator i=ips.begin(); i!=ips.end(); i++)
   {
      DebugLog( << "Adding domain for IP " << i->second  );
      tu.addDomain(i->second);
   }
#endif 

   tu.addDomain("127.0.0.1");
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
   
   RouteDbMemory routeDb;
   
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
      
      RouteProcessor* rp = new RouteProcessor;
      locators->addProcessor(std::auto_ptr<RequestProcessor>(rp));
      
#if 0  // this is for request uri manipulation
      ManipulationMonkey* manip = new ManipulationMonkey
      locators->addProcessor(std::auto_ptr<RequestProcessor>(manip));
#endif

      AmIResponsible* isme = new AmIResponsible;
      locators->addProcessor(std::auto_ptr<RequestProcessor>(isme));
      
      // [TODO] !rwm! put Gruu monkey here
      
      // [TODO] !rwm! put Tel URI monkey here 
      
      // TODO - remove next forwards all to 

#if 0 // static routes here
      ConstantLocationMonkey* cls = new ConstantLocationMonkey;
      locators->addProcessor(std::auto_ptr<RequestProcessor>(cls));
#endif
     
      RouteMonkey* routeMonkey = new RouteMonkey(routeDb);
      locators->addProcessor(std::auto_ptr<RequestProcessor>(routeMonkey));
 
      LocationServer* ls = new LocationServer(regData);
      locators->addProcessor(std::auto_ptr<RequestProcessor>(ls));
 
      requestProcessors.addProcessor(auto_ptr<RequestProcessor>(locators));
      
      if (!args.mNoChallenge)
      {
         //TODO NEEDD TO FIX THIS 
         //DigestAuthenticator* da = new DigestAuthenticator;
         //requestProcessors.addProcessor(std::auto_ptr<RequestProcessor>(da)); 
      }
   }
   
   UserDb userDb;
   
   Proxy proxy(stack, requestProcessors, userDb);
   addDomains(proxy, args);
   
#ifdef USE_SSL
   WebAdmin admin(userDb, regData, routeDb, &security, args.mNoWebChallenge );
#else
   WebAdmin admin(userDb, regData, routeDb, NULL, args.mNoWebChallenge );
#endif
   WebAdminThread adminThread(admin);

   profile.clearSupportedMethods();
   profile.addSupportedMethod(resip::REGISTER);
   profile.addSupportedScheme(Symbols::Sips);
   
   DialogUsageManager* dum = 0;
   DumThread* dumThread = 0;

#if defined(USE_SSL)
   CertServer* certServer = 0;
#endif

   resip::MessageFilterRuleList ruleList;
   if (!args.mNoRegistrar || args.mCertServer)
   {
      dum = new DialogUsageManager(stack);
      dum->setMasterProfile(&profile);
      addDomains(*dum, args);
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
      certServer = new CertServer(*dum);

      // Install rules so that the registrar only gets REGISTERs
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
         auto_ptr<ServerAuthManager> uasAuth( new ReproServerAuthManager(*dum,userDb));
         dum->setServerAuthManager(uasAuth);
      }
      dum->setMessageFilterRuleList(ruleList);
      dumThread = new DumThread(*dum);
   }

#ifndef WIN32 // !cj! TODO 
   // go add all the domains that this proxy is responsible for 
   list< pair<Data,Data> > ips = DnsUtil::getInterfaces();
   if ( ips.empty() )
   {
      ErrLog( << "No IP address found to run on" );
   } 
#endif

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
}
