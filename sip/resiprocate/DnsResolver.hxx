#ifndef DnsResolver_hxx
#define DnsResolver_hxx

#if defined(__linux__) && !defined(USE_ARES)
#define USE_ARES
#endif

#include <list>
#include <set>

#if defined(USE_ARES)
extern "C"
{
#include "ares.h"
#include "ares_dns.h"
}
#endif

#include "resiprocate/os/HashMap.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/Transport.hxx"
#include "resiprocate/Message.hxx"

namespace Vocal2
{
class SipStack;
class Data;
class Uri;
class Via;

class DnsResolver
{
   public:
      typedef std::list<Transport::Tuple> TupleList;
      typedef std::list<Transport::Tuple>::const_iterator TupleIterator;

      struct Srv
      {
            int priority;
            int weight;
            int port;
            Data host;
            Transport::Type transport;

            bool operator<(const Srv& rhs) const
            {
	       return priority < rhs.priority;
	    }
      };
      typedef std::set<DnsResolver::Srv> SrvSet;
      typedef std::set<DnsResolver::Srv>::const_iterator SrvIterator;

      struct Naptr
      {
	    int order;
	    int pref;
	    Data flags;
	    Data service;
	    Data regex;
	    Data replacement;

            bool operator<(const Naptr& rhs) const
            {
	       if (order != rhs.order)
	       {
		  return pref < rhs.pref;
	       }
	       else
	       {
	          return order < rhs.order;
	       }
	    }
      };
      typedef std::set<DnsResolver::Naptr> NaptrSet;
      typedef std::set<DnsResolver::Naptr>::const_iterator NaptrIterator;

      struct Request
      {
            Request(SipStack& pstack, const Data& ptid, const Data& phost, int pport, Transport::Type ptransport, Data pscheme)
               : stack(pstack),tid(ptid),host(phost),port(pport),transport(ptransport),scheme(pscheme),isFinal(false)
            {
            }
            
            SipStack& stack;
            Data tid;
            Data host;
            int port;
            Transport::Type transport;
	    Data scheme;
	    std::list<Transport::Type> otherTransports;
	    bool isFinal;
      };
     
      class DnsMessage : public Message
      {
         public:
            DnsMessage(const Data& tid) : mTransactionId(tid), isFinal(false) {}
            virtual const Data& getTransactionId() const { return mTransactionId; }
            virtual Data brief() const;
            virtual std::ostream& encode(std::ostream& strm) const;
            
            Data mTransactionId;
            TupleList mTuples;
            SrvSet mSrvs;
	    bool isFinal;
      };
      
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line) : BaseException(msg,file,line){}
            const char* name() const { return "DnsResolver::Exception"; }
      };

 
      DnsResolver(SipStack& stack);
      ~DnsResolver();

      void process(FdSet& fdset);
      void buildFdSet(FdSet& fdset);

      void lookup(const Data& transactionId, const Uri& url);
      void lookup(const Data& transactionId, const Via& via);

      void lookupARecords(const Data& transactionId, 
                          const Data& host, 
                          int port, 
                          Transport::Type transport);
      
      static bool isIpAddress(const Data& data);
      
      // probably not going to be supported by ares so remove
      //void stop(const Data& tid);

   private:
#if defined(USE_ARES)
      ares_channel mChannel;
      static void aresCallbackHost(void *arg, int status, struct hostent* host);
      static void aresCallbackSrvTcp(void *arg, int status, unsigned char *abuf, int alen);
      static void aresCallbackSrvUdp(void *arg, int status, unsigned char *abuf, int alen);
      static void aresCallbackNaptr(void *arg, int status, unsigned char *abuf, int alen);
#endif

      SipStack& mStack;
};
      

}

#endif
