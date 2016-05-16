#ifndef RESIP_DNS_STUB_HXX
#define RESIP_DNS_STUB_HXX

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <vector>
#include <list>
#include <map>
#include <set>

#include "rutil/FdPoll.hxx"
#include "rutil/Fifo.hxx"
#include "rutil/GenericIPAddress.hxx"
#include "rutil/SelectInterruptor.hxx"
#include "rutil/Socket.hxx"
#include "rutil/dns/DnsResourceRecord.hxx"
#include "rutil/dns/DnsAAAARecord.hxx"
#include "rutil/dns/DnsCnameRecord.hxx"
#include "rutil/dns/DnsHostRecord.hxx"
#include "rutil/dns/DnsNaptrRecord.hxx"
#include "rutil/dns/DnsSrvRecord.hxx"
#include "rutil/dns/RRCache.hxx"
#include "rutil/dns/RROverlay.hxx"
#include "rutil/dns/ExternalDns.hxx"
#include "rutil/AsyncProcessHandler.hxx"


namespace resip
{
class FdPollGrp;

class GetDnsCacheDumpHandler
{
   public:
      GetDnsCacheDumpHandler() {}
      virtual ~GetDnsCacheDumpHandler() {}
      virtual void onDnsCacheDumpRetrieved(std::pair<unsigned long, unsigned long> key, const Data& dnsCache) = 0;
};

template<typename T>
class DNSResult
{
   public:
      Data domain;
      int status;
      Data msg;
      std::vector<T> records;
      EncodeStream& dump(EncodeStream& strm) const
      {
         if (status == 0)
         {
            for (typename std::vector<T>::const_iterator i=records.begin(); i != records.end(); ++i)
            {
               if (i != records.begin())
               {
                  strm << ", ";
               }
               i->dump(strm);
            }
         }
         else
         {
            strm << domain << " lookup failed: " << msg;
         }

         return strm;
      }
};

template<class T>
EncodeStream& operator<<(EncodeStream& strm, const DNSResult<T>& r)
{
   r.dump(strm);
   return strm;
}

class DnsResultSink
{
   public:
      virtual ~DnsResultSink() {}
      virtual void onDnsResult(const DNSResult<DnsHostRecord>&) = 0;
      virtual void onLogDnsResult(const DNSResult<DnsHostRecord>&);

// DnsAAAARecord will basically be non-functional if USE_IPV6 wasn't set in the
// build.
      virtual void onDnsResult(const DNSResult<DnsAAAARecord>&) = 0;
      virtual void onLogDnsResult(const DNSResult<DnsAAAARecord>&);

      virtual void onDnsResult(const DNSResult<DnsSrvRecord>&) = 0;
      virtual void onLogDnsResult(const DNSResult<DnsSrvRecord>&);

      virtual void onDnsResult(const DNSResult<DnsNaptrRecord>&) = 0;
      virtual void onLogDnsResult(const DNSResult<DnsNaptrRecord>&);

      virtual void onDnsResult(const DNSResult<DnsCnameRecord>&) = 0;
      virtual void onLogDnsResult(const DNSResult<DnsCnameRecord>&);
};

class DnsRawSink
{
   public:
      virtual ~DnsRawSink() {}
      virtual void onDnsRaw(int statuts, const unsigned char* abuf, int len) = 0;
};

class DnsStub : public ExternalDnsHandler
{
   public:
      typedef RRCache::Protocol Protocol;
      typedef std::vector<Data> DataArr;
      typedef std::vector<DnsResourceRecord*> DnsResourceRecordsByPtr;
      typedef std::vector<GenericIPAddress> NameserverList;

      static NameserverList EmptyNameserverList;

      class Command
      {
         public:
            virtual ~Command() {}
            virtual void execute() = 0;
      };

      class ResultTransform
      {
         public:
            virtual ~ResultTransform() {}
            virtual void transform(const Data& target, int rrType, DnsResourceRecordsByPtr& src) = 0;
      };

      class DnsStubException : public BaseException
      {
         public:
            DnsStubException(const Data& msg, const Data& file, const int line)
               : BaseException(msg, file, line) 
            {
            }
            
            const char* name() const { return "DnsStubException"; }
      };

      DnsStub(const NameserverList& additional = EmptyNameserverList,
              AfterSocketCreationFuncPtr socketFunc = 0,
              AsyncProcessHandler* asyncProcessHandler = 0,
              FdPollGrp *pollGrp = 0);
      ~DnsStub();

      // call this method before you create SipStack if you'd like to change the
      // default DNS lookup timeout and number of retries.
      static void setDnsTimeoutAndTries(int timeoutInSec, int tries)
      {
         mDnsTimeout = timeoutInSec;
         mDnsTries = tries;
      }
      static void enableDnsFeatures(unsigned int features)  {mDnsFeatures |= features;} // bit mask of ExternalDns::Features

