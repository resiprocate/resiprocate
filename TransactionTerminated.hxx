#if !defined(TransactionTerminated_hxx)
#define TransactionTerminated_hxx
// Copyright 2002 Cathay Networks, Inc. 

#include "Message.hxx"

namespace resip
{

class TransactionTerminated : public Message
{
   public:
      TransactionTerminated(const Data& tid) : mTransactionId(tid) {}
      virtual const Data& getTransactionId() const { return mTransactionId; }
      virtual Data brief() const { return Data("TransactionTerminated ") + mTransactionId; }
      virtual std::ostream& encode(std::ostream& strm) const { return strm << brief(); }
      
      Data mTransactionId;
};
 
}

#endif
