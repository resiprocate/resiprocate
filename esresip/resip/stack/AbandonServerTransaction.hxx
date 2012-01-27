#ifndef AbandonServerTransaction_Include_Guard
#define AbandonServerTransaction_Include_Guard

#include "resip/stack/TransactionMessage.hxx"

#include "rutil/Data.hxx"

namespace resip
{
/**
   @internal

   Message used by the stack to tell the TransactionController to abandon a 
   server transaction. See SipStack::abandonServerTransaction()
*/
class AbandonServerTransaction : public TransactionMessage
{
   public:
      AbandonServerTransaction(const Data& tid) :
         mTid(tid)
      {}
      virtual ~AbandonServerTransaction() {}

/////////////////// Must implement unless abstract ///

      virtual const Data& getTransactionId() const {return mTid;}
      virtual bool isClientTransaction() const {return false;}
      virtual std::ostream& encode(std::ostream& strm) const
      {
         return strm << "AbandonServerTransaction: " << mTid;
      }
      virtual std::ostream& encodeBrief(std::ostream& strm) const
      {
         return strm << "AbandonServerTransaction: " << mTid;
      }

/////////////////// May override ///

      virtual Message* clone() const
      {
         return new AbandonServerTransaction(*this);
      }

   protected:
      const resip::Data mTid;

}; // class AbandonServerTransaction

} // namespace resip

#endif // include guard
