#if !defined (RESIP_DNS_HANDLER_HXX)
#define RESIP_DNS_HANDLER_HXX

namespace resip
{

class DnsResult;
class TransactionState;

class DnsHandler
{
   public:
      virtual ~DnsHandler()=0;
      
      // call when dns entries are available (or nothing found)
      // this may be called synchronously with the call to lookup
      virtual void handle(DnsResult* result)=0;
};
 
}


#endif
