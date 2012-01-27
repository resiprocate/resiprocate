#ifndef TestProxyUser_hxx
#define TestProxyUser_hxx

#include "tfm/TestUser.hxx"

namespace resip
{
class Security;
}

class TestRepro;

class TestProxyUser : public TestUser
{
   public:
      TestProxyUser(TestProxy& proxy, 
                    const resip::Uri& aor,
                    const resip::Data& authName,
                    const resip::Data& password, 
                    resip::TransportType transport,
                    const resip::Data& nwInterface,
                    resip::Security* security,
                     const resip::Uri& outboundProxy = TestSipEndPoint::NoOutboundProxy);
      virtual ~TestProxyUser();
      virtual void clean();
      
   private:
      TestProxy& mProxy;
};

#endif

/* Copyright 2007 Estacado Systems */
