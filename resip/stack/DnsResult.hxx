#if !defined(RESIP_DNSRESULT_HXX)
#define RESIP_DNSRESULT_HXX

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <iosfwd>
#include <set>
#include <vector>
#include <deque>
#include <map>
#include <stack>

#include "resip/stack/Tuple.hxx"
#include "resip/stack/Transport.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/TupleMarkManager.hxx"
#include "rutil/Condition.hxx"
#include "rutil/HeapInstanceCounter.hxx"
#include "rutil/dns/RRVip.hxx"
#include "rutil/dns/DnsStub.hxx"

#ifdef WIN32
#include <Ws2tcpip.h>
#endif

struct hostent;

namespace resip
{
class DnsInterface;
class DnsAAAARecord;
class DnsHandler;
class WhiteListLastCommand;

class EnumResultSink
{
   public:
      virtual void onEnumResult(const DNSResult<DnsNaptrRecord>& result, int order) = 0;
};

class EnumResult : public DnsResultSink
{
   public:
      EnumResult(EnumResultSink& resultSink, int order);
      ~EnumResult();

      // DnsResultSink
      void onDnsResult(const DNSResult<DnsHostRecord>&);
      void onDnsResult(const DNSResult<DnsAAAARecord>&);
      void onDnsResult(const DNSResult<DnsSrvRecord>&);
      void onDnsResult(const DNSResult<DnsNaptrRecord>&);
      void onDnsResult(const DNSResult<DnsCnameRecord>&);

      void onEnumResult(const DNSResult<DnsNaptrRecord>& result);
      void onNaptrResult(const DNSResult<DnsNaptrRecord>& result);

   private:
      EnumResultSink& mResultSink;
      int mOrder;
};

class DnsResult : public DnsResultSink, public EnumResultSink
{
   public:
      RESIP_HeapCount(DnsResult);
      DnsResult(DnsInterface& interfaceObj, DnsStub& dns, RRVip& vip, DnsHandler* handler);
      virtual ~DnsResult();

      typedef enum
      {
         Available, // A result is available now
         Pending,   // More results may be pending 
         Finished,  // No more results available and none pending
         Destroyed  // the associated transaction has been deleted
                     // (ie, this DnsResult will delete itself as soon as it
                     // gets a new result)
      } Type;

      typedef std::vector<Data> DataVector;

      /*! Starts a lookup.  Has the rules for determining the transport
         from a uri as per rfc3263 and then does a NAPTR lookup or an A
         lookup depending on the uri.  Also does ENUM lookups if
         domain matches a configured enum domain.  This call is threadsafe.
         
         @param uri The uri to resolve.
      */
      void lookup(const Uri& uri);

      /*!
         Blacklist the last returned result until the specified time (ms).
         This call is threadsafe.
         
         @param expiry The absolute expiry, in ms, of this blacklist.
         @return true iff the last result could be blacklisted
         @note This is a no-op if no results have been returned.
      */
      bool blacklistLast(UInt64 expiry);
      
      /*!
         Greylist the last returned result until the specified time (ms)
         Greylisting a tuple effectively de-prioritizes it, so it will not be
         tried if there are any non-grey or blacklisted tuples left to try.
         This call is threadsafe.
         
         @param expiry The absolute expiry, in ms, of this blacklist.
         @return true iff the last result could be greylisted
         @note This is a no-op if no results have been returned.
      */
      bool greylistLast(UInt64 expiry);
      
      /*!
         Tries to load the next tuple. If Available is returned, the tuple may
         be accessed using current(). 
         Safe to call after DnsHandler::handle callback has been called.
         
         @return Available if there is a result ready, Pending if it needs to
                  follow an SRV (more results might come in later), or Finished
                  if there are definitively no more results.
         @note ALWAYS call this before calling next()
      */
      Type available();
      
      /*!
         Return the next tuple available for this query. 
         Safe to call after DnsHandler::handle callback has been called.
         
         @return The next Tuple available for this query.
         @note ALWAYS call available() and verify the return is Available
               before calling this function.
         @note This no longer results in the last result being blacklisted. To
               blacklist the last result, use blacklistLast(). 
      */
      Tuple next();

