#include "repro/BerkeleyDb.hxx"
#include "repro/ReproServerAuthManager.hxx"
#include "repro/monkeys/AmIResponsible.hxx"
#include "repro/monkeys/DigestAuthenticator.hxx"
#include "repro/monkeys/IsTrustedNode.hxx"
#include "repro/monkeys/LocationServer.hxx"
#include "repro/monkeys/StaticRoute.hxx"
#include "repro/monkeys/StrictRouteFixup.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/Security.hxx"
#include "tfm/repro/TestRepro.hxx"

using namespace resip;
using namespace repro;



#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

static ProcessorChain&  
makeRequestProcessorChain(ProcessorChain& chain, 
                          RouteStore& store, 
                          RegistrationPersistenceManager& regData)
{
   // Either the chainName is default or we don't know about it
   // Use default if we don't recognize the name
   // Should log about it.
   ProcessorChain* locators = new ProcessorChain();
   
   StrictRouteFixup* srf = new StrictRouteFixup;
   locators->addProcessor(std::auto_ptr<Processor>(srf));

   IsTrustedNode* isTrusted = new IsTrustedNode;
   locators->addProcessor(std::auto_ptr<Processor>(isTrusted));

   DigestAuthenticator* da = new DigestAuthenticator;
   locators->addProcessor(std::auto_ptr<Processor>(da)); 

   AmIResponsible* isme = new AmIResponsible;
   locators->addProcessor(std::auto_ptr<Processor>(isme));
      
   StaticRoute* sr = new StaticRoute(store);
   locators->addProcessor(std::auto_ptr<Processor>(sr));
 
   LocationServer* ls = new LocationServer(regData);
   locators->addProcessor(std::auto_ptr<Processor>(ls));
 
   chain.addProcessor(std::auto_ptr<Processor>(locators));
   return chain;
}

static ProcessorChain&  
makeResponseProcessorChain(ProcessorChain& chain) 
{
   return chain;
}

static ProcessorChain&  
makeTargetProcessorChain(ProcessorChain& chain) 
{
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
                     int port, 
                     const resip::Data& interface,
                     Security* security) : 
   TestProxy(name, host, port, interface),
   mStack(security),
   mStackThread(mStack),
   mRegistrar(),
   mProfile(new MasterProfile),
   mDb(new BerkeleyDb),
   mStore(*mDb),
   mRequestProcessors(),
   mRegData(),
   mProxy(mStack, 
          makeUri(host, port),
          makeRequestProcessorChain(mRequestProcessors, mStore.mRouteStore, mRegData),
          makeResponseProcessorChain(mResponseProcessors),
          makeTargetProcessorChain(mTargetProcessors),
          mStore.mUserStore),
   mDum(mStack),
   mDumThread(mDum)
{
   mStack.addTransport(UDP, port, V4);
   //mStack.addTransport(TCP, port, V4);
   mStack.addTransport(TLS, port, V4, StunDisabled, Data::Empty, host );
   mProxy.addDomain(host);
   
   mProfile->clearSupportedMethods();
   mProfile->addSupportedMethod(resip::REGISTER);
   //mProfile->addSupportedScheme(Symbols::Sips);

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
    
   SharedPtr<ServerAuthManager> authMgr(new ReproServerAuthManager(mDum, mStore.mUserStore ));
   mDum.setServerAuthManager(authMgr);    

   mStack.registerTransactionUser(mProxy);

   mStackThread.run();
   mProxy.run();
   mDumThread.run();
}

TestRepro::~TestRepro()
{
}

void
TestRepro::addUser(const Data& userid, const Uri& aor, const Data& password)
{
   InfoLog (<< "Repro::addUser: " << userid << " " << aor);
   mStore.mUserStore.addUser(userid,aor.host(),aor.host(), password, Data::from(aor), Data::from(aor));
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

