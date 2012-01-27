#include "tfm/repro/TestRepro.hxx"
#include "tfm/repro/TestReproUser.hxx"

using namespace resip;
using namespace repro;

TestReproUser::TestReproUser(TestProxy& proxy, 
                             const resip::Uri& aor,
                             const resip::Data& authName,
                             const resip::Data& password, 
                             bool provideFailover,
                             resip::TransportType transport,
                             const resip::Uri& outboundProxy,
                             const resip::Data& nwInterface,
                             resip::Security* security) : 
   TestUser(aor, authName, password, transport, outboundProxy, nwInterface, security),
   mProxy(proxy)
{
   mProxy.addUser(mAuthName, aor, password,provideFailover);
}

TestReproUser::~TestReproUser()
{
}

void
TestReproUser::clean()
{
   TestUser::clean();
   mProxy.deleteBindings(mAor);
}

/* Copyright 2007 Estacado Systems */
