#include "tfm/remoteproxy/TestRemoteProxy.hxx"
#include "tfm/remoteproxy/TestProxyUser.hxx"

using namespace resip;

TestProxyUser::TestProxyUser(TestProxy& proxy, 
                             const resip::Uri& aor,
                             const resip::Data& authName,
                             const resip::Data& password, 
                             resip::TransportType transport,
                             const resip::Data& nwInterface,
                             resip::Security* security,
                            const resip::Uri& outboundProxy) : 
   TestUser(aor, authName, password, transport, outboundProxy, nwInterface, security),
   mProxy(proxy)
{
   mProxy.addUser(mAuthName, aor, password);
}

TestProxyUser::~TestProxyUser()
{
   clean();
}

void
TestProxyUser::clean()
{
   TestUser::clean();
}

/* Copyright 2007 Estacado Systems */
