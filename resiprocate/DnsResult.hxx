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
      DnsResult(DnsInterface& i);
      ~DnsResult();

      typedef enum
      {
         Available, // A result is returned
         Pending,   // More results may be pending 
         Finished,  // No more results available
         Destroyed  // the associated transaction has been deleted
      } Type;

      // Check if there are tuples available now. Will load new tuples in if
      // necessary at a lower priority. 
      bool available();
      
      // return the next tuple available for this query. 
      Transport::Tuple next();

      // Will delete this DnsResult if no pending queries are out there or wait
      // until the pending queries get responses and then delete
      void destroy();
      
      class NAPTR
      {
         public:
            NAPTR();
            bool operator<(const NAPTR& rhs) const;

            Data key; // NAPTR record key
            
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
            SRV();
            bool operator<(const SRV& rhs) const;
            
            Data key; // SRV record key
            
            Transport::Type transport;
            int priority;
            int weight;
            int cumulativeWeight; // for picking 
            int port;
            Data target;
      };

   private:
      void lookup(const Uri& uri, const Data& tid);
      void lookupARecords(const Data& target);
      void lookupNAPTR();
      void lookupSRV(const Data& target);
    
      void processNAPTR(int status, unsigned char* abuf, int alen);
      void processSRV(int status, unsigned char* abuf, int alen);
      void processHost(int status, struct hostent* result);
      
      // compute the cumulative weights for the SRV entries with the lowest
      // priority, then randomly pick according to RFC2782 from the entries with
      // the lowest priority based on weights. When all entries at the lowest
      // priority are chosen, pick the next lowest priority and repeat. After an
      // SRV entry is selected, remove it from mSRVResults
      SRV retrieveSRV();
      
      // Will retrieve the next SRV record and compute the prime the mResults
      // with the appropriate Transport::Tuples. 
      void primeResults();
      
      // this will parse any ADDITIONAL records from the dns result and stores
      // them in the DnsResult. Only keep additional SRV records where
      // mNAPTRResults contains a record with replacement is that SRV
      // Store all A records for convenience in mARecords 
      const unsigned char* parseAdditional(const unsigned char* aptr, 
                                           const unsigned char* abuf,
                                           int alen);

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
      
      NAPTR mPreferredNAPTR;
      int mCumulativeWeight; // for current priority
      std::set<SRV> mSRVResults;
      std::map<Data,std::list<struct in_addr> > mARecords;
      
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
