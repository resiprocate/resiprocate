#ifndef RESIP_WsCookieContext_hxx
#define RESIP_WsCookieContext_hxx

#include "Cookie.hxx"
#include "rutil/Data.hxx"
#include "Uri.hxx"

namespace resip
{

class WsCookieContext
{
   public:
      WsCookieContext();
      WsCookieContext(const CookieList& cookieList);
      ~WsCookieContext();

      Data getWsSessionInfo() const { return mWsSessionInfo; };
      Data getWsSessionExtra() const { return mWsSessionExtra; };
      Data getWsSessionMAC() const { return mWsSessionMAC; };
      Uri getWsFromUri() const { return mWsFromUri; };
      Uri getWsDestUri() const { return mWsDestUri; };
      time_t getExpiresTime() const { return mExpiresTime; };

   private:
      Data mWsSessionInfo;
      Data mWsSessionExtra;
      Data mWsSessionMAC;
      Uri mWsFromUri;
      Uri mWsDestUri;
      time_t mExpiresTime;
};

}

#endif

