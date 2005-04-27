#ifndef RESIP_RRCACHE_HXX
#define RESIP_RRCACHE_HXX

extern "C"
{
#include "ares.h"
#include "ares_dns.h"
}

#include <map>
#include "resiprocate/os/Data.hxx"
#include "resiprocate/dns/RROverlay.hxx"
#include "resiprocate/dns/RRFactory.hxx"
#include "resiprocate/dns/DnsResourceRecord.hxx"
#include "resiprocate/dns/DnsAAAARecord.hxx"
#include "resiprocate/dns/DnsHostRecord.hxx"
#include "resiprocate/dns/DnsNaptrRecord.hxx"
#include "resiprocate/dns/DnsSrvRecord.hxx"
#include "resiprocate/dns/DnsCnameRecord.hxx"

using namespace std;

namespace resip
{
class RRCache
{
   public:
      typedef RRList::LruList LruListType;
      typedef RRList::Records Result;

      RRCache() :
         mHead(),
         mLruHead(LruListType::makeList(&mHead)),
         mUserDefinedTTL(-1)
      {
         mFactoryMap[T_CNAME] = &mCnameRecordFactory;
         mFactoryMap[T_NAPTR] = &mNaptrRecordFacotry;
         mFactoryMap[T_SRV] = &mSrvRecordFactory;
#ifdef USE_IPV6
         mFactoryMap[T_AAAA] = &mAAAARecordFactory;
#endif
         mFactoryMap[T_A] = &mHostRecordFactory;
      }

      ~RRCache()
      {
         cleanup();
      }

      void setTTL(int ttl) { mUserDefinedTTL = ttl; }
      
      void updateCache(const Data& target,
                       const int rrType,
                       std::vector<RROverlay>::const_iterator begin, 
                       std::vector<RROverlay>::const_iterator end)
      {
         FactoryMap::iterator itf = mFactoryMap.find(rrType);
         assert(itf != mFactoryMap.end());
         MapKey key;
         key.rrType = rrType;
         key.target = target;
         RRMap::iterator itr = mRRMap.find(key);
         if (itr !=  mRRMap.end())
         {
            itr->second->update(itf->second, begin, end, mUserDefinedTTL);
            touch(itr->second);
         }
         else
         {
            RRList* val = new RRList(itf->second, begin, end, mUserDefinedTTL);
            mRRMap[key] = val;
            mLruHead->push_back(val);
         }
      }

      void cacheTTL(const Data& target,
                    const int rrType,
                    RROverlay overlay)
      {
         static int seconds = 60;
         MapKey key;
         key.rrType = rrType;
         key.target = target;
         assert(mRRMap.find(key)==mRRMap.end());
         int ttl = getTTL(overlay);
         if (ttl < 0) return; // not soa.
         if (mUserDefinedTTL > 0)
         {
            ttl = ttl < mUserDefinedTTL? ttl : mUserDefinedTTL;
         }
         RRList* val = new RRList(ttl);
         mRRMap[key] = val;
         mLruHead->push_back(val);
      }

      int getTTL(const RROverlay& overlay)
      {
         // overlay is a soa answer.
         if (overlay.type() != T_SOA) return -1;
         char* name = 0;
         int len = 0;
         ares_expand_name(overlay.data(), overlay.msg(), overlay.msgLength(), &name, &len);
         const unsigned char* pPos = overlay.data() + len;
         free(name);
         ares_expand_name(pPos, overlay.msg(), overlay.msgLength(), &name, &len);
         free(name);
         pPos += len;
         pPos += 16; // skip four 32 bit entities.
         return DNS__32BIT(pPos);         
      }

      Result lookup(const Data& target, const int type)
      {
         MapKey key;
         key.rrType = type;
         key.target = target;
         RRMap::iterator it = mRRMap.find(key);
         if (it == mRRMap.end())
         {
            return Empty;
         }
         else
         {
            if (Timer::getTimeMs()/1000 >= it->second->absoluteExpiry())
            {
               delete it->second;
               mRRMap.erase(it);
               return Empty;
            }
            else
            {
               touch(it->second);
               return it->second->records();
            }
         }
      }

      void touch(RRList* node)
      {
         node->remove();
         mLruHead->push_back(node);
      }


   private:
      typedef struct _MapKey
      {
         int rrType;
         Data target;
         bool operator<(const struct _MapKey& rhs) const
         {
            return rrType < rhs.rrType || target < rhs.target;
         }
      } MapKey;

      void cleanup()
      {
         for (map<MapKey, RRList*>::iterator it = mRRMap.begin(); it != mRRMap.end(); it++)
         {
            delete it->second;
         }
         mRRMap.clear();
      }

      RRList mHead;
      LruListType* mLruHead;
                  
      Result Empty;

      typedef std::map<MapKey, RRList*> RRMap;
      RRMap mRRMap;

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
};

}

#endif
