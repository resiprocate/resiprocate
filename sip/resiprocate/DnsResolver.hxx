#ifndef DnsResolver_hxx
#define DnsResolver_hxx

#include <list>

#include <sipstack/Transport.hxx>
#include <util/HashMap.hxx>

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
      typedef TupleList::iterator TupleIterator;
      
      class Entry
      {
         public:
            Entry(const Data& tid) 
               : transactionId(tid) 
            {}

            Data transactionId;
            TupleList tupleList;
      };
      
      typedef enum
      {
         NotStarted,
         Waiting,
         PartiallyComplete,
         Complete
      } State;
      
      typedef Entry* Id;

      DnsResolver(SipStack& stack) 
           : mStack(stack)
      {}

      ~DnsResolver();

      void lookup(const Data& transactionId, const Uri& url);
      void lookup(const Data& transactionId, const Via& via);

      Id lookupARecords(const Data& transactionId, const Data& host, int port, 
                        Transport::Type transport, bool complete,
                        Id id = 0);

      static bool isIpAddress(const Data& data);
      
      void stop(Id id);
   private:
      SipStack& mStack;
};
      

}

#endif
