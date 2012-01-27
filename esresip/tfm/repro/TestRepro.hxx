#if !defined(TFM_TestRepro_hxx)
#define TFM_TestRepro_hxx

#include "rutil/KeyValueDbIf.hxx"
#include "repro/KeyValueUserDb.hxx"
#include "repro/KeyValueRegistrationDb.hxx"
#include "repro/KeyValueConfigDb.hxx"
#include "repro/Proxy.hxx"
#include "repro/ProcessorChain.hxx"
#include "repro/ReproServerRegistrationFactory.hxx"
#include "repro/Store.hxx"
#include "resip/ReparteeManager/ReparteeManager.hxx"
#include "resip/ReparteeManager/ReparteeManagerThread.hxx"
#include "resip/ReparteeManager/MasterProfile.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/StackThread.hxx"
#include "rutil/GeneralCongestionManager.hxx"
#include "rutil/SharedPtr.hxx"
#include "tfm/TestProxy.hxx"
#include "tfm/repro/CommandLineParser.hxx"

namespace resip
{
class TransportThread;
}

class TestRepro : public TestProxy
{
   public:
      TestRepro(const resip::Data& name,
                const resip::Data& host, 
                const CommandLineParser& args, 
                const resip::Data& nwInterface = resip::Data::Empty,
                resip::Security* security=0);
      ~TestRepro();

      virtual void addUser(const resip::Data& userid, const resip::Uri& aor, const resip::Data& password, bool provideFailover=false);
      virtual void deleteUser(const resip::Data& userid, const resip::Uri& aor);
      virtual void deleteBindings(const resip::Uri& aor);
      virtual void addRoute(const resip::Data& matchingPattern,
                            const resip::Data& rewriteExpression, 
                            const resip::Data& method,
                            const resip::Data& event,
                            int priority,
                            int weight);
      virtual void deleteRoute(const resip::Data& matchingPattern, 
                               const resip::Data& method, 
                               const resip::Data& event);
      virtual bool addTrustedHost(const resip::Data& host, resip::TransportType transport, short port = 0);
      // no current support in AclStore to remove entry (AclStore::buildKey is private).
      // void deleteTrustedHost(const resip::Data& host, resip::TransportType transport, short port = 0);

   private:
      resip::SipStack mStack;
      resip::StackThread mStackThread;
      
      resip::SharedPtr<resip::MasterProfile> mProfile;
      repro::KeyValueConfigDb mConfigs;
      repro::NetStore mNetStore;
      repro::MiscStore mMiscStore;
      repro::Store mStore;
      repro::KeyValueRegistrationDb mRegData;
      repro::ProcessorChain mRequestProcessors;
      repro::ProcessorChain mResponseProcessors;
      repro::ProcessorChain mTargetProcessors;
      repro::Proxy mProxy;
      resip::ReparteeManager mReparteeManager;
      resip::ReparteeManagerThread mReparteeManagerThread;
      resip::SharedPtr<repro::ReproServerRegistrationFactory> mRegFactory;
      resip::GeneralCongestionManager mCongestionManager;
      std::vector<resip::TransportThread*> mTransportThreads;
};

#endif

/* Copyright 2007 Estacado Systems */
