#ifndef RESIP_DNS_RR_LIST
#define RESIP_DNS_RR_LIST

#include <vector>

#include "rutil/IntrusiveListElement.hxx"
#include "rutil/dns/RRFactory.hxx"

namespace resip
{
class DnsResourceRecord;
class DnsHostRecord;

class RRList : public IntrusiveListElement<RRList*>
{
   public:

      class Protocol
      {
         public:
            static const int Reserved = 0;
            static const int Sip = 1;
            static const int Stun = 2;
            static const int Http = 3;
            static const int Enum = 4;
      };

      typedef std::vector<DnsResourceRecord*> Records;
      typedef IntrusiveListElement<RRList*> LruList;
      typedef std::vector<RROverlay>::const_iterator Itr;
      typedef std::vector<Data> DataArr;

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
      
      RRList(const DnsHostRecord &record, int ttl);
      void update(const DnsHostRecord &record, int ttl);

      void update(const RRFactoryBase* factory, Itr begin, Itr end, int ttl);
      Records records(const int protocol);

      const Data& key() const { return mKey; }
      int status() const { return mStatus; }
      int rrType() const { return mRRType; }
      UInt64 absoluteExpiry() const { return mAbsoluteExpiry; }
      UInt64& absoluteExpiry() { return mAbsoluteExpiry; }
      void log();

   private:

      struct RecordItem
      {
            DnsResourceRecord* record;
            std::vector<int> blacklistedProtocols;
      };

      typedef std::vector<RecordItem> RecordArr;
      typedef RecordArr::iterator RecordItr;

      RecordArr mRecords;

      Data mKey;
      int mRRType;

      int mStatus; // dns query status.
      UInt64 mAbsoluteExpiry;

      RecordItr find(const Data&);
      void clear();
};

}

#endif
