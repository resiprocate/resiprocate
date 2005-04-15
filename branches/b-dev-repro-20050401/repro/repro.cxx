#include "resiprocate/MessageFilterRule.hxx"
#include "resiprocate/Security.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/StackThread.hxx"
#include "resiprocate/dum/DumThread.hxx"
#include "resiprocate/dum/InMemoryRegistrationDatabase.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/Log.hxx"
#include "resiprocate/os/Logger.hxx"

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
   tu.addDomain(DnsUtil::getLocalHostName());
   list<Data> ips = DnsUtil::getLocalIpAddress();
   tu.addDomain("127.0.0.1");
   tu.addDomain("localhost");
   for ( list<Data>::const_iterator i=ips.begin(); i!=ips.end(); i++)
   {
      DebugLog( << "Adding domain for IP " << *i  );
      tu.addDomain(*i);
   }
   for (std::vector<Uri>::const_iterator i=args.mDomains.begin(); 
        i != args.mDomains.end(); ++i)
   {
      InfoLog (<< "Adding domain " << i->host() << " " << i->port());
      tu.addDomain(i->host());
   }
}

int
main(int argc, char** argv)
{
   /* Initialize a stack */
   CommandLineParser args(argc, argv);
   Log::initialize(args.mLogType, args.mLogLevel, argv[0]);

   Security security(args.mCertPath);
   SipStack stack(&security);

   try
   {
      if (args.mUseV4) InfoLog (<< "V4 enabled");
      if (args.mUseV6) InfoLog (<< "V6 enabled");         

      if (args.mUdpPort)
      {
         if (args.mUseV4) stack.addTransport(UDP, args.mUdpPort, V4);
         if (args.mUseV6) stack.addTransport(UDP, args.mUdpPort, V6);
      }
      if (args.mTcpPort)
      {
         if (args.mUseV4) stack.addTransport(TCP, args.mUdpPort, V4);
         if (args.mUseV6) stack.addTransport(TCP, args.mUdpPort, V6);
      }
      if (args.mTlsPort)
      {
         if (args.mUseV4) stack.addTransport(TLS, args.mTlsPort, V4, Data::Empty, args.mTlsDomain);
         if (args.mUseV6) stack.addTransport(TLS, args.mTlsPort, V6, Data::Empty, args.mTlsDomain);
      }
      if (args.mDtlsPort)
      {
         if (args.mUseV4) stack.addTransport(DTLS, args.mTlsPort, V4, Data::Empty, args.mTlsDomain);
         if (args.mUseV6) stack.addTransport(DTLS, args.mTlsPort, V6, Data::Empty, args.mTlsDomain);
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
   
   WebAdmin admin(userDb, regData, routeDb, security);
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
