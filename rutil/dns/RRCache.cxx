#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "AresCompat.hxx"

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#ifndef __CYGWIN__
#  include <netinet/in.h>
#  include <arpa/nameser.h>
#  include <resolv.h>
#endif
#include <netdb.h>
#include <netinet/in.h>
#else
#include <Winsock2.h>
#include <svcguid.h>
#ifdef USE_IPV6
#include <ws2tcpip.h>
#endif
#endif

#include <set>
#include <vector>
#include <list>
#include <map>
#include "rutil/ResipAssert.h"
#include "rutil/BaseException.hxx"
#include "rutil/Data.hxx"
#include "rutil/Timer.hxx"
#include "rutil/Logger.hxx"
#include "rutil/dns/AresDns.hxx"
#include "rutil/dns/RRFactory.hxx"
#include "rutil/dns/RROverlay.hxx"
#include "rutil/dns/RRFactory.hxx"
#include "rutil/dns/DnsResourceRecord.hxx"
#include "rutil/dns/DnsAAAARecord.hxx"
#include "rutil/dns/DnsHostRecord.hxx"
#include "rutil/dns/DnsNaptrRecord.hxx"
#include "rutil/dns/DnsSrvRecord.hxx"
#include "rutil/dns/DnsCnameRecord.hxx"
#include "rutil/dns/RRCache.hxx"
#include "rutil/WinLeakCheck.hxx"

//#define VERBOSE_DNS_STACK_LOGS 1

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::DNS

RRCache::RRCache() 
   : mHead(),
     mLruHead(LruListType::makeList(&mHead)),
     mUserDefinedTTL(DEFAULT_USER_DEFINED_TTL),
     mSize(DEFAULT_SIZE)
{
   mFactoryMap[T_CNAME] = &mCnameRecordFactory;
   mFactoryMap[T_NAPTR] = &mNaptrRecordFacotry;
   mFactoryMap[T_SRV] = &mSrvRecordFactory;
#ifdef USE_IPV6
   mFactoryMap[T_AAAA] = &mAAAARecordFactory;
#endif
   mFactoryMap[T_A] = &mHostRecordFactory;
}

RRCache::~RRCache()
{
   cleanup();
}

void 
RRCache::updateCacheFromHostFile(const DnsHostRecord &record)
{
   //FactoryMap::iterator it = mFactoryMap.find(T_A);
   RRList* key = new RRList(record, 3600);
   RRSet::iterator lb = mRRSet.lower_bound(key);
   if (lb != mRRSet.end() &&
       !(mRRSet.key_comp()(key, *lb)))
   {
#ifdef VERBOSE_DNS_STACK_LOGS
      StackLog(<< "Updating cache from hostfile: target=" << record.name() << ", host=" << record.host());
#endif
      (*lb)->update(record, 3600);
      touch(*lb);
   }
   else
   {
#ifdef VERBOSE_DNS_STACK_LOGS
      StackLog(<< "Adding to cache from hostfile: target=" << record.name() << ", host=" << record.host());
#endif
      RRList* val = new RRList(record, 3600);
      mRRSet.insert(val);
      mLruHead->push_back(val);
      purge();
   }
   delete key;
}

void 
RRCache::updateCache(const Data& target,
                     const int rrType,
                     Itr begin, 
                     Itr end)
{
   Data domain = (*begin).domain();
   FactoryMap::iterator it = mFactoryMap.find(rrType);
   if (it != mFactoryMap.end())  // If we don't understand rrType - ignore it
   {
      RRList* key = new RRList(domain, rrType);
      RRSet::iterator lb = mRRSet.lower_bound(key);
      if (lb != mRRSet.end() &&
         !(mRRSet.key_comp()(key, *lb)))
      {
         (*lb)->update(it->second, begin, end, mUserDefinedTTL);
         if ((*lb)->numRecords() == 0)
         {
            // *lb might be a RRList with no records if parsing failed - remove from cache
            (*lb)->remove();
            delete *lb;
            mRRSet.erase(lb);
#ifdef VERBOSE_DNS_STACK_LOGS
            StackLog(<< "Update cache failed to parse, removed entry: target=" << target << ", type=" << AresDns::dnsRRTypeToString(rrType) << ", totalCachedEntries=" << mRRSet.size());
#endif
         }
         else
         {
#ifdef VERBOSE_DNS_STACK_LOGS
            StackLog(<< "Updated cache: target=" << target << ", type=" << AresDns::dnsRRTypeToString(rrType));
#endif
            // update good - touch entry
            touch(*lb);
         }
      }
      else
      {
         RRList* val = new RRList(it->second, domain, rrType, begin, end, mUserDefinedTTL);
         if (val->numRecords() == 0)
         {
#ifdef VERBOSE_DNS_STACK_LOGS
            StackLog(<< "Update cache failed to parse for new entry: target=" << target << ", type=" << AresDns::dnsRRTypeToString(rrType));
#endif
            // val might be a RRList with no records if parsing failed - don't cache
            delete val;
         }
         else
         {
            mRRSet.insert(val);
            mLruHead->push_back(val);
            purge();
#ifdef VERBOSE_DNS_STACK_LOGS
            StackLog(<< "Updated cache with new entry: target=" << target << ", type=" << AresDns::dnsRRTypeToString(rrType) << ", totalCachedEntries=" << mRRSet.size());
#endif
         }
      }
      delete key;
   }
}

