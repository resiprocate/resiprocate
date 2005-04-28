#ifndef RESIP_DNS_RR_LIST
#define RESIP_DNS_RR_LIST

#include "resiprocate/os/IntrusiveListElement.hxx"
#include "resiprocate/dns/RRFactory.hxx"

namespace resip
{
class DnsResourceRecord;

class RRList : public IntrusiveListElement<RRList*>
{
   public:      
      typedef std::vector<DnsResourceRecord*> Records;
      typedef IntrusiveListElement<RRList*> LruList;
      typedef std::vector<RROverlay>::const_iterator Itr;

      RRList();
      explicit RRList(const Data& key, const int rrtype, int ttl, int status);
      explicit RRList(const Data& key, int rrtype);
      ~RRList();
      RRList(const RRFactoryBase* factory, 
             const Data& key,
             const int rrType,
             Itr begin,
             Itr end, 
             int ttl);
      
      void update(const RRFactoryBase* factory, Itr begin, Itr end, int ttl);
      const Records& records() const { return mRecords; }
      const Data& key() const { return mKey; }
      int status() const { return mStatus; }
      int rrType() const { return mRRType; }
      UInt64 absoluteExpiry() const { return mAbsoluteExpiry; }
      UInt64& absoluteExpiry() { return mAbsoluteExpiry; }

   private:
      static const int MIN_TO_SEC = 60;
      Data mKey;
      int mRRType;
      UInt64 mAbsoluteExpiry;
      Records mRecords;
      int mStatus; // dns query status.

};

};


#endif