      void setResultTransform(ResultTransform*);
      void removeResultTransform();

      /*!
         @param enumSuffixes If the uri is enum searchable, this is the list of
                  enum suffixes (for example "e164.arpa") that will be used in
                  the attempt to resolve this uri.
      */
      void setEnumSuffixes(const std::vector<Data>& suffixes);
      const std::vector<Data>& getEnumSuffixes() const;

      /*!
         @param enumDomains The ENUM possibility is only considered if
                the URI domain part is one of these domains
      */
      void setEnumDomains(const std::map<Data,Data>& domains);
      const std::map<Data,Data>& getEnumDomains() const;

      void clearDnsCache();
      void logDnsCache();
      void getDnsCacheDump(std::pair<unsigned long, unsigned long> key, GetDnsCacheDumpHandler* handler);
      void setDnsCacheTTL(int ttl);
      void setDnsCacheSize(int size);
      bool checkDnsChange();
      bool supportedType(int);

      template<class QueryType> void lookup(const Data& target, DnsResultSink* sink)
      {
         lookup<QueryType>(target, Protocol::Reserved, sink);
      }

      // There are three pre-defined protocols (see RRList.hxx). Zero(0) is
      // reserved for internal use, so do not use 0. If you'd like to blacklist
      // for different types of protocols, just pass in any integer other than
      // those used for pre-defined protocols.
      // ?slg? Should we offer a non-queuing version of this - currently all resip lookup
      //       requests are from the DnsThread context anyway, so there is no need to queue
      //       the request to the fifo.
      template<class QueryType> void lookup(const Data& target, int protocol, DnsResultSink* sink)
      {
         QueryCommand<QueryType>* command = new QueryCommand<QueryType>(target, protocol, sink, *this);
         queueCommand(command);
      }

      virtual void handleDnsRaw(ExternalDnsRawResult);

      virtual void process(FdSet& fdset);
      virtual unsigned int getTimeTillNextProcessMS();
      virtual void buildFdSet(FdSet& fdset);
      void setPollGrp(FdPollGrp* pollGrp);
      void processTimers();

      virtual void queueCommand(Command* command);

  private:
      void processFifo();

   protected:
      void cache(const Data& key, in_addr addr);
      void cache(const Data& key, const unsigned char* abuf, int alen);
      void cacheTTL(const Data& key, int rrType, int status, const unsigned char* abuf, int alen);

   private:
      class ResultConverter //.dcm. -- flyweight?
      {
         public:
            virtual void notifyUser(const Data& target, 
                                    int status, 
                                    const Data& msg,
                                    const DnsResourceRecordsByPtr& src,
                                    DnsResultSink* sink) = 0;
            virtual ~ResultConverter() {}
      };
      
      template<class QueryType>  
      class ResultConverterImpl : public ResultConverter
      {
         public:
            virtual void notifyUser(const Data& target, 
                                    int status, 
                                    const Data& msg,
                                    const DnsResourceRecordsByPtr& src,
                                    DnsResultSink* sink)
            {
               resip_assert(sink);
               DNSResult<typename QueryType::Type>  result;
               for (unsigned int i = 0; i < src.size(); ++i)
               {
                  result.records.push_back(*(dynamic_cast<typename QueryType::Type*>(src[i])));
               }
               result.domain = target;
               result.status = status;
               result.msg = msg;
               sink->onLogDnsResult(result);
               sink->onDnsResult(result);
            }
      };

      class Query : public DnsRawSink
      {
         public:
            Query(DnsStub& stub, ResultTransform* transform, ResultConverter* resultConv, 
                  const Data& target, int rrType, bool followCname, int proto, DnsResultSink* s);
            virtual ~Query();

            enum {MAX_REQUERIES = 5};

            void go();
            void process(int status, const unsigned char* abuf, const int alen);
            void onDnsRaw(int status, const unsigned char* abuf, int alen);
            void followCname(const unsigned char* aptr, const unsigned char*abuf, const int alen, bool& bGotAnswers, bool& bDeleteThis, Data& targetToQuery);

         private:
            static DnsResourceRecordsByPtr Empty;
            int mRRType;
            DnsStub& mStub;
            ResultTransform* mTransform;
            ResultConverter* mResultConverter;
            Data mTarget;
            int mProto;
            int mReQuery;
            DnsResultSink* mSink;
            bool mFollowCname;
      };

   private:
      DnsStub(const DnsStub&);   // disable copy ctor.
      DnsStub& operator=(const DnsStub&);

