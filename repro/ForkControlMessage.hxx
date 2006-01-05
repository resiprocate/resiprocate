#ifndef FORK_CONTROL_MESSAGE_HXX
#define FORK_CONTROL_MESSAGE_HXX 1

#include "resip/stack/ApplicationMessage.hxx"
#include "resip/stack/NameAddr.hxx"
#include "rutil/Data.hxx"

namespace repro
{

class ForkControlMessage : public resip::ApplicationMessage
{

   public:
      ForkControlMessage(const resip::Data& tid, 
                           bool cancelAllClientTransactions=false)
      {
         mTid=tid;
         mShouldCancelAllClientTransactions=cancelAllClientTransactions;
      }
      
      virtual ~ForkControlMessage(){}

      
      // Adds a target to be processed when this message is received
      // by the RequestContest.
      virtual void pushTarget(const resip::NameAddr& target)
      {
         mTargetsToProcess.push_back(target);
      }

      virtual resip::NameAddr popTarget()
      {
         resip::NameAddr target = mTargetsToProcess.back();
         mTargetsToProcess.pop_back();
         return target;
      }
      
      virtual bool targetsRemain() const
      {
         return !mTargetsToProcess.empty();
      }
      
      
      // Adds a client transaction to be cancelled when this message
      // is received by the RequestContext.
      virtual void pushTid(const resip::Data& tid)
      {
         mClientTransactionsToCancel.push_back(tid);
      }
      
      virtual resip::Data popTid()
      {
         resip::Data tid = mClientTransactionsToCancel.back();
         mClientTransactionsToCancel.pop_back();
         return tid;
      }
      
      virtual bool tidsRemain() const
      {
         return !mClientTransactionsToCancel.empty();
      }
      
      //Specifies whether ALL proceeding client transactions should
      //be cancelled when this message is received by the RequestContext.
      //Accessor function
      virtual bool& cancelAll()
      {
         return mShouldCancelAllClientTransactions;
      }
      
      //The transaction that this message will return to.
      virtual const resip::Data& getTransactionId() const
      {
         return mTid;
      }
      
      virtual ForkControlMessage* clone() const 
      {
         ForkControlMessage* clone = new ForkControlMessage(mTid,mShouldCancelAllClientTransactions);
         clone->mTargetsToProcess=mTargetsToProcess;
         clone->mClientTransactionsToCancel=mClientTransactionsToCancel;
         return clone;
      }
      virtual std::ostream& encode(std::ostream& ostr) const { ostr << "ForkControlMessage("<<mTid<<") "; return ostr; }
      virtual std::ostream& encodeBrief(std::ostream& ostr) const { return encode(ostr);}
      
      
   private:
      std::vector<resip::NameAddr> mTargetsToProcess;
      std::vector<resip::Data> mClientTransactionsToCancel;
      bool mShouldCancelAllClientTransactions;
      resip::Data mTid;


}; //class

} //namespace repro 
#endif
