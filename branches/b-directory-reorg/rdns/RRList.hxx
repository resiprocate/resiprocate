#ifndef RESIP_DNS_RR_LIST
#define RESIP_DNS_RR_LIST

#include "rutil/IntrusiveListElement.hxx"
#include "rdns/RRFactory.hxx"

namespace resip
{
class DnsResourceRecord;

class RRList : public IntrusiveListElement<RRList*>
{
   public:

      class Protocol
      {
      public:
         static const int Sip = 0;
         static const int Stun = 1;
         static const int Http = 2;
         static const int Total = 3; // number of protocols.
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
      
      void update(const RRFactoryBase* factory, Itr begin, Itr end, int ttl);
      Records records(const int protocol, bool& allBlacklisted);
      void blacklist(const int protocol, const DataArr& targetsToBlacklist);

      const Data& key() const { return mKey; }
      int status() const { return mStatus; }
      int rrType() const { return mRRType; }
      UInt64 absoluteExpiry() const { return mAbsoluteExpiry; }
      UInt64& absoluteExpiry() { return mAbsoluteExpiry; }

   private:
      static const int MIN_TO_SEC = 60;
      struct RecordState
      {
            bool blacklisted;
      };
      typedef std::vector<RecordState> States;
      struct RecordItem
      {
            DnsResourceRecord* record;
            States states; // indexed by protocol.
      };

      typedef std::vector<RecordItem> RecordArr;
      typedef RecordArr::iterator RecordItr;

      RecordArr mRecords;

      Data mKey;
      int mRRType;

      int mStatus; // dns query status.
      UInt64 mAbsoluteExpiry;

      void initStates(States& states);
      RecordItr find(const Data&);
      void clear();
};

}


#endif
