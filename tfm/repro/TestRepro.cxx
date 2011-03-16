#include "repro/BerkeleyDb.hxx"
#include "repro/ReproServerAuthManager.hxx"
#include "repro/monkeys/AmIResponsible.hxx"
#include "repro/monkeys/DigestAuthenticator.hxx"
#include "repro/monkeys/IsTrustedNode.hxx"
#include "repro/monkeys/LocationServer.hxx"
#include "repro/monkeys/StaticRoute.hxx"
#include "repro/monkeys/StrictRouteFixup.hxx"
#include "repro/monkeys/OutboundTargetHandler.hxx"
#include "repro/monkeys/QValueTargetHandler.hxx"
#include "repro/monkeys/SimpleTargetHandler.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "tfm/repro/TestRepro.hxx"

#ifdef USE_SSL
#include "resip/stack/ssl/Security.hxx"
#endif

using namespace resip;
using namespace repro;



#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

static ProcessorChain&  
makeRequestProcessorChain(ProcessorChain& chain, 
                          Store& store,
                          RegistrationPersistenceManager& regData,
                          SipStack* stack)
{
   // Either the chainName is default or we don't know about it
   // Use default if we don't recognize the name
   // Should log about it.
   ProcessorChain* locators = new ProcessorChain();
   
   ProcessorChain* authenticators = new ProcessorChain();
   
   IsTrustedNode* isTrusted = new IsTrustedNode(store.mAclStore);
   authenticators->addProcessor(std::auto_ptr<Processor>(isTrusted));

   DigestAuthenticator* da = new DigestAuthenticator(store.mUserStore,
                                                      stack,
                                                      false,
                                                      "localhost",
                                                      5080,
                                                      true,
                                                      false);
   authenticators->addProcessor(std::auto_ptr<Processor>(da)); 

   StrictRouteFixup* srf = new StrictRouteFixup;
   locators->addProcessor(std::auto_ptr<Processor>(srf));

   AmIResponsible* isme = new AmIResponsible;
   locators->addProcessor(std::auto_ptr<Processor>(isme));
      
   StaticRoute* sr = new StaticRoute(store.mRouteStore,true, true, true);
   locators->addProcessor(std::auto_ptr<Processor>(sr));
 
   LocationServer* ls = new LocationServer(regData, true);
   locators->addProcessor(std::auto_ptr<Processor>(ls));
 
   chain.addProcessor(std::auto_ptr<Processor>(authenticators));
   chain.addProcessor(std::auto_ptr<Processor>(locators));
   chain.setChainType(Processor::REQUEST_CHAIN);
   return chain;
}

static ProcessorChain&  
makeResponseProcessorChain(ProcessorChain& chain,
                          RegistrationPersistenceManager& regData) 
{
   ProcessorChain* lemurs = new ProcessorChain;

   OutboundTargetHandler* ob = new OutboundTargetHandler(regData);
   lemurs->addProcessor(std::auto_ptr<Processor>(ob));

   chain.addProcessor(std::auto_ptr<Processor>(lemurs));
   chain.setChainType(Processor::RESPONSE_CHAIN);
   return chain;
}

static ProcessorChain&  
makeTargetProcessorChain(ProcessorChain& chain,const CommandLineParser& args) 
{
   ProcessorChain* baboons = new ProcessorChain;

   QValueTargetHandler* qval = 
      new QValueTargetHandler(QValueTargetHandler::EQUAL_Q_PARALLEL,
                              true, //Cancel btw fork groups?
                              true, //Wait for termination btw fork groups?
                              2000, //ms between fork groups, moot in this case
                              2000 //ms before cancel
                              );
   baboons->addProcessor(std::auto_ptr<Processor>(qval));
   
   SimpleTargetHandler* smpl = new SimpleTargetHandler;
   baboons->addProcessor(std::auto_ptr<Processor>(smpl));
   
   chain.addProcessor(std::auto_ptr<Processor>(baboons));
   chain.setChainType(Processor::TARGET_CHAIN);
   return chain;
}


static Uri  
makeUri(const resip::Data& domain, int port)
{
   Uri uri;
   uri.host() = domain;
   if (port != 5060)
   {
      uri.port() = port;
   }
   return uri;
}


TestRepro::TestRepro(const resip::Data& name,
                     const resip::Data& host, 
                     const CommandLineParser& args, 
                     const resip::Data& nwInterface,
                     Security* security) : 
   TestProxy(name, host, args.mUdpPorts, args.mTcpPorts, args.mTlsPorts, args.mDtlsPorts, nwInterface),
#ifdef USE_SIGCOMP
   mStack(security,
            DnsStub::EmptyNameserverList,
            0,
            false,
            0,
            new Compression(Compression::DEFLATE)),
#else
   mStack(security),