      /*!
         Whitelist the last tuple returned by next(). This means that the path
         to this result (NAPTR->SRV->A/AAAA) will be favored by the resolver 
         from now on. (ie, this NAPTR will be favored above all others that 
         match, even if the order/preference changes in the DNS, and this 
         A/AAAA record will be favored above all others that match, even if new
         ones are added.)
         This call is threadsafe.
         
         @note It can be argued that using this is harmful, since the load-
               leveling capabilities of DNS are ignored from here on.
         @note This will also re-order SRV records, but the order in which
               SRVs arrive is ignored by DnsResult (they are just re-sorted)
      */
      void whitelistLast();

      // return the target of associated query
      // Safe to call after DnsHandler::handle callback has been called.
      Data target() const { return mTarget; }
      unsigned int getSRVResultsSize() const {return (unsigned int)mSRVResults.size();}

      // Will delete this DnsResult if no pending queries are out there or wait
      // until the pending queries get responses and then delete
      void destroy();

      // Used to store a NAPTR result
      class NAPTR
      {
         public:
            NAPTR();
            // As defined by RFC3263
            bool operator<(const NAPTR& rhs) const;

            Data key; // NAPTR record key
            
            int order;
            int pref;
            Data flags;
            Data service;
            DnsNaptrRecord::RegExp regex;
            Data replacement;
      };
      
      class SRV
      {
         public:
            SRV();
            // As defined by RFC3263, RFC2782
            bool operator<(const SRV& rhs) const;
            
            Data key; // SRV record key
            
            int naptrpref;
            TransportType transport;
            int priority;
            int weight;
            int port;
            Data target;
      };

   private:

      /*
         The following command is used to ensure that all DnsInterface mRRVip and 
         mTupleMarkManager, and enum setting accesses are done from the DnsThread.  
         This gets the initial call * lookupInternalWithEnum to occur on the 
         DnsThread (using the DnsStub command fifo).
       */
      class LookupCommand : public DnsStub::Command
      {
      public:
          LookupCommand(DnsResult& dnsResult, const Uri& uri) : mDnsResult(dnsResult), mUri(uri) {}
          virtual ~LookupCommand() {}
          virtual void execute()
          {
              mDnsResult.lookupInternalWithEnum(mUri);
          }
      private:
          DnsResult& mDnsResult;
          Uri mUri;
      };
      friend class LookupCommand;
      void lookupInternalWithEnum(const Uri& uri);
      void lookupInternal(const Uri& uri);

      // Given a transport and port from uri, return the default port to use
      int getDefaultPort(TransportType transport, int port);
      
      void lookupHost(const Data& target);
      
      // compute the cumulative weights for the SRV entries with the lowest
      // priority, then randomly pick according to RFC2782 from the entries with
      // the lowest priority based on weights. When all entries at the lowest
      // priority are chosen, pick the next lowest priority and repeat. After an
      // SRV entry is selected, remove it from mSRVResults
      SRV retrieveSRV();
      
      // Will retrieve the next SRV record and compute the prime the mResults
      // with the appropriate Tuples. 
      void primeResults();
      
   private:
      DnsInterface& mInterface;
      DnsStub& mDnsStub;
      RRVip& mVip;
      DnsHandler* mHandler;
      int mSRVCount;
      Uri mInputUri;
      int mDoingEnum;
      std::map<int,Uri> mEnumDestinations;
      resip::Mutex mEnumDestinationsMutex;
      
      bool mSips;
      Data mTarget;
      Data mSrvKey;
      TransportType mTransport; // current
      int mPort; // current
      
      /*!
         @author bwc
         This is set to true when the RFC 3263 logic has chosen the transport
         it will be using. Incoming SRVs will be filtered according to
         mTransport if mHaveChosenTransport is true. It is VITAL that this
         boolean not change during the phase where we are acquiring/processing
         SRV records, because the state of this boolean denotes whether we
         filtered incoming SRVs or not. (If it changes halfway through, some
         of the SRVs will have been filtered, but some won't, and this will
         break retrieveSRV() )
      */
      bool mHaveChosenTransport;
      
