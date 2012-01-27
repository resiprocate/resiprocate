#include "rutil/SqliteKVDb.hxx"
#include "repro/ReproServerAuthManager.hxx"
#include "repro/ReproServerRegistrationFactory.hxx"
#include "repro/monkeys/AmIResponsible.hxx"
#include "repro/monkeys/DigestAuthenticator.hxx"
#include "repro/monkeys/IsTrustedNode.hxx"
#include "repro/monkeys/LocationServer.hxx"
#include "repro/monkeys/StaticRoute.hxx"
#include "repro/monkeys/StrictRouteFixup.hxx"
#include "repro/monkeys/RecursiveRedirect.hxx"
#include "repro/monkeys/OutboundTargetHandler.hxx"
#include "repro/monkeys/QValueTargetHandler.hxx"
#include "repro/monkeys/SimpleTargetHandler.hxx"
#include "repro/monkeys/ProvisionedRouteMonkey.hxx"
#include "repro/monkeys/RedirectServer.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "resip/stack/KeyValueTransportDb.hxx"
#include "resip/stack/TransportThread.hxx"
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
   
   IsTrustedNode* isTrusted = new IsTrustedNode(store.mAclStore,store.mMiscStore);
   authenticators->addProcessor(std::auto_ptr<Processor>(isTrusted));

   DigestAuthenticator* da = new DigestAuthenticator();
   authenticators->addProcessor(std::auto_ptr<Processor>(da)); 

   StrictRouteFixup* srf = new StrictRouteFixup;
   locators->addProcessor(std::auto_ptr<Processor>(srf));

   AmIResponsible* isme = new AmIResponsible;
   locators->addProcessor(std::auto_ptr<Processor>(isme));
      
   StaticRoute* sr = new StaticRoute(store.mRouteStore);
   locators->addProcessor(std::auto_ptr<Processor>(sr));
 
   ProvisionedRouteMonkey* proute = new ProvisionedRouteMonkey();
   locators->addProcessor(std::auto_ptr<Processor>(proute));
   
   LocationServer* ls = new LocationServer(regData,false);
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
   chain.addProcessor(std::auto_ptr<Processor>(new OutboundTargetHandler(regData)));
   RecursiveRedirect* redir = new RecursiveRedirect();
   chain.addProcessor(std::auto_ptr<Processor>(redir));
   chain.setChainType(Processor::RESPONSE_CHAIN);
   return chain;
}