   public:
      // sailesh - due to a bug in CodeWarrior,
      // QueryCommand::execute() can only access this method
      // if it's public. Even using "friend" doesn't work.
      template<class QueryType>
      void query(const Data& target, int proto, DnsResultSink* sink)
      {
         Query* query = new Query(*this, mTransform, 
                                  new ResultConverterImpl<QueryType>(), 
                                  target, QueryType::getRRType(),
                                  QueryType::SupportsCName, proto, sink);
         mQueries.insert(query);
         query->go();
      }
      
   private:

      template<class QueryType>
      class QueryCommand : public Command
      {
         public:
            QueryCommand(const Data& target, 
                         int proto,
                         DnsResultSink* sink,
                         DnsStub& stub)
               : mTarget(target),
                 mProto(proto),
                 mSink(sink),
                 mStub(stub)
            {}

            ~QueryCommand() {}

            void execute()
            {
               mStub.query<QueryType>(mTarget, mProto, mSink);
            }

         private:
            Data mTarget;
            int mProto;
            DnsResultSink* mSink;
            DnsStub& mStub;
      };

      void doSetEnumSuffixes(const std::vector<Data>& suffixes);
      void doSetEnumDomains(const std::map<Data,Data>& domains);

      class SetEnumSuffixesCommand : public Command
      {
         public:
            SetEnumSuffixesCommand(DnsStub& stub, 
                                   const std::vector<Data>& suffixes)
               : mStub(stub),
                 mEnumSuffixes(suffixes)
            {}             
            ~SetEnumSuffixesCommand() {}
            void execute()
            {
               mStub.doSetEnumSuffixes(mEnumSuffixes);
            }

         private:
            DnsStub& mStub;
            std::vector<Data> mEnumSuffixes;
      };

      class SetEnumDomainsCommand : public Command
      {
         public:
            SetEnumDomainsCommand(DnsStub& stub,
                                   const std::map<Data,Data>& domains)
               : mStub(stub),
                 mEnumDomains(domains)
            {}
            ~SetEnumDomainsCommand() {}
            void execute()
            {
               mStub.doSetEnumDomains(mEnumDomains);
            }

         private:
            DnsStub& mStub;
            std::map<Data,Data> mEnumDomains;
      };

      void doClearDnsCache();

      class ClearDnsCacheCommand : public Command
      {
         public:
            ClearDnsCacheCommand(DnsStub& stub)
               : mStub(stub)
            {}             
            ~ClearDnsCacheCommand() {}
            void execute()
            {
               mStub.doClearDnsCache();
            }

         private:
            DnsStub& mStub;
      };

      void doLogDnsCache();

      class LogDnsCacheCommand : public Command
      {
         public:
            LogDnsCacheCommand(DnsStub& stub)
               : mStub(stub)
            {}             
            ~LogDnsCacheCommand() {}
            void execute()
            {
               mStub.doLogDnsCache();
            }

         private:
            DnsStub& mStub;
      };

      void doGetDnsCacheDump(std::pair<unsigned long, unsigned long> key, GetDnsCacheDumpHandler* handler);

      class GetDnsCacheDumpCommand : public Command
      {
         public:
            GetDnsCacheDumpCommand(DnsStub& stub, std::pair<unsigned long, unsigned long> key, GetDnsCacheDumpHandler* handler)
               : mStub(stub), mKey(key), mHandler(handler)
            {}             
            ~GetDnsCacheDumpCommand() {}
            void execute()
            {
               mStub.doGetDnsCacheDump(mKey, mHandler);
            }

         private:
            DnsStub& mStub;
            std::pair<unsigned long, unsigned long> mKey;
            GetDnsCacheDumpHandler* mHandler;
      };

      SelectInterruptor mSelectInterruptor;
      FdPollItemHandle mInterruptorHandle;

      resip::Fifo<Command> mCommandFifo;

      const unsigned char* skipDNSQuestion(const unsigned char *aptr,
                                           const unsigned char *abuf,
                                           int alen);
      const unsigned char* createOverlay(const unsigned char* abuf, 
                                         const int alen, 
                                         const unsigned char* aptr, 
                                         std::vector<RROverlay>&,
                                         bool discard=false);
      void removeQuery(Query*);
      void lookupRecords(const Data& target, unsigned short type, DnsRawSink* sink);
      Data errorMessage(int status);

      ResultTransform* mTransform;
      ExternalDns* mDnsProvider;
      FdPollGrp* mPollGrp;
      std::set<Query*> mQueries;

      std::vector<Data> mEnumSuffixes; // where to do enum lookups
      std::map<Data,Data> mEnumDomains;

      static int mDnsTimeout; // in seconds
      static int mDnsTries;
      static unsigned int mDnsFeatures;    // bit mask of ExternalDns::Features

      /// if this object exists, it gets notified when ApplicationMessage's get posted
      AsyncProcessHandler* mAsyncProcessHandler;

      /// Dns Cache
      RRCache mRRCache;
};

typedef DnsStub::Protocol Protocol;

}

#endif
 
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
 
