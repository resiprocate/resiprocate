#if !defined(RESIP_DNSRESULT_HXX)
#define RESIP_DNSINTERFACE_HXX

#include <iostream>
#include <set>
#include <list>

#include "resiprocate/Transport.hxx"

namespace resip
{
class DnsInterface;

class DnsResult
{
   public:
      DnsResult(DnsInterface& interface);
      ~DnsResult();

      typedef enum
      {
         Available, // A result is returned
         Pending,   // More results may be pending 
         Destroyed  // the associated transaction has been deleted
      } Type;

      // There are tuples available now
      bool available() const;

      // Check if there are any more tuples for this target. May move to the
      // Pending state if additional async dns queries are required
      bool finished();
      
      // return the next tuple available for this query. 
      Transport::Tuple next();

      // Will delete this DnsResult if no pending queries are out there or wait
      // until the pending queries get responses and then delete
      void destroy();
      
      class NAPTR
      {
         public:
            bool operator<(const NAPTR& rhs) const;
            
            int order;
            int pref;
            Data flags;
            Data service;
            Data regex;
            Data replacement;
      };
      
      class SRV
      {
         public:
            bool operator<(const SRV& rhs) const;
            
            Transport::Type transport;
            int priority;
            int weight;
            int port;
            Data target;
      };

   private:
      void lookup(const Uri& uri, const Data& tid);
      void lookupARecords(const Data& target);
      void lookupNAPTR();
      void lookupSRV(const Data& target);

      static void aresNAPTRCallback(void *arg, int status, unsigned char *abuf, int alen);
      static void aresSRVCallback(void *arg, int status, unsigned char *abuf, int alen);
      static void aresHostCallback(void *arg, int status, struct hostent* result);

      static const unsigned char* skipDNSQuestion(const unsigned char *aptr,
                                                  const unsigned char *abuf,
                                                  int alen);
      static const unsigned char* parseSRV(const unsigned char *aptr,
                                           const unsigned char *abuf, 
                                           int alen,
                                           SRV& srv);
      static const unsigned char* parseNAPTR(const unsigned char *aptr,
                                             const unsigned char *abuf, 
                                             int alen,
                                             NAPTR& naptr);

      void processHost(int status, struct hostent* result);
      void processNAPTR(int status, unsigned char* abuf, int alen);
      void processSRV(int status, unsigned char* abuf, int alen);
      
   private:
      DnsInterface& mInterface;
      int mSRVCount;
      bool mSips;
      Data mTarget;
      Data mTransactionId;
      Transport::Type mTransport; // current
      int mPort; // current
      Type mType;
      
      std::list<Transport::Tuple> mResults;
      std::set<NAPTR> mNAPTRResults;
      std::set<SRV> mSRVResults;

      friend class DnsInterface;
};


std::ostream& operator<<(std::ostream& strm, const DnsResult::SRV&);
std::ostream& operator<<(std::ostream& strm, const DnsResult::NAPTR&);

}

#endif
/* 
   Copyright (c) 2003, Jason Fischl
 
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    
   * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.
    
   * Neither the name of any of the copyright holders nor the names of
   its contributors may be used to endorse or promote products
   derived from this software without specific prior written
   permission.
    
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
