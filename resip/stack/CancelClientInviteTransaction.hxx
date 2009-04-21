#ifndef CancelClientInviteTransaction_Include_Guard
#define CancelClientInviteTransaction_Include_Guard

#include "resip/stack/TransactionMessage.hxx"

#include "rutil/Data.hxx"

namespace resip
{
class CancelClientInviteTransaction : public TransactionMessage
{
   public:
      explicit CancelClientInviteTransaction(const resip::Data& tid) :
         mTid(tid)
      {}
      virtual ~CancelClientInviteTransaction(){}

/////////////////// Must implement unless abstract ///

      virtual const Data& getTransactionId() const {return mTid;}
      virtual bool isClientTransaction() const {return true;}
      virtual EncodeStream& encode(EncodeStream& strm) const
      {
         return strm << "CancelClientInviteTransaction: " << mTid;
      }
      virtual EncodeStream& encodeBrief(EncodeStream& strm) const
      {
         return strm << "CancelClientInviteTransaction: " << mTid;
      }

/////////////////// May override ///

      virtual Message* clone() const
      {
         return new CancelClientInviteTransaction(*this);
      }

   protected:
      const resip::Data mTid;

}; // class CancelClientInviteTransaction

} // namespace resip

#endif // include guard
