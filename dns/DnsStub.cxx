#if defined(USE_ARES)
extern "C"
{
#include "ares.h"
#include "ares_dns.h"
}
#endif

#include <set>
#include <vector>

#include "resiprocate/os/socket.hxx"
#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Inserter.hxx"

#include "resiprocate/dns/DnsStub.hxx"
#include "resiprocate/DnsInterface.hxx"

using namespace resip;
using namespace std;

DnsStub::DnsStub(DnsInterface* dns) : mDns(dns)
{
   setupCache();
}

DnsStub::~DnsStub()
{
   for (map<short, RRCacheBase*>::iterator it = mCacheMap.begin(); it != mCacheMap.end(); ++it)
   {
      if (it->first != T_CNAME)
      {
         delete it->second;
      }
   }

   for (set<QueryBase*>::iterator it = mQueries.begin(); it != mQueries.end(); ++it)
   {
      delete *it;
   }
}

void DnsStub::cache(const Data& key,
                    const unsigned char* abuf, 
                    int alen)
{
   // skip header
   const unsigned char* aptr = abuf + HFIXEDSZ;

   int qdcount = DNS_HEADER_QDCOUNT(abuf); // questions.
   for (int i = 0; i < qdcount && aptr; ++i)
   {
      aptr = skipDNSQuestion(aptr, abuf, alen);
   }

   vector<RROverlay> overlays;

   // answers.
   int ancount = DNS_HEADER_ANCOUNT(abuf);
   for (int i = 0; i < ancount; i++)
   {
      aptr = createOverlay(abuf, alen, aptr, overlays);
   }
   
   // name server records.
   int nscount = DNS_HEADER_NSCOUNT(abuf);
   for (int i = 0; i < nscount; i++)
   {
      aptr = createOverlay(abuf, alen, aptr, overlays);
   }

   // additional records.
   int arcount = DNS_HEADER_ARCOUNT(abuf);
   for (int i = 0; i < arcount; i++)
   {
      aptr = createOverlay(abuf, alen, aptr, overlays);
   }

   // sort overlays by type.
   sort(overlays.begin(), overlays.end());

   vector<RROverlay>::iterator itLow = lower_bound(overlays.begin(), overlays.end(), *overlays.begin());
   vector<RROverlay>::iterator itHigh = upper_bound(overlays.begin(), overlays.end(), *overlays.begin());
   while (itLow != overlays.end())
   {
      map<short, RRCacheBase*>::iterator it = mCacheMap.find((*itLow).type());
      if (it != mCacheMap.end())
      {
         it->second->updateCache(key, itLow, itHigh);
      }

      itLow = itHigh;
      if (itHigh != overlays.end())
      {
         itHigh = upper_bound(itLow, overlays.end(), *itLow);
      }
   }   
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
const unsigned char* 
DnsStub::skipDNSQuestion(const unsigned char *aptr,
                         const unsigned char *abuf,
                         int alen)
{
   char *name=0;
   int status=0;
   int len=0;
   
   // Parse the question name. 
   status = ares_expand_name(aptr, abuf, alen, &name, &len);
   if (status != ARES_SUCCESS)
   {
      throw DnsStubException("Failed DNS preparse", __FILE__, __LINE__);
   }
   aptr += len;

   // Make sure there's enough data after the name for the fixed part
   // of the question.
   if (aptr + QFIXEDSZ > abuf + alen)
   {
      free(name);
      throw DnsStubException("Failed DNS preparse", __FILE__, __LINE__);
   }

   aptr += QFIXEDSZ;  
   free(name);
   return aptr;
}

bool
DnsStub::supportedType(int type)
{
   return (T_A == type ||
           T_AAAA == type ||
           T_NAPTR == type ||
           T_SRV == type ||
           T_CNAME == type);
}

const unsigned char*
DnsStub::createOverlay(const unsigned char* abuf,
                       const int alen,
                       const unsigned char* aptr, 
                       vector<RROverlay>& overlays)
{
   const unsigned char* rptr = aptr;
   char* name = 0;
   int len = 0;
   int status = ares_expand_name(aptr, abuf, alen, &name, &len);
   if (ARES_SUCCESS != status)
   {
      throw DnsStubException("Failed overlay creation", __FILE__, __LINE__);
   }
   free(name);
   aptr += len;
   int type = DNS_RR_TYPE(aptr);
   int dlen = DNS_RR_LEN(aptr);
   if (!supportedType(type)) 
   {
      aptr += RRFIXEDSZ;
      aptr += dlen;
      return aptr;
   }
   // rewind before handing it off to overlay.
   aptr -= len;
   RROverlay overlay(aptr, abuf, alen);
   overlays.push_back(overlay);
   return rptr + len + RRFIXEDSZ + dlen;
}

void
DnsStub::setupCache()
{
   mCacheMap.insert(CacheMap::value_type(T_CNAME, &mCnameCache));
}

void
DnsStub::removeQuery(QueryBase* query)
{
   set<QueryBase*>::iterator it = mQueries.find(query);
   if (it != mQueries.end())
   {
      mQueries.erase(it);
   }   
}
