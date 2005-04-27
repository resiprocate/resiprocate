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


      RRList() {}

      ~RRList()
      {
         for (Records::iterator it = mRecords.begin(); it != mRecords.end(); ++it)
         {
            delete (*it);
         }
         mRecords.clear();
      }

      template<typename Iter> RRList(const RRFactoryBase* factory, typename Iter begin, typename Iter end, int ttl)
      {
         update(factory, begin, end, ttl);
      }

      RRList(int ttl)
      {
         mAbsoluteExpiry = ttl;
      }
      
      template<class Iter> 
      void update(const RRFactoryBase* factory, typename Iter begin, typename Iter end, int ttl)
      {
         mRecords.clear();
         if (ttl >= 0)
         {
            mAbsoluteExpiry = ttl * 60; // convert from minutes to seconds.
         }
         else
         {
            mAbsoluteExpiry = ULONG_MAX;
         }
         for (Iter it = begin; it != end; it++)
         {
            mRecords.push_back(factory->create(*it));
            if ((unsigned long)it->ttl() < mAbsoluteExpiry)
            {
               mAbsoluteExpiry = it->ttl();
            }
         }
         mAbsoluteExpiry += Timer::getTimeMs()/1000;
      }

      const Records& records() const
      {
         return mRecords;
      }

      UInt64 absoluteExpiry() const { return mAbsoluteExpiry; }
      UInt64& absoluteExpiry() { return mAbsoluteExpiry; }
   protected:

   private:
      UInt64 mAbsoluteExpiry;
      Records mRecords;

};

};


#endif
