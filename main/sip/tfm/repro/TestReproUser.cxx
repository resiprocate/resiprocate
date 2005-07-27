#include "tfm/repro/TestRepro.hxx"
#include "tfm/repro/TestReproUser.hxx"

using namespace resip;
using namespace repro;

TestReproUser::TestReproUser(TestProxy& proxy, 
                             const resip::Uri& aor,
                             const resip::Data& authName,
                             const resip::Data& password, 
                             resip::TransportType transport,
                             const resip::Uri& outboundProxy,
                             const resip::Data& interface) : 
   TestUser(aor, authName, password, transport, outboundProxy, interface),
   mProxy(proxy)
{
   mProxy.addUser(mAuthName, aor, password);
}

TestReproUser::~TestReproUser()
{
   mProxy.deleteUser(mAuthName, mAor);
}
