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

      typedef enum 
      {
         Unknown = 0,
         UDP,
         TCP,
         TLS,
         SCTP,
         DCCP,
         MAX_TRANSPORT
      } Type;

      struct Srv
      {
            int priority;
            int weight;
            int port;
            Data host;
            Type transport;

            bool operator<(const Srv& rhs) const
            {
               return priority < rhs.priority;
            }
      };
      static std::list<DnsUtil::Srv> lookupSRVRecords(const Data& host);
      
      static Data getHostByAddr(const Data& ipAddress);
      static Data getLocalHostName();
      static Data getLocalDomainName();
      static Data getLocalIpAddress();
      static Data getIpAddress(const struct in_addr& addr);
      static bool isIpAddress(const Data& ipAddress);
      static bool isIpV4Address(const Data& ipAddress);
      static bool isIpV6Address(const Data& ipAddress);
};


}


#endif
