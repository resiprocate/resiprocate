#ifndef RESIP_WsConnectionBase_hxx
#define RESIP_WsConnectionBase_hxx

#include <resip/stack/WsConnectionValidator.hxx>
#include <resip/stack/Cookie.hxx>
#include <resip/stack/WsCookieContext.hxx>
#include "rutil/Data.hxx"
#include "rutil/SharedPtr.hxx"

#include <vector>

namespace resip
{

class WsConnectionBase
{
   public:
      WsConnectionBase();
      WsConnectionBase(SharedPtr<WsConnectionValidator> mWsConnectionValidator);
      virtual ~WsConnectionBase();

      void setCookies(CookieList& cookies) { mCookies = cookies; };
      void getCookies(CookieList& cookies) const { cookies = mCookies; };
      void getWsCookieContext(WsCookieContext& wsCookieContext) const { wsCookieContext = mWsCookieContext; }
      void setWsCookieContext(const WsCookieContext& wsCookieContext) { mWsCookieContext = wsCookieContext; }
      SharedPtr<WsConnectionValidator> connectionValidator() const;

   private:
      CookieList mCookies;
      WsCookieContext mWsCookieContext;
      SharedPtr<WsConnectionValidator> mWsConnectionValidator;
};

}

#endif

