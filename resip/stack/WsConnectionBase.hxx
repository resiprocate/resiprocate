#ifndef RESIP_WsConnectionBase_hxx
#define RESIP_WsConnectionBase_hxx

#include <list>
#include "rutil/Data.hxx"

namespace resip
{

class WsConnectionBase
{
   public:
      WsConnectionBase();
      virtual ~WsConnectionBase();

      void setCookies(std::list<Data> & cookies) { mCookies = cookies; };
      void getCookies(std::list<Data> & cookies) const { cookies = mCookies; };

   private:
      std::list<Data> mCookies;
};

}

#endif

