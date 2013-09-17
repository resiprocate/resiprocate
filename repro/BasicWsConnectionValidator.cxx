
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "repro/BasicWsConnectionValidator.hxx"
#include "resip/stack/Cookie.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/stun/Stun.hxx"
#include "rutil/Logger.hxx"

#include <time.h>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;


BasicWsConnectionValidator::BasicWsConnectionValidator(Data wsCookieAuthSharedSecret)
   : mWsCookieAuthSharedSecret(wsCookieAuthSharedSecret)
{
}

BasicWsConnectionValidator::~BasicWsConnectionValidator()
{
}

bool BasicWsConnectionValidator::validateConnection(resip::CookieList cookieList)
{
   Data wsSessionInfo;
   Data wsSessionExtra;
   Data wsSessionMAC;

   for(CookieList::iterator it = cookieList.begin(); it != cookieList.end(); ++it)
   {
      if((*it).name() == "WSSessionInfo")
      {
         wsSessionInfo = (*it).value();
      }
      else if ((*it).name() == "WSSessionExtra")
      {
         wsSessionExtra = (*it).value();
      }
      else if  ((*it).name() == "WSSessionMAC")
      {
         wsSessionMAC = (*it).value();;
      }
   }

   ParseBuffer pb(wsSessionInfo);
   pb.skipToChar(':');
   pb.skipChar(':');
   time_t expires = (time_t)pb.uInt64();

   if(difftime(time(NULL), expires) < 0)
   {
      WarningLog(<< "Received expired cookie");
      return false;
   }

   Data message = wsSessionInfo + ':' + wsSessionExtra;
   unsigned char hmac[20];
   computeHmac((char*)hmac, message.data(), message.size(), mWsCookieAuthSharedSecret.data(), mWsCookieAuthSharedSecret.size());

   if(memcmp(wsSessionMAC.data(), Data(hmac, 20).hex().data(), 20) != 0)
   {
      WarningLog(<< "Received invalid cookie");
      return false;
   }

   return true;
}
