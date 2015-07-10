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

using namespace resip;
using namespace std;

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
      (*lb)->update(record, 3600);
      touch(*lb);
   }
   else
   {
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
   resip_assert(it != mFactoryMap.end());
   RRList* key = new RRList(domain, rrType);         
   RRSet::iterator lb = mRRSet.lower_bound(key);
   if (lb != mRRSet.end() &&
       !(mRRSet.key_comp()(key, *lb)))
   {
      (*lb)->update(it->second, begin, end, mUserDefinedTTL);
      touch(*lb);
   }
   else
   {
      RRList* val = new RRList(it->second, domain, rrType, begin, end, mUserDefinedTTL);
      mRRSet.insert(val);
      mLruHead->push_back(val);
      purge();
   }
   delete key;
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
}

bool 
RRCache::lookup(const Data& target, 
                const int type, 
                const int protocol,
                Result& records, 
                int& status)
{
   records.empty();
   status = 0;
   RRList* key = new RRList(target, type);
   RRSet::iterator it = mRRSet.find(key);
   delete key;
   if (it == mRRSet.end())
   {
      return false;
   }
   else
   {
      if (Timer::getTimeSecs() >= (*it)->absoluteExpiry())
      {
         delete *it;
         mRRSet.erase(it);
         return false;
      }
      else
      {
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
}

int 
RRCache::getTTL(const RROverlay& overlay)
{
   // overlay is a soa answer.
   if (overlay.type() != T_SOA) return -1;
   char* name = 0;
   long len = 0;
   int status = ares_expand_name(overlay.data(), overlay.msg(), overlay.msgLength(), &name, &len);
   resip_assert( status == ARES_SUCCESS );
   const unsigned char* pPos = overlay.data() + len;
   free(name); name = 0;
   status = ares_expand_name(pPos, overlay.msg(), overlay.msgLength(), &name, &len);
   resip_assert( status == ARES_SUCCESS );
   free(name);
   pPos += len;
   pPos += 16; // skip four 32 bit entities.
   return DNS__32BIT(pPos);         
}

void 
RRCache::purge()
{
   if (mRRSet.size() < mSize) return;
   RRList* lst = *(mLruHead->begin());
   RRSet::iterator it = mRRSet.find(lst);
   resip_assert(it != mRRSet.end());
   lst->remove();
   delete *it;
   mRRSet.erase(it);
}

void 
RRCache::logCache()
{
   UInt64 now = Timer::getTimeSecs();
   for (std::set<RRList*, CompareT>::iterator it = mRRSet.begin(); it != mRRSet.end(); )
   {
      if (now >= (*it)->absoluteExpiry())
      {
         delete *it;
         mRRSet.erase(it++);
      }
      else
      {
         (*it)->log();
         ++it;
      }
   }
}

void 
RRCache::getCacheDump(Data& dnsCacheDump)
{
   UInt64 now = Timer::getTimeSecs();
   DataStream strm(dnsCacheDump);
   for (std::set<RRList*, CompareT>::iterator it = mRRSet.begin(); it != mRRSet.end(); )
   {
      if (now >= (*it)->absoluteExpiry())
      {
         delete *it;
         mRRSet.erase(it++);
      }
      else
      {
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
