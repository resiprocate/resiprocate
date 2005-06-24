#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#if defined(USE_ARES)
extern "C"
{
#include "ares.h"
#include "ares_dns.h"
}
#endif

#include <set>
#include <vector>
#include <cassert>

#ifndef WIN32
#ifndef __CYGWIN__
#include <arpa/nameser.h>
#endif
#endif

#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Inserter.hxx"
#include "resiprocate/dns/DnsStub.hxx"
#include "resiprocate/DnsInterface.hxx"

using namespace resip;
using namespace std;

DnsStub::DnsResourceRecordsByPtr DnsStub::Query::Empty;

DnsStub::DnsStub(DnsInterface* dns) : mDns(dns), mTransform(0)
{
}

DnsStub::~DnsStub()
{
   for (set<Query*>::iterator it = mQueries.begin(); it != mQueries.end(); ++it)
   {
      delete *it;
   }
}

void DnsStub::process()
{
   while (mCommandFifo.messageAvailable())
   {
      Command* command = mCommandFifo.getNext();
      command->doIt();
      delete command;
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
      aptr = createOverlay(abuf, alen, aptr, overlays, true);
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
      RRCache::instance()->updateCache(key, (*itLow).type(), itLow, itHigh);
      itLow = itHigh;
      if (itHigh != overlays.end())
      {
         itHigh = upper_bound(itLow, overlays.end(), *itLow);
      }
   }   
}

void DnsStub::cacheTTL(const Data& key,
                       int rrType,
                       int status,
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
   if (ancount != 0) return;
   
   // name server records.
   int nscount = DNS_HEADER_NSCOUNT(abuf);
   if (nscount == 0) return;
   vector<RROverlay> soa;
   aptr = createOverlay(abuf, alen, aptr, soa);
   RRCache::instance()->cacheTTL(key, rrType, status, soa[0]);
}

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
           T_CNAME == type ||
           T_SOA == type);
}

const unsigned char*
DnsStub::createOverlay(const unsigned char* abuf,
                       const int alen,
                       const unsigned char* aptr, 
                       vector<RROverlay>& overlays,
                       bool discard)
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
   if (!discard)
   {
      RROverlay overlay(aptr, abuf, alen);
      overlays.push_back(overlay);
   }
   return rptr + len + RRFIXEDSZ + dlen;
}

void
DnsStub::removeQuery(Query* query)
{
   set<Query*>::iterator it = mQueries.find(query);
   if (it != mQueries.end())
   {
      mQueries.erase(it);
   }
}

void DnsStub::doBlacklisting(const Data& target,
                             int rrType, 
                             int protocol, 
                             const DataArr& targetsToBlacklist)
{
   RRCache::instance()->blacklist(target, rrType, protocol, targetsToBlacklist);
   ListenerMap::iterator it = mListenerMap.find(rrType);
   if (it != mListenerMap.end())
   {
      for (DataArr::const_iterator itB = targetsToBlacklist.begin(); itB != targetsToBlacklist.end(); ++itB)
      {  
         for (Listeners::iterator itL = (*it).second.begin(); itL != (*it).second.end(); ++itL)
         {
            (*itL)->onBlacklisted(rrType, *itB);
         }
      }
   }
}

void DnsStub::setResultTransform(ResultTransform* transform)
{
   mTransform = transform;
}

void DnsStub::removeResultTransform()
{
   mTransform = 0;
}

void DnsStub::registerBlacklistListener(int rrType, BlacklistListener* listener)
{
   ListenerMap::iterator it = mListenerMap.find(rrType);
   if (it == mListenerMap.end())
   {
      Listeners lst;
      lst.push_back(listener);
      mListenerMap.insert(ListenerMap::value_type(rrType, lst));
   }
   else
   {
      for (Listeners::iterator itr = it->second.begin(); itr != it->second.end(); ++itr)
      {
         if ((*itr) == listener) return;
      }
      it->second.push_back(listener);
   }
}

void DnsStub::unregisterBlacklistListener(int rrType, BlacklistListener* listener)
{
   ListenerMap::iterator it = mListenerMap.find(rrType);
   if (it != mListenerMap.end())
   {
      for (Listeners::iterator itr = it->second.begin(); itr != it->second.end(); ++itr)
      {
         if ((*itr) == listener)
         {
            it->second.erase(itr);
            break;
         }
      }
   }
}

DnsStub::Query::Query(DnsStub& stub, ResultTransform* transform, ResultConverter* resultConv, 
                      const Data& target, int rrType, 
                      bool followCname, int proto, DnsResultSink* s)
   : mRRType(rrType),
     mStub(stub), 
     mTransform(transform),
     mResultConverter(resultConv),
     mTarget(target),
     mProto(proto),
     mReQuery(0),
     mSink(s),
     mDns(0),
     mFollowCname(followCname)
{
   assert(s);               
}