#endif
   mStackThread(mStack),
   mRegistrar(),
   mProfile(new MasterProfile),
   mDb(new BerkeleyDb),
   mStore(*mDb),
   mRequestProcessors(),
   mRegData(),
   mProxy(mStack, 
          makeUri(host, *args.mUdpPorts.begin()),
          args.mForceRecordRoute, //<- Enable record-route
          makeRequestProcessorChain(mRequestProcessors, mStore, mRegData,&mStack),
          makeResponseProcessorChain(mResponseProcessors,mRegData),
          makeTargetProcessorChain(mTargetProcessors,args),
          mStore.mUserStore,
          180),
   mDum(mStack),
   mDumThread(mDum)
{
   resip::InteropHelper::setRRTokenHackEnabled(args.mEnableFlowTokenHack);
   resip::InteropHelper::setOutboundSupported(true);
   resip::InteropHelper::setOutboundVersion(5626); // RFC 5626

   mProxy.addDomain("localhost");

   // !bwc! TODO Once we have something we _do_ support, put that here.
   mProxy.addSupportedOption("p-fakeoption");
   mStack.addAlias("localhost",5060);
   mStack.addAlias("localhost",5061);

   std::list<resip::Data> domains;
   domains.push_back("127.0.0.1");
   domains.push_back("localhost");
   
   try
   {
      mStack.addTransport(UDP, 
                           5060, 
                           V4,
                           StunDisabled,
                           nwInterface,
                           resip::Data::Empty,
                           resip::Data::Empty,
                           resip::SecurityTypes::TLSv1,
                           0);
   }
   catch(...)
   {}

   try
   {
      mStack.addTransport(TCP, 
                           5060, 
                           V4,
                           StunDisabled,
                           nwInterface,
                           resip::Data::Empty,
                           resip::Data::Empty,
                           resip::SecurityTypes::TLSv1,
                           0);
   }
   catch(...)
   {}

#ifdef RESIP_USE_SCTP
   try
   {
      mStack.addTransport(SCTP, 
                           5060, 
                           V4,
                           StunDisabled,
                           "0.0.0.0",// multihomed
                           resip::Data::Empty,
                           resip::Data::Empty,
                           resip::SecurityTypes::TLSv1,
                           0);
   }
   catch(...)
   {}
#endif

#ifdef USE_SSL
   std::list<resip::Data> localhost;
   localhost.push_back("localhost");
   
   try
   {
      mStack.addTransport(TLS, 
                           5061, 
                           V4, 
                           StunDisabled, 
                           nwInterface, 
                           "localhost",
                           resip::Data::Empty,
                           resip::SecurityTypes::TLSv1,
                           0);
   }
   catch(...)
   {}
#endif
   mProxy.addDomain(host);
   
   std::vector<Data> enumSuffixes;
   enumSuffixes.push_back(args.mEnumSuffix);
   mStack.setEnumSuffixes(enumSuffixes);

   mProxy.addSupportedOption("outbound");
   mProxy.addSupportedOption("p-fakeoption");

   mProfile->clearSupportedMethods();
   mProfile->addSupportedMethod(resip::REGISTER);
   mProfile->addSupportedScheme(Symbols::Sips);

   mDum.setMasterProfile(mProfile);
   mDum.setServerRegistrationHandler(&mRegistrar);
   mDum.setRegistrationPersistenceManager(&mRegData);
   mDum.addDomain(host);
   
   // Install rules so that the registrar only gets REGISTERs
   resip::MessageFilterRule::MethodList methodList;
   methodList.push_back(resip::REGISTER);

   resip::MessageFilterRuleList ruleList;
   ruleList.push_back(MessageFilterRule(resip::MessageFilterRule::SchemeList(),
                                        resip::MessageFilterRule::Any,
                                        methodList) );
   mDum.setMessageFilterRuleList(ruleList);
    
   SharedPtr<ServerAuthManager> authMgr(new ReproServerAuthManager(mDum, mStore.mUserStore, mStore.mAclStore, true, false));
   mDum.setServerAuthManager(authMgr);    

   mStack.registerTransactionUser(mProxy);

   mStackThread.run();
   mProxy.run();
   mDumThread.run();
}

TestRepro::~TestRepro()
{
   mDumThread.shutdown();
   mDumThread.join();
   mStackThread.shutdown();
   mStackThread.join();
}

void
TestRepro::addUser(const Data& userid, const Uri& aor, const Data& password)
{
   InfoLog (<< "Repro::addUser: " << userid << " " << aor);
   mStore.mUserStore.addUser(userid,aor.host(),aor.host(), password, true, Data::from(aor), Data::from(aor));
}

void
TestRepro::deleteUser(const Data& userid, const Uri& aor)
{
   //InfoLog (<< "Repro::delUser: " << userid);
   mStore.mUserStore.eraseUser(userid);
   mRegData.removeAor(aor);
}

void
TestRepro::deleteBindings(const Uri& aor)
{
   //InfoLog (<< "Repro::delBindings: " << aor);
   mRegData.removeAor(aor);
}

void 
TestRepro::addRoute(const resip::Data& matchingPattern,
                    const resip::Data& rewriteExpression, 
                    const resip::Data& method,
                    const resip::Data& event,
                    int priority,
                    int weight) 
{
   mStore.mRouteStore.addRoute(method, event, matchingPattern, rewriteExpression, priority);
}

void 
TestRepro::deleteRoute(const resip::Data& matchingPattern, 
                       const resip::Data& method, 
                       const resip::Data& event)
{
   mStore.mRouteStore.eraseRoute(method, event, matchingPattern);
}

bool
TestRepro::addTrustedHost(const resip::Data& host, resip::TransportType transport, short port)
{
   return mStore.mAclStore.addAcl(host, port, static_cast<const short&>(transport));
}

