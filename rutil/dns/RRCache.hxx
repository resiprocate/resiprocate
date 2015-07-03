#ifndef RESIP_RRCACHE_HXX
#define RESIP_RRCACHE_HXX

#include <map>
#include <set>
#include <memory>

#include "rutil/dns/RRFactory.hxx"
#include "rutil/dns/DnsResourceRecord.hxx"
#include "rutil/dns/DnsAAAARecord.hxx"
#include "rutil/dns/DnsHostRecord.hxx"
#include "rutil/dns/DnsNaptrRecord.hxx"
#include "rutil/dns/DnsSrvRecord.hxx"
#include "rutil/dns/DnsCnameRecord.hxx"
#include "rutil/dns/RRList.hxx"

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

      RRCache();
      ~RRCache();
      void setTTL(int ttl) { if (ttl > 0) mUserDefinedTTL = ttl * MIN_TO_SEC; }
      void setSize(int size) { mSize = size; }
      // Update existing cache record, or add a new one
      void updateCache(const Data& target,
                       const int rrType,
                       Itr  begin, 
                       Itr  end);
      void updateCacheFromHostFile(const DnsHostRecord&);
      // Called to update the cache when there are DNS server errors (ie. record not found)
      void cacheTTL(const Data& target,
                    const int rrType,
                    const int status,
                    RROverlay overlay);
      bool lookup(const Data& target, const int type, const int proto, Result& records, int& status);
      void clearCache();
      void logCache();
      void getCacheDump(Data& dnsCacheDump);

   private:
      static const int MIN_TO_SEC = 60;
      static const int DEFAULT_USER_DEFINED_TTL = 10; // in seconds.

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
                  return (Data(lhs->key())).lowercase() < (Data(rhs->key())).lowercase();
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
      RRFactory<DnsAAAARecord> mAAAARecordFactory;
      RRFactory<DnsNaptrRecord> mNaptrRecordFacotry;
      RRFactory<DnsCnameRecord> mCnameRecordFactory;

      typedef std::map<int, resip::RRFactoryBase*> FactoryMap;
      FactoryMap  mFactoryMap;
      
      int mUserDefinedTTL; // used when the ttl in RR is 0 or less than default(60). in seconds.
      unsigned int mSize;
};

}

#endif


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
