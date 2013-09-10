#if !defined(RESIP_DumFeatureMessage_hxx)
#define RESIP_DumFeatureMessage_hxx 

#include <iosfwd>
#include "resip/stack/ApplicationMessage.hxx"

namespace resip
{

//!dcm! -- what is the intent of ApplicationMessage, especially as used in
//repro? Is this really what ApplicationMessage should be(always has tid)

class DumFeatureMessage : public ApplicationMessage
{
   public:
      DumFeatureMessage(const Data& tid);      
      DumFeatureMessage(const DumFeatureMessage&);      
      ~DumFeatureMessage();

      Message* clone() const;

      virtual EncodeStream& encode(EncodeStream& strm) const;
      /// output a brief description to stream
      virtual EncodeStream& encodeBrief(EncodeStream& str) const;

      virtual const Data& getTransactionId() const { return mTransactionId; }
   private:
      Data mTransactionId;      
};

}

#endif
