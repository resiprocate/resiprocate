#if !defined(REPRO_BASICWSCONNECTIONVALIDATOR_HXX)
#define REPRO_BASICWSCONNECTIONVALIDATOR_HXX

#include "resip/stack/WsConnectionValidator.hxx"
#include "resip/stack/Cookie.hxx"
#include "rutil/Data.hxx"

using namespace resip;

namespace repro
{

class BasicWsConnectionValidator: public WsConnectionValidator
{
   public:

      BasicWsConnectionValidator(const Data& wsCookieAuthSharedSecret);
      virtual ~BasicWsConnectionValidator();

      virtual bool validateConnection(resip::CookieList cookieList);

   private:
      Data mWsCookieAuthSharedSecret;

};

}
#endif

