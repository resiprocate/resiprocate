#if !defined(RESIP_COOKIE_AUTHENTICATOR_HXX)
#define RESIP_COOKIE_AUTHENTICATOR_HXX

#include "rutil/Data.hxx"
#include "repro/Processor.hxx"
#include "resip/stack/Cookie.hxx"
#include "resip/stack/WsCookieContext.hxx"
#include "resip/stack/SipStack.hxx"

using namespace resip;

namespace repro
{
  class CookieAuthenticator : public Processor
  {
    public:

      CookieAuthenticator(const Data& wsCookieAuthSharedSecret, resip::SipStack* stack);
      ~CookieAuthenticator();

      virtual processor_action_t process(RequestContext &);
      virtual void dump(EncodeStream &os) const;

    private:
      bool authorizedForThisIdentity(const WsCookieContext& wsCookieContext, resip::Uri &fromUri, resip::Uri &toUri);
  };

}
#endif
