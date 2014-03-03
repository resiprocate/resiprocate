#if !defined(TFM_TestRepro_hxx)
#define TFM_TestRepro_hxx

#include "repro/AbstractDb.hxx"
#include "repro/Proxy.hxx"
#include "repro/ProxyConfig.hxx"
#include "repro/Registrar.hxx"
#include "repro/ProcessorChain.hxx"
#include "repro/Store.hxx"
#include "repro/Dispatcher.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/DumThread.hxx"
#include "resip/dum/InMemoryRegistrationDatabase.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/EventStackThread.hxx"
#include "rutil/FdPoll.hxx"
#include "rutil/CongestionManager.hxx"
#include "rutil/SharedPtr.hxx"
#include "tfm/TestProxy.hxx"
#include "tfm/repro/CommandLineParser.hxx"

#include <memory>

class TfmProxyConfig : public repro::ProxyConfig
{
public:
   TfmProxyConfig(repro::AbstractDb* db, const CommandLineParser& args);
};

class TestRepro : public TestProxy
{
   public:
      TestRepro(const resip::Data& name,
                const resip::Data& host, 
                const CommandLineParser& args, 
                const resip::Data& nwInterface = resip::Data::Empty,
                resip::Security* security=0);
      ~TestRepro();

      virtual void addUser(const resip::Data& userid, const resip::Uri& aor, const resip::Data& password);
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
      virtual bool addTrustedHost(const resip::Data& host, resip::TransportType transport, short port = 0, short mask = 0, short family=resip::V4);
      virtual void deleteTrustedHost(const resip::Data& host, resip::TransportType transport, short port = 0, short mask = 0, short family=resip::V4);

   private:
      resip::FdPollGrp* mPollGrp;
      resip::EventThreadInterruptor* mInterruptor;
      resip::SipStack* mStack;
      resip::EventStackThread* mStackThread;
      
      repro::Registrar mRegistrar;
      resip::SharedPtr<resip::MasterProfile> mProfile;
      repro::AbstractDb* mDb;
      TfmProxyConfig mConfig;
      repro::Dispatcher* mAuthRequestDispatcher;
      repro::ProcessorChain mRequestProcessors;
      repro::ProcessorChain mResponseProcessors;
      repro::ProcessorChain mTargetProcessors;
      resip::InMemoryRegistrationDatabase mRegData;
      repro::Proxy mProxy;
      resip::DialogUsageManager* mDum;
      resip::DumThread* mDumThread;
      std::auto_ptr<resip::CongestionManager> mCongestionManager;
};

#endif
