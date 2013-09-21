
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "repro/BasicWsConnectionValidator.hxx"
#include "resip/stack/Cookie.hxx"
#include "resip/stack/WsCookieContext.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/stun/Stun.hxx"
#include "rutil/Logger.hxx"

#include <time.h>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;


BasicWsConnectionValidator::BasicWsConnectionValidator(const Data& wsCookieAuthSharedSecret)
   : mWsCookieAuthSharedSecret(wsCookieAuthSharedSecret)
{
}

BasicWsConnectionValidator::~BasicWsConnectionValidator()
{
}

bool BasicWsConnectionValidator::validateConnection(const resip::WsCookieContext& wsCookieContext)
{
   Data message = wsCookieContext.getWsSessionInfo() + ':' + wsCookieContext.getWsSessionExtra();
   unsigned char hmac[20];
   computeHmac((char*)hmac, message.data(), message.size(), mWsCookieAuthSharedSecret.data(), mWsCookieAuthSharedSecret.size());

   if(memcmp(wsCookieContext.getWsSessionMAC().data(), Data(hmac, 20).hex().data(), 20) != 0)
   {
      WarningLog(<< "Cookie MAC validation failed");
      return false;
   }

   if(difftime(time(NULL), wsCookieContext.getExpiresTime()) < 0)
   {
      WarningLog(<< "Received expired cookie");
      return false;
   }

   return true;
}
