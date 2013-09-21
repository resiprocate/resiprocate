#ifndef RESIP_WsConnectionValidator_hxx
#define RESIP_WsConnectionValidator_hxx

#include "WsCookieContext.hxx"

namespace resip
{

class WsConnectionValidator
{
   public:
      virtual bool validateConnection(const WsCookieContext& wsCookieContext)=0;
};

}

#endif

