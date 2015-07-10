#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//	WINCE -- stl headers have to be defined before standard c headers because of
//	MS non-consistent declaration of time_t. we defined _USE_32BIT_TIME_T
//	in all projects and that solved the issue with beta compiler, however
//	release version messes time_t definition again
#include <set>
#include <vector>
#include "rutil/ResipAssert.h"

#include "AresCompat.hxx"

#ifndef WIN32
#ifndef __CYGWIN__
#include <arpa/nameser.h>
#endif
#endif

#include "rutil/FdPoll.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Socket.hxx"
#include "rutil/compat.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/Data.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/dns/DnsStub.hxx"
#include "rutil/dns/ExternalDns.hxx"
#include "rutil/dns/ExternalDnsFactory.hxx"
#include "rutil/dns/QueryTypes.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::DNS

DnsStub::DnsResourceRecordsByPtr DnsStub::Query::Empty;

DnsStub::NameserverList DnsStub::EmptyNameserverList;
int DnsStub::mDnsTimeout = 0;
int DnsStub::mDnsTries = 0;
unsigned int DnsStub::mDnsFeatures = 0;

void
DnsResultSink::onLogDnsResult(const DNSResult<DnsHostRecord>& rr)
{
   DebugLog (<< rr);
}

void
DnsResultSink::onLogDnsResult(const DNSResult<DnsAAAARecord>& rr)
{
#if defined(USE_IPV6)
   DebugLog (<< rr);
#else
   ErrLog(<< "Something called "
            "DnsResultSink::onLogDnsResult(const DNSResult<DnsAAAARecord>& rr)"
            " when ipv6 support was disabled.");
#endif
}

void
DnsResultSink::onLogDnsResult(const DNSResult<DnsSrvRecord>& rr)
{
   DebugLog (<< rr);
}

void
DnsResultSink::onLogDnsResult(const DNSResult<DnsNaptrRecord>& rr)
{
   DebugLog (<< rr);
}

void
DnsResultSink::onLogDnsResult(const DNSResult<DnsCnameRecord>& rr)
{
   DebugLog (<< rr);
}

DnsStub::DnsStub(const NameserverList& additional,
                 AfterSocketCreationFuncPtr socketFunc,
                 AsyncProcessHandler* asyncProcessHandler,
                 FdPollGrp *pollGrp) :
   mInterruptorHandle(0),
   mCommandFifo(&mSelectInterruptor),
   mTransform(0),
   mDnsProvider(ExternalDnsFactory::createExternalDns()),
   mPollGrp(0),
   mAsyncProcessHandler(asyncProcessHandler)
{
   setPollGrp(pollGrp);

   int retCode = mDnsProvider->init(additional, socketFunc, mDnsTimeout, mDnsTries, mDnsFeatures);
   if (retCode != ExternalDns::Success)
   {
      if (retCode == ExternalDns::BuildMismatch)
      {
         resip_assert(0);
         throw DnsStubException("Library was not build w/ required capabilities(probably USE_IPV6 resip/ares mismatch",
                                __FILE__,__LINE__);
      }

      Data err(Data::Take, mDnsProvider->errorMessage(retCode));
      ErrLog (<< "Failed to initialize async dns library: " << err);

      throw DnsStubException("Failed to initialize async dns library " + err, __FILE__,__LINE__);
   }
}

DnsStub::~DnsStub()
{
   for (set<Query*>::iterator it = mQueries.begin(); it != mQueries.end(); ++it)
   {
      delete *it;
   }

   setPollGrp(0);
   delete mDnsProvider;
}

unsigned int
DnsStub::getTimeTillNextProcessMS()
{
    if(mCommandFifo.size() > 0) return 0;
    return mDnsProvider->getTimeTillNextProcessMS();
}

void
DnsStub::buildFdSet(FdSet& fdset)
{
   mDnsProvider->buildFdSet(fdset.read, fdset.write, fdset.size);
   mSelectInterruptor.buildFdSet(fdset);
}