static ProcessorChain&  
makeTargetProcessorChain(ProcessorChain& chain,const CommandLineParser& args) 
{
   ProcessorChain* baboons = new ProcessorChain;

   if(args.mRedirectServer)
   {
      RedirectServer* rds = new RedirectServer();
      baboons->addProcessor(std::auto_ptr<Processor>(rds));
   }
   else
   {
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
   }
   
   chain.addProcessor(std::auto_ptr<Processor>(baboons));
   chain.setChainType(Processor::TARGET_CHAIN);
   
   return chain;   
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
   mProfile(new MasterProfile),
   mConfigs(std::auto_ptr<KeyValueDbIf>(new SqliteKVDb("repro_acls.db")),
            std::auto_ptr<KeyValueDbIf>(new SqliteKVDb("repro_routes.db"))
           ),
   mNetStore(new resip::KeyValueTransportDb(std::auto_ptr<KeyValueDbIf>(new SqliteKVDb("repro_interfaces.db")))),
   mMiscStore(std::auto_ptr<KeyValueDbIf>(new SqliteKVDb("repro_misc.db"))),
   mStore(&mConfigs,
         &mNetStore,
         &mMiscStore,
         new KeyValueUserDb(
               std::auto_ptr<KeyValueDbIf>(new SqliteKVDb("repro_users.db") )),
         new KeyValueUserDb(
               std::auto_ptr<KeyValueDbIf>(new SqliteKVDb("repro_admins.db") ))
         ),
   mRegData(std::auto_ptr<KeyValueDbIf>(new SqliteKVDb("repro_registrations.db"))),
   mRequestProcessors(),
   mProxy(mStack, 
          true,
          makeRequestProcessorChain(mRequestProcessors, mStore, mRegData,&mStack),
          makeResponseProcessorChain(mResponseProcessors, mRegData),
          makeTargetProcessorChain(mTargetProcessors,args),
          mStore.mUserStore,
          mStore.mNetStore,
          180),
   mReparteeManager(mStack),
   mReparteeManagerThread(mReparteeManager)
{
   mStack.setFilloutContacts(false);

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
      resip::Transport* t= mStack.addTransport(UDP, 
                                                5060, 
                                                V4,
                                                StunDisabled,
                                                nwInterface,
                                                resip::Data::Empty,
                                                resip::Data::Empty,
                                                resip::SecurityTypes::TLSv1,
                                                0,
                                                true);
      mStore.mNetStore.setNetRecord(nwInterface,5060,resip::UDP,
                                    domains,true,true);
      if(t)
      {
         mTransportThreads.push_back(new resip::TransportThread(*t));
      }
   }
   catch(...)
   {}

   try
   {
      resip::Transport* t= mStack.addTransport(TCP, 
                                                5060, 
                                                V4,
                                                StunDisabled,
                                                nwInterface,
                                                resip::Data::Empty,
                                                resip::Data::Empty,
                                                resip::SecurityTypes::TLSv1,
                                                0,
                                                true);
      mStore.mNetStore.setNetRecord(nwInterface,5060,resip::TCP,
                                    domains,true,true);
      if(t)
      {
         mTransportThreads.push_back(new resip::TransportThread(*t));
      }
   }
   catch(...)
   {}

#ifdef RESIP_USE_SCTP
   try
   {
      resip::Transport* t= mStack.addTransport(SCTP, 
                                                5060, 
                                                V4,
                                                StunDisabled,
                                                "0.0.0.0",// multihomed
                                                resip::Data::Empty,
                                                resip::Data::Empty,
                                                resip::SecurityTypes::TLSv1,
                                                0,
                                                true);
      mStore.mNetStore.setNetRecord(resip::Data::Empty,5060,resip::SCTP,
                                    domains,true,true);
      if(t)
      {
         mTransportThreads.push_back(new resip::TransportThread(*t));
      }
   }
   catch(...)
   {}
#endif

#ifdef USE_SSL
   std::list<resip::Data> localhost;
   localhost.push_back("localhost");
   
   try
   {
      resip::Transport* t= mStack.addTransport(TLS, 
                                                5061, 
                                                V4, 
                                                StunDisabled, 
                                                nwInterface, 
                                                "localhost",
                                                resip::Data::Empty,
                                                resip::SecurityTypes::TLSv1,
                                                0,
                                                true);
      mStore.mNetStore.setNetRecord(nwInterface,5061,resip::TLS,localhost,true,true);
      if(t)
      {
         mTransportThreads.push_back(new resip::TransportThread(*t));
      }
   }
   catch(...)
   {}
#endif

   
   std::vector<Data> enumSuffixes;
   if (!mStore.mMiscStore.getString("enumSuffix").empty())
   {
      enumSuffixes.push_back(mStore.mMiscStore.getString("enumSuffix"));
      mStack.setEnumSuffixes(enumSuffixes);
   }

   mProfile->clearSupportedMethods();
   mProfile->addSupportedMethod(resip::REGISTER);
   //mProfile->addSupportedScheme(Symbols::Sips);

   mReparteeManager.setMasterProfile(mProfile);

   mRegFactory.reset(new ReproServerRegistrationFactory(mReparteeManager, mRegData));

   mReparteeManager.setServerRegistrationFactory(mRegFactory);
   mReparteeManager.addDomain(host);
   
   // Install rules so that the registrar only gets REGISTERs
   resip::MessageFilterRule::MethodList methodList;
   methodList.push_back(resip::REGISTER);

   resip::MessageFilterRuleList ruleList;
   ruleList.push_back(MessageFilterRule(resip::MessageFilterRule::SchemeList(),
                                        resip::MessageFilterRule::Any,
                                        methodList) );
   mReparteeManager.setMessageFilterRuleList(ruleList);
    
   std::list<resip::Feature*> incomingFeatures;
   incomingFeatures.push_back(new ReproServerAuthManager(mReparteeManager,
                                                      mStore.mUserStore,
                                                      mStore.mAclStore,
                                                      true /* useAuthInt */ ));
   mReparteeManager.setIncomingFeatures(incomingFeatures);

   mStack.registerTransactionUser(mProxy);
   mStack.setCongestionManager(&mCongestionManager);

   for(std::vector<resip::TransportThread*>::iterator t=mTransportThreads.begin(); t!=mTransportThreads.end(); ++t)
   {
      (*t)->run();
   }

   mStackThread.run();
   mStack.run();
   mProxy.run();
   mReparteeManagerThread.run();
}

TestRepro::~TestRepro()
{
   mReparteeManagerThread.shutdown();
   mProxy.shutdown();
   mStackThread.shutdown();
   for(std::vector<resip::TransportThread*>::iterator t=mTransportThreads.begin(); t!=mTransportThreads.end(); ++t)
   {
      (*t)->shutdown();
   }
   mRegFactory->shutdown();
   mReparteeManagerThread.join();
   mProxy.join();
   mStackThread.join();
   for(std::vector<resip::TransportThread*>::iterator t=mTransportThreads.begin(); t!=mTransportThreads.end(); ++t)
   {
      (*t)->join();
      delete *t;
   }
   mRegFactory->join();
   mStack.setCongestionManager(0);
}

void
TestRepro::addUser(const Data& userid, const Uri& aor, const Data& password, bool provideFailover)
{
   InfoLog (<< "Repro::addUser: " << userid << " " << aor);
   if(provideFailover)
   {
      mStore.mUserStore.addUser(userid,aor.host(),aor.host(), password,true, Data::from(aor), Data::from(aor),Data::Empty,"sip:voicemail@localhost");      
   }
   else
   {
      mStore.mUserStore.addUser(userid,aor.host(),aor.host(), password, true,Data::from(aor), Data::from(aor));
   }
}

void
TestRepro::deleteUser(const Data& userid, const Uri& aor)
{
   InfoLog (<< "Repro::delUser: " << userid);
   mStore.mUserStore.eraseUser(userid,aor.host());
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
   mStore.mRouteStore.addRoute(method, event, matchingPattern, rewriteExpression, std::vector<resip::NameAddr>(),priority,true);
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


/* Copyright 2007 Estacado Systems */
