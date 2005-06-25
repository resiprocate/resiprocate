#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include <vector>
#include <list>

#include "rutil/compat.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/Timer.hxx"
#include "rdns/DnsResourceRecord.hxx"
#include "rdns/RRFactory.hxx"
#include "rdns/RRList.hxx"

using namespace resip;
using namespace std;

RRList::RRList() : mRRType(0), mStatus(0), mAbsoluteExpiry(ULONG_MAX) {}

RRList::RRList(const Data& key, 
                const int rrtype, 
                int ttl, 
                int status)
   : mKey(key), mRRType(rrtype), mStatus(status)
{
   mAbsoluteExpiry = ttl + Timer::getTimeMs()/1000;
}

RRList::RRList(const Data& key, int rrtype)
   : mKey(key), mRRType(rrtype), mStatus(0), mAbsoluteExpiry(ULONG_MAX)
{}

RRList::~RRList()
{
   this->clear();
}

RRList::RRList(const RRFactoryBase* factory, 
               const Data& key,
               const int rrType,
               Itr begin,
               Itr end, 
               int ttl)
   : mKey(key), mRRType(rrType), mStatus(0)
{
   update(factory, begin, end, ttl);
}
      
void RRList::update(const RRFactoryBase* factory, Itr begin, Itr end, int ttl)
{
   this->clear();
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
      RecordItem item;
      item.record = factory->create(*it);
      mRecords.push_back(item);
      if ((UInt64)it->ttl() < mAbsoluteExpiry)
      {
         mAbsoluteExpiry = it->ttl();
      }
   }
   mAbsoluteExpiry += Timer::getTimeMs()/1000;
}

RRList::Records RRList::records(const int protocol, bool& allBlacklisted)
{
   Records records;
   allBlacklisted = false;
   if (mRecords.empty()) return records;

   for (std::vector<RecordItem>::iterator it = mRecords.begin(); it != mRecords.end(); ++it)
   {
      if ((*it).states.empty())
      {
         records.push_back((*it).record);
      }
      else if (!(*it).states[protocol].blacklisted)
      {
         records.push_back((*it).record);
      }
   }
   if (records.empty())
   {
      // every record is blacklisted.
      // two options:
      //    1. reset the states and return all the records.
      //    2. get caller to remove the cache and requery.
      // Option 2 is used in this implementation.
      allBlacklisted = true;
   }
   return records;
}

void RRList::blacklist(const int protocol,
                       const DataArr& targets)
{
   for (DataArr::const_iterator it = targets.begin(); it != targets.end(); ++it)
   {
      RecordItr recordItr = find(*it);
      if (recordItr != mRecords.end())
      {
         if ((*recordItr).states.empty())
         {
            initStates((*recordItr).states);
         }
         (*recordItr).states[protocol].blacklisted = true;
      }
   }
}

void RRList::initStates(States& states)
{
   RecordState state;
   state.blacklisted = false;
   for (int i = 0; i < Protocol::Total; ++i)
   {
      states.push_back(state);
   }
}

RRList::RecordItr RRList::find(const Data& value)
{
   for (RecordItr it = mRecords.begin(); it != mRecords.end(); ++it)
   {
      if ((*it).record->isSameValue(value))
      {
         return it;
      }
   }
   return mRecords.end();
}

void RRList::clear()
{
   for (RecordArr::iterator it = mRecords.begin(); it != mRecords.end(); ++it)
   {
      delete (*it).record;
   }
   mRecords.clear();
}
