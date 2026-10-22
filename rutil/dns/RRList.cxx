#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <vector>
#include <list>

#include "ares.h"
#ifdef WIN32
#undef write  // Note:  ares.h defines write to be _write for WIN32 - we don't want that here, since we use fdset.write and stream write
#endif

#include "AresCompat.hxx"

#ifndef WIN32
#ifndef __CYGWIN__
#include <arpa/nameser.h>
#endif
#endif

#include "rutil/Logger.hxx"
#include "rutil/compat.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/Timer.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/dns/AresDns.hxx"
#include "rutil/dns/DnsResourceRecord.hxx"
#include "rutil/dns/DnsHostRecord.hxx"
#include "rutil/dns/RRFactory.hxx"
#include "rutil/dns/RRList.hxx"
#include "rutil/dns/DnsAAAARecord.hxx"
#include "rutil/dns/DnsHostRecord.hxx"
#include "rutil/dns/DnsNaptrRecord.hxx"
#include "rutil/dns/DnsSrvRecord.hxx"
#include "rutil/dns/DnsCnameRecord.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::DNS

RRList::RRList() : mRRType(0), mStatus(0), mAbsoluteExpiry(ULONG_MAX) {}

RRList::RRList(const Data& key, 
               const int rrtype, 
               int ttl, 
               int status)
   : mKey(key), mRRType(rrtype), mStatus(status)
{
   mAbsoluteExpiry = ttl + Timer::getTimeSecs();
}

RRList::RRList(const DnsHostRecord &record, int ttl)
   : mKey(record.name()), mRRType(T_A), mStatus(0), mAbsoluteExpiry(ULONG_MAX)
{
   update(record, ttl);
}

void RRList::update(const DnsHostRecord &record, int ttl)
{
   this->clear();

   RecordItem item;
   item.record = new DnsHostRecord(record);
   mRecords.push_back(item);
   mAbsoluteExpiry = Timer::getTimeSecs() + ttl;
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
   mAbsoluteExpiry = ULONG_MAX;
   
   // Add each record and track the lowest ttl of all the records
   for (Itr it = begin; it != end; it++)
   {
      try
      {
         RecordItem item;
         item.record = factory->create(*it);
         mRecords.push_back(item);
         if ((uint64_t)it->ttl() < mAbsoluteExpiry)
         {
            mAbsoluteExpiry = it->ttl();
         }
      }
      catch (BaseException& e)
      {
         ErrLog(<< e.getMessage() << endl);
      }
   }

   if (mAbsoluteExpiry == ULONG_MAX || mAbsoluteExpiry < (uint64_t)ttl)
   {
      mAbsoluteExpiry = ttl;
   }

   mAbsoluteExpiry += Timer::getTimeSecs();
}

RRList::Records RRList::records(const int protocol)
{
   Records records;
   if (mRecords.empty()) return records;

   for (std::vector<RecordItem>::iterator it = mRecords.begin(); it != mRecords.end(); ++it)
   {
      records.push_back((*it).record);
   }
   return records;
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

EncodeStream&
RRList::encodeRRList(EncodeStream& strm)
{
   strm << "Key=" << mKey << ", Type=" << AresDns::dnsRRTypeToString(mRRType) << ", secsToExpirey=" << (mAbsoluteExpiry - Timer::getTimeSecs()) << ", status=" << mStatus << ", numRecords=" << mRecords.size();
   int recNum = 0;
   for (RecordArr::iterator it = mRecords.begin(); it != mRecords.end(); ++it)
   {
      recNum++;
      strm << endl << "  Record " << recNum << ": ";
      it->record->dump(strm);
   }
   return strm;
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
