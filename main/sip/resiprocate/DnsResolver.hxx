#ifndef DnsResolver_hxx
#define DnsResolver_hxx

#include <list>
#if defined(__linux__)
extern "C"
{
#include <ares.h>
}
#endif

#include "sip2/util/HashMap.hxx"
#include "sip2/util/BaseException.hxx"
#include "sip2/sipstack/Transport.hxx"
#include "sip2/sipstack/Message.hxx"

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

      class DnsMessage : public Message
      {
         public:
            DnsMessage(const Data& tid) : mTransactionId(tid) {}
            virtual const Data& getTransactionId() const { return mTransactionId; }
            virtual Data brief() const;
            virtual std::ostream& encode(std::ostream& strm) const;
            
            Data mTransactionId;
            TupleList mTuples;
      };
      
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line) : BaseException(msg,file,line){}
            const char* name() const { return "DnsResolver::Exception"; }
      };

      struct Request
      {
            Request(SipStack& pstack, const Data& ptid, const Data& phost, int pport, Transport::Type ptransport) 
               : stack(pstack), tid(ptid),host(phost),port(pport),transport(ptransport)
            {
            }
            
            SipStack& stack;
            Data tid;
            Data host;
            int port;
            Transport::Type transport;
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
      static void aresCallback(void *arg, int status, unsigned char *abuf, int alen);
      static void aresCallback2(void *arg, int status, struct hostent* host);

      SipStack& mStack;
#if defined(__linux__)
      ares_channel mChannel;
#endif
};
      

}

#endif
