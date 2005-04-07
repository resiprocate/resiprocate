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
#include "repro/stateAgents/CertServer.hxx"


#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

using namespace repro;
using namespace resip;
using namespace std;

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
      if (args.mUdpPort)
      {
         stack.addTransport(UDP, args.mUdpPort);
      }
      if (args.mTcpPort)
      {
         stack.addTransport(TCP, args.mTcpPort);
         stack.addTransport(TCP, args.mTcpPort-1);
      }
      if (args.mTlsPort)
      {
         stack.addTransport(TLS, args.mTlsPort, V4, Data::Empty, args.mTlsDomain);
      }
      if (args.mDtlsPort)
      {
         stack.addTransport(DTLS, args.mTlsPort, V4, Data::Empty, args.mTlsDomain);
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
      
      AmIResponsible* isme = new AmIResponsible;
      locators->addProcessor(std::auto_ptr<RequestProcessor>(isme));
      
      // [TODO] !rwm! put Gruu monkey here
      
      // [TODO] !rwm! put Tel URI monkey here 
      
#if 0
      // TODO - remove next forwards all to 
      ConstantLocationMonkey* cls = new ConstantLocationMonkey;
      locators->addProcessor(std::auto_ptr<RequestProcessor>(cls));
#endif
     
#if 1
      RouteMonkey* routeMonkey = new RouteMonkey(routeDb);
      locators->addProcessor(std::auto_ptr<RequestProcessor>(routeMonkey));
#endif
 
      LocationServer* ls = new LocationServer(regData);
      locators->addProcessor(std::auto_ptr<RequestProcessor>(ls));
 
      requestProcessors.addProcessor(auto_ptr<RequestProcessor>(locators));
      
      if (!args.mNoChallenge)
      {
         DigestAuthenticator* da = new DigestAuthenticator();
         requestProcessors.addProcessor(std::auto_ptr<RequestProcessor>(da)); 
      }
   }
   
   UserDb userDb;
   
   Proxy proxy(stack, requestProcessors, userDb);
   
   proxy.addDomain(DnsUtil::getLocalHostName());
   proxy.addDomain(DnsUtil::getLocalHostName(), 5060);

// TODO fix next line 
//   Data foo = DnsUtil::getLocalIpAddress();
#if !defined( __APPLE__ ) 
   // TODO - this fails on mac  
   proxy.addDomain(DnsUtil::getLocalIpAddress());
   proxy.addDomain(DnsUtil::getLocalIpAddress(), 5060);
#endif

   for (std::vector<Uri>::const_iterator i=args.mDomains.begin(); 
        i != args.mDomains.end(); ++i)
   {
      //InfoLog (<< "Adding domain " << i->host() << " " << i->port());
      proxy.addDomain(i->host(), i->port());
   }
   
   WebAdmin admin(userDb, regData, routeDb, security);
   WebAdminThread adminThread(admin);

   profile.clearSupportedMethods();
   profile.addSupportedMethod(resip::REGISTER);
   profile.addSupportedScheme(Symbols::Sips);
   
   DialogUsageManager* dum = 0;
   DumThread* dumThread = 0;
   CertServer* certServer = 0;

   resip::MessageFilterRuleList ruleList;
   if (!args.mNoRegistrar || args.mCertServer)
   {
      dum = new DialogUsageManager(stack);
      dum->setMasterProfile(&profile);
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
      certServer = new CertServer(*dum);

      // Install rules so that the registrar only gets REGISTERs
      resip::MessageFilterRule::MethodList methodList;
      methodList.push_back(resip::SUBSCRIBE);
      methodList.push_back(resip::PUBLISH);
      ruleList.push_back(MessageFilterRule(resip::MessageFilterRule::SchemeList(),
                                           resip::MessageFilterRule::Any,
                                           methodList) );
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