void
DnsStub::processFifo()
{
   while (mCommandFifo.messageAvailable())
   {
      Command* command = mCommandFifo.getNext();
      command->execute();
      delete command;
   }
}

void
DnsStub::process(FdSet& fdset)
{
   mSelectInterruptor.process(fdset);
   processFifo();
   mDnsProvider->process(fdset.read, fdset.write);
}

void
DnsStub::processTimers()
{
   // the fifo is captures as a timer within getTimeTill... above
   processFifo();
   mDnsProvider->processTimers();
}

void 
DnsStub::queueCommand(Command* command)
{
   mCommandFifo.add(command);

   // Wake up fifo reader
   if (mAsyncProcessHandler)
   {
      mAsyncProcessHandler->handleProcessNotification();
   }
}

void
DnsStub::cache(const Data& key,
               in_addr addr)
{
   DnsHostRecord record(key, addr);
   mRRCache.updateCacheFromHostFile(record);
}

void
DnsStub::cache(const Data& key,
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
      mRRCache.updateCache(key, (*itLow).type(), itLow, itHigh);
      itLow = itHigh;
      if (itHigh != overlays.end())
      {
         itHigh = upper_bound(itLow, overlays.end(), *itLow);
      }
   }
}

void
DnsStub::cacheTTL(const Data& key,
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
   if(soa.empty())
   {
      return;
   }

   mRRCache.cacheTTL(key, rrType, status, soa[0]);
}

