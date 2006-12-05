#ifndef TestReproUser_hxx
#define TestReproUser_hxx

#include "tfm/TestUser.hxx"

namespace resip
{
class Security;
}

class TestRepro;

class TestReproUser : public TestUser
{
   public:
      TestReproUser(TestProxy& proxy, 
                    const resip::Uri& aor,
                    const resip::Data& authName,
                    const resip::Data& password, 
                    resip::TransportType transport = resip::UDP,
                    const resip::Uri& outboundProxy = TestSipEndPoint::NoOutboundProxy,
                    const resip::Data& nwInterface = resip::Data::Empty,
                    resip::Security* security=0);
      virtual ~TestReproUser();
      virtual void clean();
      
   private:
      TestProxy& mProxy;
};

#endif
