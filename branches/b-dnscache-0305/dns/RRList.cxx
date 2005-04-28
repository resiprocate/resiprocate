#include <vector>

#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/os/Timer.hxx"
#include "resiprocate/dns/DnsResourceRecord.hxx"
#include "resiprocate/dns/RRFactory.hxx"
#include "resiprocate/dns/RRList.hxx"

using namespace resip;
using namespace std;

RRList::RRList() : mStatus(0), mRRType(0), mAbsoluteExpiry(ULONG_MAX) {}

RRList::RRList(const Data& key, 
                const int rrtype, 
                int ttl, 
                int status)
   : mKey(key), mStatus(status), mRRType(rrtype)
{
   mAbsoluteExpiry = ttl + Timer::getTimeMs()/1000;
}

RRList::RRList(const Data& key, int rrtype)
   : mKey(key), mRRType(rrtype), mAbsoluteExpiry(ULONG_MAX)
{}

RRList::~RRList()
{
   for (Records::iterator it = mRecords.begin(); it != mRecords.end(); ++it)
   {
      delete (*it);
   }
   mRecords.clear();
}

RRList::RRList(const RRFactoryBase* factory, 
               const Data& key,
               const int rrType,
               Itr begin,
               Itr end, 
               int ttl)
   : mKey(key), mRRType(rrType)
{
   update(factory, begin, end, ttl);
}
      
void RRList::update(const RRFactoryBase* factory, Itr begin, Itr end, int ttl)
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