const unsigned char*
DnsStub::skipDNSQuestion(const unsigned char *aptr,
                         const unsigned char *abuf,
                         int alen)
{
   char *name=0;
   int status=0;
   long len = 0;

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
DnsStub::checkDnsChange()
{
	return mDnsProvider ? mDnsProvider->checkDnsChange() : false;
}

bool
DnsStub::supportedType(int type)
{
   if(mDnsProvider && mDnsProvider->hostFileLookupLookupOnlyMode())
   {
      return (T_A == type);
   }
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
   long len = 0;

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

void
DnsStub::setResultTransform(ResultTransform* transform)
{
   mTransform = transform;
}

void
DnsStub::removeResultTransform()
{
   mTransform = 0;
}

void 
DnsStub::setPollGrp(FdPollGrp* pollGrp)
{
   if(mPollGrp)
   {
      // unregister our select interruptor
      mPollGrp->delPollItem(mInterruptorHandle);
      mInterruptorHandle=0;
   }

   mPollGrp=pollGrp;

   if (mPollGrp)
   {
      mInterruptorHandle = mPollGrp->addPollItem(mSelectInterruptor.getReadSocket(), FPEM_Read, &mSelectInterruptor);
   }

   mDnsProvider->setPollGrp(mPollGrp);
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
   resip_assert(s);
}

DnsStub::Query::~Query()
{
   delete mResultConverter; //.dcm. flyweight?
}

static Data
typeToData(int rr)
{
   // !dcm! fix this
   if (rr == RR_A::getRRType())
   {
      return RR_A::getRRTypeName();
   }
#if defined(USE_IPV6)
   else if(rr == RR_AAAA::getRRType())
   {
      return RR_AAAA::getRRTypeName();
   }
#endif
   else if (rr ==RR_NAPTR::getRRType())
   {
      return RR_NAPTR::getRRTypeName();
   }
   else if(rr == RR_SRV::getRRType())
   {
      return RR_SRV::getRRTypeName();
   }
   else if (RR_CNAME::getRRType())
   {
      return RR_CNAME::getRRTypeName();
   }
   else
   {
      return "Unknown";
   }
}

void
DnsStub::Query::go()
{
   StackLog(<< "DNS query of:" << mTarget << " " << typeToData(mRRType));

   DnsResourceRecordsByPtr records;
   int status = 0;
   bool cached = false;
   Data targetToQuery = mTarget;
   cached = mStub.mRRCache.lookup(mTarget, mRRType, mProto, records, status);

   if (!cached)
   {
      if (mRRType != T_CNAME)
      {
         do
         {
            DnsResourceRecordsByPtr cnames;
            cached = mStub.mRRCache.lookup(targetToQuery, T_CNAME, mProto, cnames, status);
            if (cached)
            {
               targetToQuery = (dynamic_cast<DnsCnameRecord*>(cnames[0]))->cname();
            }
         } while(cached);
      }
   }

   if (targetToQuery != mTarget)
   {
      StackLog(<< mTarget << " mapped to CNAME " << targetToQuery);
      cached = mStub.mRRCache.lookup(targetToQuery, mRRType, mProto, records, status);
   }

   if (!cached)
   {
      if(mStub.mDnsProvider && mStub.mDnsProvider->hostFileLookupLookupOnlyMode())
      {
         resip_assert(mRRType == T_A);
         StackLog (<< targetToQuery << " not cached. Doing hostfile lookup");
         in_addr address;
         if (mStub.mDnsProvider->hostFileLookup(targetToQuery.c_str(), address))
         {
            mStub.cache(mTarget, address);
            DnsResourceRecordsByPtr result;
            int queryStatus = 0;

            mStub.mRRCache.lookup(mTarget, mRRType, mProto, result, queryStatus);
            if (mTransform)
            {
                mTransform->transform(mTarget, mRRType, result);
            }
            mResultConverter->notifyUser(mTarget, queryStatus, mStub.errorMessage(queryStatus), result, mSink);
         }
         else
         {
            // Not in hosts file - return error - or.. we could fallback to doing the lookupRecords call on the local named
            mResultConverter->notifyUser(mTarget, ARES_ENOTFOUND, mStub.errorMessage(ARES_ENOTFOUND), Empty, mSink);
         }
         mReQuery = 0;
         mStub.removeQuery(this);
         delete this;
         return;
      }
      else
      {
         StackLog (<< targetToQuery << " not cached. Doing external dns lookup");
         mStub.lookupRecords(targetToQuery, mRRType, this);
      }
   }
   else // is cached
   {
      if (mTransform && !records.empty())
      {
         mTransform->transform(mTarget, mRRType, records);
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
      switch (status)
      {
         case ARES_ENODATA:
         case ARES_EFORMERR:
         case ARES_ESERVFAIL:
         case ARES_ENOTFOUND:
         case ARES_ENOTIMP:
         case ARES_EREFUSED:
            if(mRRType == T_A)
            {
               in_addr address;
               if (mStub.mDnsProvider->hostFileLookup(mTarget.c_str(), address))
               {
                  mStub.cache(mTarget, address);
                  mReQuery = 0;
                  DnsResourceRecordsByPtr result;
                  int queryStatus = 0;

                  mStub.mRRCache.lookup(mTarget, mRRType, mProto, result, queryStatus);
                  if (mTransform)
                  {
                     mTransform->transform(mTarget, mRRType, result);
                  }
                  mResultConverter->notifyUser(mTarget, queryStatus, mStub.errorMessage(queryStatus), result, mSink);
                  mStub.removeQuery(this);
                  delete this;
                  return;
               }
            }
            try
            {
               mStub.cacheTTL(mTarget, mRRType, status, abuf, alen);
            }
            catch (BaseException& e)
            {
               // if the response isn't parsable, we might want to consider caching
               // TTL anyways to delay the query attempt for this record.
               ErrLog(<< "Couldn't parse failure response to lookup for " << mTarget);
               InfoLog(<< e.getMessage());
            }
            break;

         case ARES_ECONNREFUSED:
         case ARES_ETIMEOUT:
            ErrLog (<< "Connection error " << mStub.errorMessage(status) << " for " << mTarget);
            break;
         case ARES_EBADRESP:
            ErrLog (<< "Server response error " << mStub.errorMessage(status) << " for " << mTarget);
            break;
         case ARES_EOF:
         case ARES_EFILE:
         case ARES_ENOMEM:
         case ARES_EDESTRUCTION:
            ErrLog (<< "Error " << mStub.errorMessage(status) << " for " << mTarget);
            break;
         case ARES_EBADNAME:
            ErrLog(<< "Garbage hostname failed to resolve: " << mStub.errorMessage(status) << " for " << mTarget);
            break;
         case ARES_EBADQUERY:
            ErrLog(<< "Query was malformed (probably because hostname was "
                        "too long) " << mStub.errorMessage(status) << " for "
                        << mTarget);
            break;
         case ARES_EBADFAMILY:
            ErrLog (<< "Bad lookup type " << mStub.errorMessage(status) << " for " << mTarget);
            // .bwc. This should not happen. If it does, we have code to fix.
            resip_assert(0);
            break;
         default:
            ErrLog (<< "Unknown error " << mStub.errorMessage(status) << " for " << mTarget);
            resip_assert(0);
            break;
      }

      // For other error status values, we may also want to cacheTTL to delay
      // requeries. Especially if the server refuses.
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
         ErrLog(<< "Error parsing DNS record for " << mTarget << ": " << e.getMessage());
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

         if (mTarget != targetToQuery) DebugLog (<< mTarget << " mapped to " << targetToQuery << " and returned result");
         mStub.mRRCache.lookup(targetToQuery, mRRType, mProto, result, queryStatus);
         if (mTransform)
         {
            mTransform->transform(mTarget, mRRType, result);
         }
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
   long len = 0;

   if (ARES_SUCCESS != ares_expand_name(aptr, abuf, alen, &name, &len))
   {
      ErrLog(<< "Failed DNS preparse for " << targetToQuery);
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
      ErrLog(<< "Failed to cache result for " << targetToQuery << ": " << e.getMessage());
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
               cached = mStub.mRRCache.lookup(targetToQuery, T_CNAME, mProto, cnames, status);
               if (cached)
               {
                  ++mReQuery;
                  targetToQuery = (dynamic_cast<DnsCnameRecord*>(cnames[0]))->cname();
               }
            } while(mReQuery < MAX_REQUERIES && cached);

            DnsResourceRecordsByPtr result;
            if (!mStub.mRRCache.lookup(targetToQuery, mRRType, mProto, result, status))
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

void
DnsStub::lookupRecords(const Data& target, unsigned short type, DnsRawSink* sink)
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
   queueCommand(command);
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
DnsStub::setEnumDomains(const std::map<Data,Data>& domains)
{
   SetEnumDomainsCommand* command = new SetEnumDomainsCommand(*this, domains);
   queueCommand(command);
}

const std::map<Data,Data>&
DnsStub::getEnumDomains() const
{
   return mEnumDomains;
}

void
DnsStub::doSetEnumDomains(const std::map<Data,Data>& domains)
{
   mEnumDomains = domains;
}

void
DnsStub::clearDnsCache()
{
   ClearDnsCacheCommand* command = new ClearDnsCacheCommand(*this);
   queueCommand(command);
}

void
DnsStub::doClearDnsCache()
{
   mRRCache.clearCache();
}

void
DnsStub::logDnsCache()
{
   LogDnsCacheCommand* command = new LogDnsCacheCommand(*this);
   queueCommand(command);
}

void
DnsStub::doLogDnsCache()
{
   mRRCache.logCache();
}

void 
DnsStub::getDnsCacheDump(std::pair<unsigned long, unsigned long> key, GetDnsCacheDumpHandler* handler)
{
   GetDnsCacheDumpCommand* command = new GetDnsCacheDumpCommand(*this, key, handler);
   queueCommand(command);
}

void 
DnsStub::doGetDnsCacheDump(std::pair<unsigned long, unsigned long> key, GetDnsCacheDumpHandler* handler)
{
   resip_assert(handler != 0);
   Data dnsCacheDump;
   mRRCache.getCacheDump(dnsCacheDump);
   handler->onDnsCacheDumpRetrieved(key, dnsCacheDump);
}

void
DnsStub::setDnsCacheTTL(int ttl)
{
   mRRCache.setTTL(ttl);
}

void
DnsStub::setDnsCacheSize(int size)
{
   mRRCache.setSize(size);
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
 
