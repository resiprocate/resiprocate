#if !defined(TFM_TestRepro_hxx)
#define TFM_TestRepro_hxx

#include "repro/AbstractDb.hxx"
#include "repro/Proxy.hxx"
#include "repro/Registrar.hxx"
#include "repro/RequestProcessorChain.hxx"
#include "repro/Store.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/StackThread.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/DumThread.hxx"
#include "resiprocate/dum/InMemoryRegistrationDatabase.hxx"
#include "resiprocate/dum/MasterProfile.hxx"
#include "resiprocate/os/SharedPtr.hxx"
#include "tfm/TestProxy.hxx"

class TestRepro : public TestProxy
{
   public:
      TestRepro(const resip::Data& name,
                const resip::Data& host, 
                int port, 
                const resip::Data& interface = resip::Data::Empty);
      ~TestRepro();

      virtual void addUser(const resip::Data& userid, const resip::Uri& aor, const resip::Data& password);
      virtual void deleteUser(const resip::Data& userid, const resip::Uri& aor);
      
   private:
      resip::SipStack mStack;
      resip::StackThread mStackThread;
      
      repro::Registrar mRegistrar;
      resip::SharedPtr<resip::MasterProfile> mProfile;
      repro::AbstractDb* mDb;
      repro::Store mStore;
      repro::RequestProcessorChain mRequestProcessors;
      resip::InMemoryRegistrationDatabase mRegData;
      repro::Proxy mProxy;
      resip::DialogUsageManager mDum;
      resip::DumThread mDumThread;
};

#endif
