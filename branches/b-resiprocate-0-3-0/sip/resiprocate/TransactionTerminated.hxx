#if !defined(RESIP_TRANSACTIONTERMINATED_HXX)
#define RESIP_TRANSACTIONTERMINATED_HXX 

// Copyright 2002 Cathay Networks, Inc. 

#include "Message.hxx"

namespace resip
{

class TransactionTerminated : public Message
{
   public:
      TransactionTerminated(const Data& tid, bool isClient) : mTransactionId(tid), mIsClient(isClient) {}
      virtual const Data& getTransactionId() const { return mTransactionId; }
      virtual bool isClientTransaction() const { return mIsClient; }

      virtual Data brief() const { return (mIsClient ? Data("ClientTransactionTerminated ") : Data("ServerTransactionTerminated ")) + mTransactionId; }
      virtual std::ostream& encode(std::ostream& strm) const { return strm << brief(); }
      
      Data mTransactionId;
      bool mIsClient;
};
 
}

#endif
