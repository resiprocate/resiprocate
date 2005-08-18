#ifndef RESIP_RRCACHE_HXX
#define RESIP_RRCACHE_HXX

#include <map>
#include <set>

#include "resiprocate/dns/RRFactory.hxx"
#include "resiprocate/dns/DnsResourceRecord.hxx"
#include "resiprocate/dns/DnsAAAARecord.hxx"
#include "resiprocate/dns/DnsHostRecord.hxx"
#include "resiprocate/dns/DnsNaptrRecord.hxx"
#include "resiprocate/dns/DnsSrvRecord.hxx"
#include "resiprocate/dns/DnsCnameRecord.hxx"
#include "resiprocate/dns/RRList.hxx"

namespace resip
{
class RROverlay;

class RRCache
{
   public:
      typedef RRList::Protocol Protocol;
      typedef RRList::LruList LruListType;
      typedef RRList::Records Result;
      typedef std::vector<RROverlay>::const_iterator Itr;
      typedef std::vector<Data> DataArr;

      static RRCache* instance();
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
      bool lookup(const Data& target, int type, int proto, Result& records, int& status);
      void blacklist(const Data& target, int rrType, int protocol, const DataArr& targetsToBlacklist);

   private:
      RRCache();
      static const int DEFAULT_SIZE = 512;
      static std::auto_ptr<RRCache> mInstance;
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

      typedef std::map<int, resip::RRFactoryBase*> FactoryMap;
      FactoryMap  mFactoryMap;
      
      int mUserDefinedTTL; // in minutes
      unsigned int mSize;
};

}

#endif
