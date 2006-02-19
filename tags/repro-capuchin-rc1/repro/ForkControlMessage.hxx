#ifndef FORK_CONTROL_MESSAGE_HXX
#define FORK_CONTROL_MESSAGE_HXX 1

#include "repro/ProcessorMessage.hxx"
#include "resip/stack/NameAddr.hxx"
#include "rutil/Data.hxx"

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
      
      virtual std::ostream& encode(std::ostream& ostr) const { ostr << "ForkControlMessage("<<mTid<<") "; return ostr; }
      virtual std::ostream& encodeBrief(std::ostream& ostr) const { return encode(ostr);}

      std::vector<resip::Data> mTransactionsToProcess;
      std::vector<resip::Data> mTransactionsToCancel;
      bool mShouldCancelAll;
      
      


}; //class

} //namespace repro 
#endif
