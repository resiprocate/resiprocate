#include "resip/stack/WsCookieContext.hxx"
#include "rutil/Data.hxx"

using namespace resip;

WsCookieContext::WsCookieContext()
{
}

WsCookieContext:: WsCookieContext(const CookieList& cookieList)
{
   for (CookieList::const_iterator it = cookieList.begin(); it != cookieList.end(); ++it)
   {
      if ((*it).name() == "WSSessionInfo")
      {
         mWsSessionInfo = (*it).value();
      }
      else if ((*it).name() == "WSSessionExtra")
      {
         mWsSessionExtra = (*it).value();
      }
      else if ((*it).name() == "WSSessionMAC")
      {
         mWsSessionMAC = (*it).value();
      }
   }

   ParseBuffer pb(mWsSessionInfo);
   pb.skipToChar(':');
   pb.skipChar(':');
   mExpiresTime = (time_t) pb.uInt64();

   const char* anchor;
   Data uriString;

   pb.skipToChar(':');
   pb.skipChar(':');
   anchor = pb.position();
   pb.skipToChar(':');
   pb.data(uriString, anchor);
   mWsFromUri = Uri("sip:" + uriString);

   pb.skipChar(':');
   anchor = pb.position();
   pb.skipToChar(':');
   pb.data(uriString, anchor);
   mWsDestUri = Uri("sip:" + uriString);
}

WsCookieContext::~WsCookieContext()
{
}