void 
RRCache::cacheTTL(const Data& target,
                  const int rrType,
                  const int status,
                  RROverlay overlay)
{
   int ttl = getTTL(overlay);

   if (ttl < 0) 
   {
      return;
   }

   if (ttl < mUserDefinedTTL)
   {
      ttl = mUserDefinedTTL;
   }

   RRList* val = new RRList(target, rrType, ttl, status);
   RRSet::iterator it = mRRSet.find(val);
   if (it != mRRSet.end())
   {
      (*it)->remove();
      delete *it;
      mRRSet.erase(it);
   }
   mRRSet.insert(val);
   mLruHead->push_back(val);
   purge();

#ifdef VERBOSE_DNS_STACK_LOGS
   StackLog(<< "Updated cache ttl: target=" << target << ", type=" << AresDns::dnsRRTypeToString(rrType) << ", status=" << status << ", ttl=" << ttl << ", totalCachedEntries=" << mRRSet.size());
#endif
}

bool 
RRCache::lookup(const Data& target, 
                const int type, 
                const int protocol,
                Result& records, 
                int& status)
{
   records.clear();
   status = 0;
   RRList* key = new RRList(target, type);
   RRSet::iterator it = mRRSet.find(key);
   delete key;
   if (it == mRRSet.end())
   {
#ifdef VERBOSE_DNS_STACK_LOGS
      StackLog(<< "Cache lookup failed for: target=" << target << ", type=" << AresDns::dnsRRTypeToString(type) << ", protocol=" << protocol << ", totalCachedEntries=" << mRRSet.size());
#endif
      return false;
   }
   else
   {
      if (Timer::getTimeSecs() >= (*it)->absoluteExpiry())
      {
#ifdef VERBOSE_DNS_STACK_LOGS
         StackLog(<< "Cache lookup found expired entry for: target=" << target << ", type=" << AresDns::dnsRRTypeToString(type) << ", protocol=" << protocol << ", totalCachedEntries=" << mRRSet.size());
#endif
         (*it)->remove();
         delete *it;
         mRRSet.erase(it);
         return false;
      }
      else
      {
#ifdef VERBOSE_DNS_STACK_LOGS
         StackLog(<< "Cache lookup success for: target=" << target << ", type=" << AresDns::dnsRRTypeToString(type) << ", protocol=" << protocol << ", totalCachedEntries=" << mRRSet.size());
#endif
         records = (*it)->records(protocol);
         status = (*it)->status();
         touch(*it);
         return true;
      }
   }
}

void 
RRCache::clearCache()
{
    cleanup();
}

void 
RRCache::touch(RRList* node)
{
   node->remove();
   mLruHead->push_back(node);
}

void 
RRCache::cleanup()
{
   for (std::set<RRList*, CompareT>::iterator it = mRRSet.begin(); it != mRRSet.end(); it++)
   {
      (*it)->remove();
      delete *it;
   }
   mRRSet.clear();
#ifdef VERBOSE_DNS_STACK_LOGS
   StackLog(<< "Cache emptied, totalCachedEntries=" << mRRSet.size());
#endif
}

int
RRCache::getTTL(const RROverlay& overlay)
{
   // overlay is a soa answer.
   if (overlay.type() != T_SOA) return -1;
   char* name = 0;
   long len = 0;
   int status = ares_expand_name(overlay.data(), overlay.msg(), overlay.msgLength(), &name, &len);
   if (status == ARES_SUCCESS)
   {
      const unsigned char* pPos = overlay.data() + len;
      free(name);
      name = 0;
      status = ares_expand_name(pPos, overlay.msg(), overlay.msgLength(), &name, &len);
      if (status == ARES_SUCCESS)
      {
         free(name);
         pPos += len;
         pPos += 16; // skip four 32 bit entities.
         return DNS__32BIT(pPos);
      }
   }
   return -1;
}

void 
RRCache::purge()
{
   if (mRRSet.size() < mSize) return;
   RRList* lst = *(mLruHead->begin());
   RRSet::iterator it = mRRSet.find(lst);
   if (it != mRRSet.end()) // safety check incase code forgets to remove from LRU list when removing from mRRset
   {
#ifdef VERBOSE_DNS_STACK_LOGS
      StackLog(<< "Cache full purging LRU record, target=" << lst->key() << ", type=" << AresDns::dnsRRTypeToString(lst->rrType()) << ", status=" << lst->status() << ", totalCachedEntries=" << mRRSet.size() << ", maxSize=" << mSize);
#endif

      lst->remove();
      delete* it;
      mRRSet.erase(it);
   }
}

void 
RRCache::logCache()
{
   Data dnsCacheDump;
   getCacheDump(dnsCacheDump);
   WarningLog(<< endl << dnsCacheDump);
}

void 
RRCache::getCacheDump(Data& dnsCacheDump)
{
   uint64_t now = Timer::getTimeSecs();
   DataStream strm(dnsCacheDump);
   strm << "DNSCACHE: TotalEntries=" << mRRSet.size();
   for (std::set<RRList*, CompareT>::iterator it = mRRSet.begin(); it != mRRSet.end(); )
   {
      if (now >= (*it)->absoluteExpiry())
      {
         (*it)->remove();
         delete *it;
         mRRSet.erase(it++);
      }
      else
      {
         strm << endl;
         (*it)->encodeRRList(strm);
         ++it;
      }
   }
   strm.flush();
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * Copyright (c) 2010 SIP Spectrum, Inc.  All rights reserved.
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
