#ifndef FORK_CONTROL_MESSAGE_HXX
#define FORK_CONTROL_MESSAGE_HXX 1

#include "repro/ProcessorMessage.hxx"
#include "resip/stack/NameAddr.hxx"
#include "rutil/Data.hxx"
#include "rutil/Inserter.hxx"

namespace repro
{

class ForkControlMessage : public ProcessorMessage
{

   public:
      ForkControlMessage( const repro::Processor& proc,
                           const resip::Data& tid,
                           resip::TransactionUser* tuPassed,
                           bool cancelAllClientTransactions=false):
         ProcessorMessage(proc,tid,tuPassed)
      {
         mShouldCancelAll=cancelAllClientTransactions;
      }
      
      ForkControlMessage(const ForkControlMessage& orig):
         ProcessorMessage(orig)
      {
         mShouldCancelAll=orig.mShouldCancelAll;
         mTransactionsToProcess=orig.mTransactionsToProcess;
         mTransactionsToCancel=orig.mTransactionsToCancel;
      }
      
      virtual ~ForkControlMessage(){}

            
      //The transaction that this message will return to.
      virtual const resip::Data& getTransactionId() const
      {
         return mTid;
      }
      
      virtual ForkControlMessage* clone() const 
      {
         return new ForkControlMessage(*this);
      }
      
      virtual EncodeStream& encode(EncodeStream& ostr) const 
      { 
         ostr << "ForkControlMessage("<<mTid<<") " << std::endl;
         ostr << "Transactions to process: " << resip::Inserter(mTransactionsToProcess) << std::endl;
         ostr << "Transactions to cancel: " << resip::Inserter(mTransactionsToCancel) << std::endl;
         ostr << "Should cancel all: " << mShouldCancelAll;
         return ostr; 
      }
      virtual EncodeStream& encodeBrief(EncodeStream& ostr) const { return encode(ostr);}

      std::vector<resip::Data> mTransactionsToProcess;
      std::vector<resip::Data> mTransactionsToCancel;
      bool mShouldCancelAll;
      
      


}; //class

} //namespace repro 
#endif
