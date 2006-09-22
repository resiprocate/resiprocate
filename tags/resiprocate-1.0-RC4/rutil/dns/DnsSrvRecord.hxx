#ifndef RESIP_DNS_SRV_RECORD
#define RESIP_DNS_SRV_RECORD

#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/dns/DnsResourceRecord.hxx"

namespace resip
{

class Data;
class DnsResourceRecord;
class RROverlay;

class DnsSrvRecord : public DnsResourceRecord
{
   public:
      class SrvException : public BaseException
      {
         public:
            SrvException(const Data& msg, const Data& file, const int line)
               : BaseException(msg, file, line) 
            {
            }
            
            const char* name() const { return "SrvException"; }
      };

      DnsSrvRecord(const RROverlay&);
      ~DnsSrvRecord() {}

      // accessors.
      int priority() const { return mPriority; }
      int& priority() { return mPriority; }
      int weight() const { return mWeight; }
      int port() const { return mPort; }
      const Data& target() const { return mTarget; }
      const Data& name() const { return mName; }
      bool isSameValue(const Data& value) const;
      
   private:
      int mPriority;
      int mWeight;
      int mPort;
      Data mTarget; // domain name of the target host.
      Data mName;
};

}


#endif
