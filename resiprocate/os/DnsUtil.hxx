#if !defined(RESIP_DNSUTIL_HXX)
#define RESIP_DNSUTIL_HXX

#include <list>
#include "BaseException.hxx"
#include "Data.hxx"


struct in_addr;

namespace resip
{

class Tuple;

class DnsUtil
{
   public:
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg,
                      const Data& file,
                      const int line)
               : BaseException(msg, file, line) {}            
         protected:
            virtual const char* name() const { return "DnsUtil::Exception"; }
      };

      static Data getLocalHostName();
      static Data getLocalDomainName();
      
      // wrappers for the not so ubiquitous inet_pton, inet_ntop (e.g. WIN32)
      static Data inet_ntop(const struct in_addr& addr);
      static Data inet_ntop(const struct in6_addr& addr);
      static Data inet_ntop(const Tuple& tuple);

      static int inet_pton(const Data& printableIp, struct in_addr& dst);
      static int inet_pton(const Data& printableIp, struct in6_addr& dst);

      static bool isIpAddress(const Data& ipAddress);
      static bool isIpV4Address(const Data& ipAddress);
      static bool isIpV6Address(const Data& ipAddress);

      // returns pair of interface name, ip address
      static std::list<std::pair<Data,Data> > getInterfaces();

      // XXXX:0:0:0:YYYY:192.168.2.233 => XXXX::::YYYY:192.168.2.233
      // so string (case) comparison will work
      // or something
      static void canonicalizeIpV6Address(Data& ipV6Address);
};

}


#endif
