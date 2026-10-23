#include <cstring>
#include <iostream>
#include <vector>
#include "assert.h"

#include "rutil/BaseException.hxx"
#include "rutil/Logger.hxx"
#include "rutil/dns/RROverlay.hxx"
#include "rutil/dns/DnsHostRecord.hxx"
#include "rutil/dns/DnsAAAARecord.hxx"
#include "rutil/dns/DnsSrvRecord.hxx"
#include "rutil/dns/DnsNaptrRecord.hxx"
#include "rutil/dns/DnsCnameRecord.hxx"
#include "rutil/dns/RRCache.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

// Builds a DNS message holding one resource record for "a." with the given
// type and RDATA, ending exactly where the RDATA ends, so that reading past
// the RDATA reads past the message. A trailer, if given, follows the RDATA,
// standing in for the next record in the answer.
class RRMessage
{
   public:
      RRMessage(int type, const vector<unsigned char>& rdata,
                const vector<unsigned char>& trailer = vector<unsigned char>())
      {
         static const unsigned char header[] = { 0, 1, 0x81, 0x80, 0, 0, 0, 1, 0, 0, 0, 0 };
         vector<unsigned char> msg(header, header + sizeof(header));
         mRecordOffset = msg.size();
         msg.push_back(1); msg.push_back('a'); msg.push_back(0);
         msg.push_back((unsigned char)(type >> 8)); msg.push_back((unsigned char)type);
         msg.push_back(0); msg.push_back(1);
         msg.push_back(0); msg.push_back(0); msg.push_back(0x0e); msg.push_back(0x10);
         msg.push_back((unsigned char)(rdata.size() >> 8)); msg.push_back((unsigned char)rdata.size());
         msg.insert(msg.end(), rdata.begin(), rdata.end());
         msg.insert(msg.end(), trailer.begin(), trailer.end());

         // exactly sized heap copy, so an over-read is visible to AddressSanitizer
         mLen = (int)msg.size();
         mBuf = new unsigned char[mLen];
         memcpy(mBuf, &msg[0], mLen);
      }
      ~RRMessage()
      {
         delete [] mBuf;
      }
      RROverlay overlay() const
      {
         return RROverlay(mBuf + mRecordOffset, mBuf, mLen);
      }

   private:
      RRMessage(const RRMessage&);
      RRMessage& operator=(const RRMessage&);

      unsigned char* mBuf;
      int mLen;
      size_t mRecordOffset;
};

template<class T>
static bool
parses(int type, const vector<unsigned char>& rdata,
       const vector<unsigned char>& trailer = vector<unsigned char>())
{
   RRMessage msg(type, rdata, trailer);
   try
   {
      T record(msg.overlay());
      return true;
   }
   catch (BaseException& e)
   {
      resipCerr << "rejected: " << e << endl;
      return false;
   }
}

// Feeds an SOA record with the given RDATA to RRCache::cacheTTL and reports
// whether a negative entry for "a" was cached, which only happens when
// getTTL managed to read the minimum field.
static bool
cachesTTL(const vector<unsigned char>& rdata)
{
   RRMessage msg(6, rdata);
   RRCache cache;
   cache.cacheTTL("a", 1, 4 /* ARES_ENOTFOUND */, msg.overlay());

   RRCache::Result records;
   int status = 0;
   return cache.lookup("a", 1, RRList::Protocol::Sip, records, status);
}

static vector<unsigned char>
bytes(const char* data, size_t len)
{
   return vector<unsigned char>(data, data + len);
}

int
main(int argc, char* argv[])
{
   Log::initialize(Log::Cout, Log::Debug, argv[0]);

   // A
   assert(parses<DnsHostRecord>(1, bytes("\x7f\x00\x00\x01", 4)));
   assert(!parses<DnsHostRecord>(1, bytes("", 0)));
   assert(!parses<DnsHostRecord>(1, bytes("\x7f\x00", 2)));
   assert(!parses<DnsHostRecord>(1, bytes("\x7f\x00\x00\x01\x7f\x00\x00\x02", 8)));

#ifdef USE_IPV6
   // AAAA
   assert(parses<DnsAAAARecord>(28, bytes("\x20\x01\x0d\xb8\0\0\0\0\0\0\0\0\0\0\0\x01", 16)));
   assert(!parses<DnsAAAARecord>(28, bytes("", 0)));
   assert(!parses<DnsAAAARecord>(28, bytes("\x20\x01\x0d\xb8", 4)));
   assert(!parses<DnsAAAARecord>(28, bytes("\x20\x01\x0d\xb8\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\0", 20)));
#endif

   // SRV: priority, weight, port, target
   assert(parses<DnsSrvRecord>(33, bytes("\x00\x0a\x00\x05\x13\xc4\x01x\x00", 9)));
   assert(!parses<DnsSrvRecord>(33, bytes("", 0)));
   assert(!parses<DnsSrvRecord>(33, bytes("\x00\x0a\x00\x05", 4)));
   assert(!parses<DnsSrvRecord>(33, bytes("\x00\x0a\x00\x05\x13\xc4", 6)));
   // target runs past RDLENGTH into the next record
   assert(!parses<DnsSrvRecord>(33, bytes("\x00\x0a\x00\x05\x13\xc4\x01x", 8), bytes("\x00", 1)));

   // NAPTR: order, preference, flags, service, regexp, replacement
   assert(parses<DnsNaptrRecord>(35, bytes("\x00\x0a\x00\x05\x01s\x07SIP+D2U\x00\x01x\x00", 18)));
   assert(parses<DnsNaptrRecord>(35, bytes("\x00\x0a\x00\x05\x00\x00\x00\x00", 8)));
   assert(!parses<DnsNaptrRecord>(35, bytes("", 0)));
   assert(!parses<DnsNaptrRecord>(35, bytes("\x00\x0a\x00\x05", 4)));
   assert(!parses<DnsNaptrRecord>(35, bytes("\x00\x0a\x00\x05\x01s", 6)));
   assert(!parses<DnsNaptrRecord>(35, bytes("\x00\x0a\x00\x05\x01s\x00", 7)));
   assert(!parses<DnsNaptrRecord>(35, bytes("\x00\x0a\x00\x05\x01s\x00\x00", 8)));
   // replacement runs past RDLENGTH into the next record
   assert(!parses<DnsNaptrRecord>(35, bytes("\x00\x0a\x00\x05\x01s\x07SIP+D2U\x00\x01x", 17), bytes("\x00", 1)));

   // CNAME
   assert(parses<DnsCnameRecord>(5, bytes("\x01x\x00", 3)));
   assert(!parses<DnsCnameRecord>(5, bytes("", 0)));
   assert(!parses<DnsCnameRecord>(5, bytes("\x01x", 2), bytes("\x00", 1)));

   // SOA: mname, rname, serial, refresh, retry, expire, minimum
   assert(cachesTTL(bytes("\x01m\x00\x01r\x00\0\0\0\x01\0\0\0\x02\0\0\0\x03\0\0\0\x04\0\0\x0e\x10", 26)));
   assert(!cachesTTL(bytes("\x01m\x00\x01r\x00", 6)));
   assert(!cachesTTL(bytes("\x01m\x00\x01r\x00\0\0\0\x01\0\0\0\x02\0\0\0\x03\0\0\0\x04", 22)));
   assert(!cachesTTL(bytes("\x01m\x00\x01r\x00\0\0\0\x01\0\0\0\x02\0\0\0\x03\0\0\0\x04\0\0", 24)));

   resipCerr << "All OK" << endl;
   return 0;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
