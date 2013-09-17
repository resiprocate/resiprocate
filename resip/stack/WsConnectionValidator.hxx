#ifndef RESIP_WsConnectionValidator_hxx
#define RESIP_WsConnectionValidator_hxx

#include "Cookie.hxx"

namespace resip
{

class WsConnectionValidator
{
   public:
      virtual bool validateConnection(CookieList cookieList)=0;
};

}

#endif

