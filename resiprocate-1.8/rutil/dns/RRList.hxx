#ifndef RESIP_DNS_RR_LIST
#define RESIP_DNS_RR_LIST

#include <vector>

#include "rutil/IntrusiveListElement.hxx"
#include "rutil/dns/RRFactory.hxx"

namespace resip
{
class DnsResourceRecord;
class DnsHostRecord;

class RRList : public IntrusiveListElement<RRList*>
{
   public:

      class Protocol
      {
         public:
            static const int Reserved = 0;
            static const int Sip = 1;
            static const int Stun = 2;
            static const int Http = 3;
            static const int Enum = 4;
      };

      typedef std::vector<DnsResourceRecord*> Records;
      typedef IntrusiveListElement<RRList*> LruList;
      typedef std::vector<RROverlay>::const_iterator Itr;
      typedef std::vector<Data> DataArr;

      RRList();
      explicit RRList(const Data& key, const int rrtype, int ttl, int status);
      explicit RRList(const Data& key, int rrtype);
      ~RRList();
      RRList(const RRFactoryBase* factory, 
             const Data& key,
             const int rrType,
             Itr begin,
             Itr end, 
             int ttl);
      
      RRList(const DnsHostRecord &record, int ttl);
      void update(const DnsHostRecord &record, int ttl);

      void update(const RRFactoryBase* factory, Itr begin, Itr end, int ttl);
      Records records(const int protocol);

      const Data& key() const { return mKey; }
      int status() const { return mStatus; }
      int rrType() const { return mRRType; }
      UInt64 absoluteExpiry() const { return mAbsoluteExpiry; }
      UInt64& absoluteExpiry() { return mAbsoluteExpiry; }
      void log();
      EncodeStream& encodeRRList(EncodeStream& strm);

   private:

      struct RecordItem
      {
            DnsResourceRecord* record;
            std::vector<int> blacklistedProtocols;
      };

      typedef std::vector<RecordItem> RecordArr;
      typedef RecordArr::iterator RecordItr;

      RecordArr mRecords;

      Data mKey;
      int mRRType;

      int mStatus; // dns query status.
      UInt64 mAbsoluteExpiry;

      RecordItr find(const Data&);
      void clear();
      EncodeStream& encodeRecordItem(RRList::RecordItem& item, EncodeStream& strm);
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
