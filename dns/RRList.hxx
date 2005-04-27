#ifndef RESIP_DNS_RR_LIST
#define RESIP_DNS_RR_LIST

#include <vector>

#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/Timer.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/IntrusiveListElement.hxx"
#include "resiprocate/dns/DnsResourceRecord.hxx"
#include "resiprocate/dns/RRFactory.hxx"


namespace resip
{
class RRList : public IntrusiveListElement<RRList*>
{
   public:      
      typedef std::vector<DnsResourceRecord*> Records;
      typedef IntrusiveListElement<RRList*> LruList;
      typedef std::vector<RROverlay>::const_iterator Itr;


      RRList() : mStatus(0), mRRType(0), mAbsoluteExpiry(ULONG_MAX) {}

      explicit RRList(const Data& key, const int rrtype, int ttl, int status)
         : mKey(key), mStatus(status), mRRType(rrtype)
      {
         mAbsoluteExpiry = ttl + Timer::getTimeMs()/1000;
      }

      explicit RRList(const Data& key, int rrtype)
         : mKey(key), mRRType(rrtype), mAbsoluteExpiry(ULONG_MAX)
      {}

      ~RRList()
      {
         for (Records::iterator it = mRecords.begin(); it != mRecords.end(); ++it)
         {
            delete (*it);
         }
         mRecords.clear();
      }

      RRList(const RRFactoryBase* factory, 
             const Data& key,
             const int rrType,
             Itr begin,
             Itr end, 
             int ttl)
             : mKey(key), mRRType(rrType)
      {
         update(factory, begin, end, ttl);
      }
      
      void update(const RRFactoryBase* factory, Itr begin, Itr end, int ttl)
      {
         mRecords.clear();
         if (ttl >= 0)
         {
            mAbsoluteExpiry = ttl * MIN_TO_SEC;
         }
         else
         {
            mAbsoluteExpiry = ULONG_MAX;
         }
         for (Itr it = begin; it != end; it++)
         {
            mRecords.push_back(factory->create(*it));
            if ((UInt64)it->ttl() < mAbsoluteExpiry)
            {
               mAbsoluteExpiry = it->ttl();
            }
         }
         mAbsoluteExpiry += Timer::getTimeMs()/1000;
      }

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
