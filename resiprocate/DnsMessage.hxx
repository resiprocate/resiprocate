#ifndef DnsMessage_hxx
#define DnsMessage_hxx

#include <sipstack/DnsResolver.hxx>


namespace Vocal2 
{

class DnsMessage : public Message
{
   public:
      DnsMessage(Data transactionId, DnsResolver::TupleIterator start, DnsResolver::TupleIterator end, bool isComplete)
         : mTransactionId(transactionId), mStart(start), mEnd(end), mIsComplete(isComplete)
      {}

      virtual const Data& getTransactionId() const { return mTransactionId; }

      bool complete() const { return mIsComplete; }

      DnsResolver::TupleIterator begin() { return mStart; }
      DnsResolver::TupleIterator end() { return mEnd; }
      
      virtual Data brief() const;
      virtual std::ostream& encode(std::ostream& strm) const;

   private:
      Data mTransactionId;
      DnsResolver::TupleIterator mStart;
      DnsResolver::TupleIterator mEnd;
      bool mIsComplete;
};


}


#endif
