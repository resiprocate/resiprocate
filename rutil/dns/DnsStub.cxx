#if defined(HAVE_CONFIG_H)
#include "rutil/config.hxx"
#endif

//	WINCE -- stl headers have to be defined before standard c headers because of 
//	MS non-consistent declaration of time_t. we defined _USE_32BIT_TIME_T
//	in all projects and that solved the issue with beta compiler, however 
//	release version messes time_t definition again
#include <set>
#include <vector>
#include <cassert>

#if defined(USE_ARES)
#include "ares.h"
#include "ares_dns.h"
#endif

#ifndef WIN32
#ifndef __CYGWIN__
#include <arpa/nameser.h>
#endif
#endif

#include "rutil/Logger.hxx"
#include "rutil/Socket.hxx"
#include "rutil/compat.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/Data.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/dns/DnsStub.hxx"
#include "rutil/dns/ExternalDns.hxx"
#include "rutil/dns/ExternalDnsFactory.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::DNS

DnsStub::DnsResourceRecordsByPtr DnsStub::Query::Empty;

DnsStub::NameserverList DnsStub::EmptyNameserverList;
int DnsStub::mDnsTimeout = 0;
int DnsStub::mDnsTries = 0;

DnsStub::DnsStub(const NameserverList& additional,
                 AfterSocketCreationFuncPtr socketFunc) :
   mTransform(0),
   mDnsProvider(ExternalDnsFactory::createExternalDns())
{
   int retCode = mDnsProvider->init(additional, socketFunc, mDnsTimeout, mDnsTries);
   if (retCode != ExternalDns::Success)
   {
      if (retCode == ExternalDns::BuildMismatch)
      {
         throw DnsStubException("Library was not build w/ required capabilities(probably USE_IPV6 resip/ares mismatch", 
                                __FILE__,__LINE__);
      }
      
      ErrLog (<< "Failed to initialize async dns library");
      char* errmem = mDnsProvider->errorMessage(retCode);
      ErrLog (<< errmem);
      delete errmem;
      throw DnsStubException("Failed to initialize async dns library", __FILE__,__LINE__);
   }
}

DnsStub::~DnsStub()
{
   for (set<Query*>::iterator it = mQueries.begin(); it != mQueries.end(); ++it)
   {
      delete *it;
   }

   delete mDnsProvider;
}

bool 
DnsStub::requiresProcess()
{
   return mDnsProvider->requiresProcess();
}

void 
DnsStub::buildFdSet(FdSet& fdset)
{
   mDnsProvider->buildFdSet(fdset.read, fdset.write, fdset.size);
}

void DnsStub::process(FdSet& fdset)
{
   while (mCommandFifo.messageAvailable())
   {
      Command* command = mCommandFifo.getNext();
      command->execute();
      delete command;
   }
   mDnsProvider->process(fdset.read, fdset.write);
}

