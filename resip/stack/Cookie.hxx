#if !defined(RESIP_COOKIE_HXX)
#define RESIP_COOKIE_HXX


#include "rutil/Data.hxx"

#include <vector>

namespace resip
{

class Cookie
{
   public:

      Cookie();
      Cookie(const Data& name, const Data& value);

      Cookie& operator=(const Cookie&);
      bool operator==(const Cookie& other) const;
      bool operator<(const Cookie& rhs) const;
      friend EncodeStream& operator<<(EncodeStream& strm, const Cookie& c);

      const Data& name() const;
      Data& name();

      const Data& value() const;
      Data& value();

   private:
      Data mName;
      Data mValue;
};

typedef std::vector<Cookie> CookieList;

}

#endif
