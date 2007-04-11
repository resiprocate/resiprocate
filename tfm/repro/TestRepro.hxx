#if !defined(TFM_TestRepro_hxx)
#define TFM_TestRepro_hxx

#include "repro/AbstractDb.hxx"
#include "repro/Proxy.hxx"
#include "repro/Registrar.hxx"
#include "repro/ProcessorChain.hxx"
#include "repro/Store.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/DumThread.hxx"
#include "resip/dum/InMemoryRegistrationDatabase.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/StackThread.hxx"
#include "rutil/SharedPtr.hxx"
#include "tfm/TestProxy.hxx"

class TestRepro : public TestProxy
{
   public:
      TestRepro(const resip::Data& name,
                const resip::Data& host, 
                int port, 
                const resip::Data& nwInterface = resip::Data::Empty,
                bool forceRecordRoute=false,
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
      
   private:
      resip::SipStack mStack;
      resip::StackThread mStackThread;
      
      repro::Registrar mRegistrar;
      resip::SharedPtr<resip::MasterProfile> mProfile;
      repro::AbstractDb* mDb;
      repro::Store mStore;
      repro::ProcessorChain mRequestProcessors;
      repro::ProcessorChain mResponseProcessors;
      repro::ProcessorChain mTargetProcessors;
      resip::InMemoryRegistrationDatabase mRegData;
      repro::Proxy mProxy;
      resip::DialogUsageManager mDum;
      resip::DumThread mDumThread;
};

#endif