void DnsStub::cache(const Data& key,
                    const unsigned char* abuf, 
                    int alen)
{

   vector<RROverlay> overlays;

   // skip header
   const unsigned char* aptr = abuf + HFIXEDSZ;

   int qdcount = DNS_HEADER_QDCOUNT(abuf); // questions.
   for (int i = 0; i < qdcount && aptr; ++i)
   {
      aptr = skipDNSQuestion(aptr, abuf, alen);
   }

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
   if (soa.empty())
   {
      DebugLog(<< "no TTL to cache");
      return;
   }
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
#ifdef USE_IPV6
   return (T_A == type ||
           T_AAAA == type ||
           T_NAPTR == type ||
           T_SRV == type ||
           T_CNAME == type ||
           T_SOA == type);
#else
   return (T_A == type ||
           T_NAPTR == type ||
           T_SRV == type ||
           T_CNAME == type ||
           T_SOA == type);
#endif
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
     mFollowCname(followCname)
{
   assert(s);               
}

DnsStub::Query::~Query() 
{
   delete mResultConverter; //.dcm. flyweight?
}


void 
DnsStub::Query::go()
{
   StackLog(<< "DnsStub::Query::go: " << mTarget << " type(enum): " << mRRType << " proto: " << mProto);   
   DnsResourceRecordsByPtr records;
   int status = 0;
   bool cached = false;
   Data targetToQuery = mTarget;
   cached = RRCache::instance()->lookup(mTarget, mRRType, mProto, records, status);

   if (!cached)
   {
      if (mRRType != T_CNAME)
      {
         do
         {
            DnsResourceRecordsByPtr cnames;
            cached = RRCache::instance()->lookup(targetToQuery, T_CNAME, mProto, cnames, status);
            if (cached) 
            {
               targetToQuery = (dynamic_cast<DnsCnameRecord*>(cnames[0]))->cname();
            }
         } while(cached);
      }
   }

   if (targetToQuery != mTarget)
   {
      cached = RRCache::instance()->lookup(targetToQuery, mRRType, mProto, records, status);
   }
   
   if (!cached)
   {
      mStub.lookupRecords(targetToQuery, mRRType, this);
   }
   else
   {
      if (mTransform && !records.empty())
      {
         mTransform->transform(targetToQuery, mRRType, records);
      }
      mResultConverter->notifyUser(mTarget, status, mStub.errorMessage(status), records, mSink); 
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
         try
         {
            mStub.cacheTTL(mTarget, mRRType, status, abuf, alen);
         }
         catch (BaseException& e)
         {
            ErrLog(<< e.getMessage() << endl);
         }
      }
      mResultConverter->notifyUser(mTarget, status, mStub.errorMessage(status), Empty, mSink);
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
      try
      {
         aptr = mStub.skipDNSQuestion(aptr, abuf, alen);
      }
      catch (BaseException& e)
      {
         ErrLog(<< e.getMessage() << endl);
         mResultConverter->notifyUser(mTarget, ARES_EFORMERR, e.getMessage(), Empty, mSink); 
         mStub.removeQuery(this);
         delete this;
         return;
      }
   }

   int ancount = DNS_HEADER_ANCOUNT(abuf);
   if (ancount == 0)
   {
      mResultConverter->notifyUser(mTarget, 0, mStub.errorMessage(0), Empty, mSink); 
   }
   else
   {
      bool bGotAnswers = true;
      Data targetToQuery;
      followCname(aptr, abuf, alen, bGotAnswers, bDeleteThis, targetToQuery);

      if (bGotAnswers)
      {
         mReQuery = 0;
         DnsResourceRecordsByPtr result;
         int queryStatus = 0;
         RRCache::instance()->lookup(targetToQuery, mRRType, mProto, result, queryStatus);
         if (mTransform) mTransform->transform(targetToQuery, mRRType, result);
         mResultConverter->notifyUser(mTarget, queryStatus, mStub.errorMessage(queryStatus), result, mSink);
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
DnsStub::Query::followCname(const unsigned char* aptr, const unsigned char*abuf, const int alen, bool& bGotAnswers, bool& bDeleteThis, Data& targetToQuery)
{
   bGotAnswers = true;
   bDeleteThis = true;

   char* name = 0;
   int len = 0;

   if (ARES_SUCCESS != ares_expand_name(aptr, abuf, alen, &name, &len))
   {
      ErrLog(<< "Failed DNS preparse"  << endl);
      mResultConverter->notifyUser(mTarget, ARES_EFORMERR, "Failed DNS preparse", Empty, mSink); 
      bGotAnswers = false;
      return;
   }

   targetToQuery = name;
   aptr += len;

   try
   {
      mStub.cache(name, abuf, alen);
   }
   catch (BaseException& e)
   {
      ErrLog(<< e.getMessage() << endl);
      mResultConverter->notifyUser(mTarget, ARES_EFORMERR, e.getMessage(), Empty, mSink); 
      bGotAnswers = false;
      return;
   }

   if (mRRType != T_CNAME)
   {
      if (DNS_RR_TYPE(aptr) == T_CNAME)
      {
         if (mFollowCname && mReQuery < MAX_REQUERIES)
         {
            ++mReQuery;
            int status = 0;
            bool cached = false;

            do
            {
               DnsResourceRecordsByPtr cnames;
               cached = RRCache::instance()->lookup(targetToQuery, T_CNAME, mProto, cnames, status);
               if (cached) 
               {
                  ++mReQuery;
                  targetToQuery = (dynamic_cast<DnsCnameRecord*>(cnames[0]))->cname();
               }
            } while(mReQuery < MAX_REQUERIES && cached);

            DnsResourceRecordsByPtr result;
            if (!RRCache::instance()->lookup(targetToQuery, mRRType, mProto, result, status))
            {
               mStub.lookupRecords(targetToQuery, mRRType, this);
               bDeleteThis = false;
               bGotAnswers = false;
            }
         }
         else
         {
            mReQuery = 0;
            mResultConverter->notifyUser(mTarget, 1, mStub.errorMessage(1), Empty, mSink);
            bGotAnswers = false;
         }
      }
   }

   free(name);
}

Data 
DnsStub::errorMessage(int status)
{
   return Data(Data::Take, mDnsProvider->errorMessage(status));
}

void DnsStub::lookupRecords(const Data& target, unsigned short type, DnsRawSink* sink)
{
   mDnsProvider->lookup(target.c_str(), type, this, sink);
}

void 
DnsStub::handleDnsRaw(ExternalDnsRawResult res)
{
   reinterpret_cast<DnsRawSink*>(res.userData)->onDnsRaw(res.errorCode(), res.abuf, res.alen);
   mDnsProvider->freeResult(res);
}

void
DnsStub::setEnumSuffixes(const std::vector<Data>& suffixes)
{
   SetEnumSuffixesCommand* command = new SetEnumSuffixesCommand(*this, suffixes);
   mCommandFifo.add(command);
}

const std::vector<Data>& 
DnsStub::getEnumSuffixes() const
{
   return mEnumSuffixes;
}

void
DnsStub::doSetEnumSuffixes(const std::vector<Data>& suffixes)
{
   mEnumSuffixes = suffixes;
}

void
DnsStub::clearDnsCache()
{
   ClearDnsCacheCommand* command = new ClearDnsCacheCommand(*this);
   mCommandFifo.add(command);
}

void
DnsStub::doClearDnsCache()
{
    RRCache::instance()->clearCache();
}

void
DnsStub::logDnsCache()
{
   LogDnsCacheCommand* command = new LogDnsCacheCommand(*this);
   mCommandFifo.add(command);
}

void
DnsStub::doLogDnsCache()
{
    RRCache::instance()->logCache();
}
