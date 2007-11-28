#ifndef RESIP_DNS_STUB_HXX
#define RESIP_DNS_STUB_HXX

#include <vector>
#include <list>
#include <map>
#include <set>

#include "rutil/Fifo.hxx"
#include "rutil/GenericIPAddress.hxx"
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


namespace resip
{

template<typename T>
class DNSResult
{
   public:
      Data domain;
      int status;
      Data msg;
      std::vector<T> records;
      std::ostream& dump(std::ostream& strm) const
      {
         if (status == 0)
         {
            for (typename std::vector<T>::const_iterator i=records.begin(); i != records.end(); ++i)
            {
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
std::ostream& operator<<(std::ostream& strm, const DNSResult<T>& r)
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

#ifdef USE_IPV6
      virtual void onDnsResult(const DNSResult<DnsAAAARecord>&) = 0;
      virtual void onLogDnsResult(const DNSResult<DnsAAAARecord>&);
#endif

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
              AfterSocketCreationFuncPtr socketFunc = 0);
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
      void setEnumSuffixes(const std::vector<Data>& suffixes);
      const std::vector<Data>& getEnumSuffixes() const;
      void clearDnsCache();
      void logDnsCache();

      template<class QueryType> void lookup(const Data& target, DnsResultSink* sink)
      {
         lookup<QueryType>(target, Protocol::Reserved, sink);
      }

      // There are three pre-defined protocols (see RRList.hxx). Zero(0) is
      // reserved for internal use, so do not use 0. If you'd like to blacklist
      // for different types of protocols, just pass in any integer other than
      // those used for pre-defined protocols.
      //
      template<class QueryType> void lookup(const Data& target, int protocol, DnsResultSink* sink)
      {
         QueryCommand<QueryType>* command = new QueryCommand<QueryType>(target, protocol, sink, *this);
         mCommandFifo.add(command);
      }

      void process(FdSet& fdset);
      bool requiresProcess();
      void buildFdSet(FdSet& fdset);

      virtual void handleDnsRaw(ExternalDnsRawResult);
      
   protected:
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
               assert(sink);
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

      class Command
      {
         public:
            virtual ~Command() {}
            virtual void execute() = 0;
      };


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

      resip::Fifo<Command> mCommandFifo;

      const unsigned char* skipDNSQuestion(const unsigned char *aptr,
                                           const unsigned char *abuf,
                                           int alen);
      bool supportedType(int);
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
      std::set<Query*> mQueries;

      std::vector<Data> mEnumSuffixes; // where to do enum lookups

      static int mDnsTimeout; // in seconds
      static int mDnsTries;
      static unsigned int mDnsFeatures;    // bit mask of ExternalDns::Features
};

typedef DnsStub::Protocol Protocol;

}

#endif
 