      /*!
         @author bwc
         DnsResult::transition is the ONLY function that should ever touch this
         (This is because we need to notify mInterface when we are done making
         queries, and this is when we transition from either Pending or 
         Available to either Destroyed or Finished.)
      */
      Type mType;

      //Ugly hack
      Data mPassHostFromAAAAtoA;

      void transition(Type t);      
      
      // This is where the current pending (ordered) results are stored. As they
      // are retrieved by calling next(), they are popped from the front of the list
      std::deque<Tuple> mResults;
      std::vector<Tuple> mGreylistedTuples;
      
      // Map of NAPTR records by replacement (ie. SRV lookup key)
      std::map<Data, NAPTR> mTopOrderedNAPTRs;

      // used in determining the next SRV record to use as per rfc2782
      int mCumulativeWeight; // for current priority

      // All SRV records sorted in order of preference
      std::vector<SRV> mSRVResults;
      
      friend class DnsInterface;
      friend EncodeStream& operator<<(EncodeStream& strm, const DnsResult&);
      friend EncodeStream& operator<<(EncodeStream& strm, const DnsResult::SRV&);
      friend EncodeStream& operator<<(EncodeStream& strm, const DnsResult::NAPTR&);

      // DnsResultSink
      void onDnsResult(const DNSResult<DnsHostRecord>&);
      void onDnsResult(const DNSResult<DnsAAAARecord>&);
      void onDnsResult(const DNSResult<DnsSrvRecord>&);
      void onDnsResult(const DNSResult<DnsNaptrRecord>&);
      void onDnsResult(const DNSResult<DnsCnameRecord>&);

      void onEnumResult(const DNSResult<DnsNaptrRecord>& result, int order);
      void onNaptrResult(const DNSResult<DnsNaptrRecord>& result);

      typedef struct
      {
         Data domain;
         int rrType;
         Data value; // stores ip for A/AAAA, target host:port for SRV, and replacement for NAPTR.
      } Item;

      /*!
         @author bwc
         This is a snapshot of mCurrentPath when it was returned last.
         (This will be empty if we haven't returned anything)
         This is primarily used for whitelisting the last returned result.
      */
      std::vector<Item> mLastReturnedPath;
      
      /*!
         @author bwc
         The current DNS path we are working on.
         (ie NAPTR -> SRV -> A/AAAA) There is at most one of these types
         in here at any given time, and they will always be in order.
         This exists solely to allow mLastReturnedPath to be defined.
      */
      std::vector<Item> mCurrentPath;
      bool mHaveReturnedResults;
      void clearCurrPath();
      Tuple mLastResult;

      /*
         The following two commands are to ensure that all DnsInterface mVip 
         and mTupleMarkManager calls are done from the DnsThread (using the 
         DnsStub command fifo).
      */
      class WhitelistCommand : public DnsStub::Command
      {
      public:
         WhitelistCommand(RRVip& vip, std::vector<Item>& path) : mVip(vip), mPath(path) {}
         virtual ~WhitelistCommand() {}
         virtual void execute();
      private:
         RRVip& mVip;
         std::vector<Item> mPath;
      };

      class GreyOrBlacklistCommand : public DnsStub::Command
      {
      public:
         GreyOrBlacklistCommand(RRVip& vip, TupleMarkManager& markManager, Item& pathTop, Tuple& result, UInt64 expiry, TupleMarkManager::MarkType markType) : 
            mVip(vip), mMarkManager(markManager), mPathTop(pathTop), mResult(result), mExpiry(expiry), mMarkType(markType) {}
         virtual ~GreyOrBlacklistCommand() {}
         virtual void execute();
      private:
         RRVip& mVip;
         TupleMarkManager& mMarkManager;
         Item mPathTop;
         Tuple mResult;
         UInt64 mExpiry;
         TupleMarkManager::MarkType mMarkType;
      };
};

EncodeStream& operator<<(EncodeStream& strm, const DnsResult&);
EncodeStream& operator<<(EncodeStream& strm, const DnsResult::SRV&);
EncodeStream& operator<<(EncodeStream& strm, const DnsResult::NAPTR&);

}

#endif

//  Copyright (c) 2003, Jason Fischl 
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