DnsStub::Query::~Query() 
{
   delete mResultConverter; //.dcm. flyweight?
}


void 
DnsStub::Query::go(DnsInterface* dns)
{
   mDns = dns;
   assert(mDns!=0);
   DnsResourceRecordsByPtr records;
   int status = 0;
   bool cached = false;
   Data targetToQuery = mTarget;
   cached = RRCache::instance()->lookup(mTarget, mRRType, mProto, records, status);
   if (!cached)
   {
      if (mRRType != T_CNAME)
      {
         DnsResourceRecordsByPtr cnames;
         cached = RRCache::instance()->lookup(mTarget, T_CNAME, mProto, cnames, status);
         if (cached && !cnames.empty()) 
         {
            targetToQuery = (dynamic_cast<DnsCnameRecord*>(cnames[0]))->cname();
            // check the cache.
            cached = RRCache::instance()->lookup(targetToQuery, mRRType, mProto, records, status);
         }
      }
   }

   if (!cached)
   {
      mDns->lookupRecords(targetToQuery, mRRType, this);
   }
   else
   {
      if (mTransform && !records.empty())
      {
         mTransform->transform(targetToQuery, mRRType, records);
      }
      mResultConverter->notifyUser(mTarget, status, mDns->errorMessage(status), records, mSink); 
      mStub.removeQuery(this);
      delete this;
   }
}

void 
DnsStub::Query::process(int status, const unsigned char* abuf, const int alen)
{
   if (status != 0)
   {
      if (status == 4 || status == 1) // domain name not found or no answer.
      {
         mStub.cacheTTL(mTarget, mRRType, status, abuf, alen);
      }
      mResultConverter->notifyUser(mTarget, status, mDns->errorMessage(status), Empty, mSink);
      mReQuery = 0;
      mStub.removeQuery(this);
      delete this;
      return;
   }

   bool bDeleteThis = true;

   // skip header
   const unsigned char* aptr = abuf + HFIXEDSZ;

   int qdcount = DNS_HEADER_QDCOUNT(abuf); // questions.
   for (int i = 0; i < qdcount && aptr; ++i)
   {
      aptr = mStub.skipDNSQuestion(aptr, abuf, alen);
   }

   int ancount = DNS_HEADER_ANCOUNT(abuf);
   if (ancount == 0)
   {
      mResultConverter->notifyUser(mTarget, 0, mDns->errorMessage(0), Empty, mSink); 
   }
   else
   {
      bool bGotAnswers = true;
      if (ancount == 1)
      {
         followCname(aptr, abuf, alen, bGotAnswers, bDeleteThis);
      }

      if (bGotAnswers)
      {
         mStub.cache(mTarget, abuf, alen);
         mReQuery = 0;
         int status = 0;
         Data targetToQuery = mTarget;
         DnsResourceRecordsByPtr result;
         DnsResourceRecordsByPtr cnames;
         RRCache::instance()->lookup(mTarget, T_CNAME, mProto, cnames, status);
         if (!cnames.empty()) targetToQuery = (dynamic_cast<DnsCnameRecord*>(cnames[0]))->cname();
         RRCache::instance()->lookup(targetToQuery, mRRType, mProto, result, status);
         if (mTransform) mTransform->transform(targetToQuery, mRRType, result);
         mResultConverter->notifyUser(mTarget, status, mDns->errorMessage(status), result, mSink);
      }
   }
               
   if (bDeleteThis) 
   {
      mStub.removeQuery(this);
      delete this;
   }
}

void 
DnsStub::Query::onDnsRaw(int status, const unsigned char* abuf, int alen)
{
   process(status, abuf, alen);
}

void 
DnsStub::Query::followCname(const unsigned char* aptr, const unsigned char*abuf, const int alen, bool& bGotAnswers, bool& bDeleteThis)
{
   bGotAnswers = true;
   bDeleteThis = true;
   if (mRRType != T_CNAME)
   {
      char* name = 0;
      int len = 0;
      ares_expand_name(aptr, abuf, alen, &name, &len);
      aptr += len;
      free(name);

      if (DNS_RR_TYPE(aptr) == T_CNAME)
      {
         if (mFollowCname && mReQuery < MAX_REQUERIES)
         {
            mStub.cache(mTarget, abuf, alen);
            ++mReQuery;
            DnsResourceRecordsByPtr cnames;
            int status = 0;
            RRCache::instance()->lookup(mTarget, T_CNAME, mProto, cnames, status);
            assert(!cnames.empty());
            mDns->lookupRecords((dynamic_cast<DnsCnameRecord*>(cnames[0]))->cname(), mRRType, this);
            bDeleteThis = false;
         }
         else
         {
            mReQuery = 0;
            mResultConverter->notifyUser(mTarget, 0, mDns->errorMessage(0), Empty, mSink);
         }
         bGotAnswers = false;
      }
   }
}


