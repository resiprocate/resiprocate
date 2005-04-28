#ifndef RESIP_RRCACHE_HXX
#define RESIP_RRCACHE_HXX

#include "resiprocate/dns/RRFactory.hxx"
#include "resiprocate/dns/DnsResourceRecord.hxx"
#include "resiprocate/dns/DnsAAAARecord.hxx"
#include "resiprocate/dns/DnsHostRecord.hxx"
#include "resiprocate/dns/DnsNaptrRecord.hxx"
#include "resiprocate/dns/DnsSrvRecord.hxx"
#include "resiprocate/dns/DnsCnameRecord.hxx"

namespace resip
{
class RRList;
class RROverlay;

class RRCache
{
   public:
      typedef RRList::LruList LruListType;
      typedef RRList::Records Result;
      typedef std::vector<RROverlay>::const_iterator Itr;

      RRCache();
      ~RRCache();
      void setTTL(int ttl) { mUserDefinedTTL = ttl; }
      void setSize(int size) { mSize = size; }      
      void updateCache(const Data& target,
                       const int rrType,
                       Itr  begin, 
                       Itr  end);
      void cacheTTL(const Data& target,                    
                    const int rrType,
                    const int status,
                    RROverlay overlay);
      bool lookup(const Data& target, const int type, Result& records, int& status);

   private:
      static const int DEFAULT_SIZE = 512;
      class CompareT  : public std::binary_function<const RRList*, const RRList*, bool>
      {
         public:
            bool operator()(RRList* lhs, RRList* rhs) const
            {
               if (lhs->rrType() < rhs->rrType())
               {
                  return true;
               }
               else if (lhs->rrType() > rhs->rrType())
               {
                  return false;
               }
               else
               {
                  return lhs->key() < rhs->key();
               }
            }
      };

      void touch(RRList* node);
      void cleanup();
      int getTTL(const RROverlay& overlay);
      void purge();

      RRList mHead;
      LruListType* mLruHead;                     
      Result Empty;

      typedef std::set<RRList*, CompareT> RRSet;
      RRSet mRRSet;

      RRFactory<DnsHostRecord> mHostRecordFactory;
      RRFactory<DnsSrvRecord> mSrvRecordFactory;
#ifdef USE_IPV6
      RRFactory<DnsAAAARecord> mAAAARecordFactory;
#endif
      RRFactory<DnsNaptrRecord> mNaptrRecordFacotry;
      RRFactory<DnsCnameRecord> mCnameRecordFactory;

      typedef std::map<int, RRFactoryBase*> FactoryMap;
      FactoryMap  mFactoryMap;

      int mUserDefinedTTL; // in minutes
      unsigned int mSize;
};

}

#endif
