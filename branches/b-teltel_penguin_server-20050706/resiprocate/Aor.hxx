#if !defined(RESIP_AOR_HXX)
#define RESIP_AOR_HXX 

#include <iosfwd>
#include "resiprocate/os/Data.hxx"

namespace resip
{

class Uri;

class Aor
{
   public:
      explicit Aor(const Data& value);
      Aor();
      Aor(const Uri& uri);
      Aor(const Aor& aor);
      Aor& operator=(const Aor& aor);
      
      bool operator==(const Aor& other) const;
      bool operator!=(const Aor& other) const;
      bool operator<(const Aor& other) const;

      const Data& value() const;

      Data& scheme();
      const Data& scheme() const;

      Data& host();
      const Data& host() const;

      Data& user();
      const Data& user() const;

      int& port();
      int port() const;
      
      std::ostream& operator<<(std::ostream& str) const;

   private:
      mutable Data mValue;

      mutable Data mOldScheme;
      mutable Data mOldUser;
      mutable Data mOldHost;
      mutable int mOldPort;

      // cache for IPV6 host comparison
      mutable Data mCanonicalHost;

      Data mScheme;
      Data mUser;
      Data mHost;
      int mPort;
};
      
}

#endif
