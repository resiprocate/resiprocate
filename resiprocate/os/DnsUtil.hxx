#if !defined(RESIP_DNSUTIL_HXX)
#define RESIP_DNSUTIL_HXX

#include "BaseException.hxx"
#include "Data.hxx"
#include <list>

struct in_addr;

namespace resip
{

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

      static std::list<Data> lookupARecords(const Data& host);
      
      static Data getHostByAddr(const Data& ipAddress);
      static Data getLocalHostName();
      static Data getLocalIpAddress();
      static Data getIpAddress(const struct in_addr& addr);
      static bool isIpV4Address(const Data& ipAddress);
};
      
      
}


#endif
