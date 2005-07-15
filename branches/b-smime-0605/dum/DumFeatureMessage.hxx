#if !defined(RESIP_DumFeatureMessage_hxx)
#define RESIP_DumFeatureMessage_hxx 

#include <iosfwd>
#include "resiprocate/Message.hxx"

namespace resip
{

class DumFeatureMessage : public Message
{
   public:
      DumFeatureMessage(const Data& tid);      
      DumFeatureMessage(const DumFeatureMessage&);      
      ~DumFeatureMessage();

      Message* clone() const;

      virtual std::ostream& encode(std::ostream& strm) const;
      /// output a brief description to stream
      virtual std::ostream& encodeBrief(std::ostream& str) const;

      virtual const Data& getTransactionId() const { return mTransactionId; }
   private:
      Data mTransactionId;      
};

}

#endif
