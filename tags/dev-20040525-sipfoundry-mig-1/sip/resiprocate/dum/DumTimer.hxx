#if !defined(RESIP_DUMTIMER_HXX)
#define RESIP_DUMTIMER_HXX 

#include <iostream>
#include "resiprocate/Message.hxx"

namespace resip
{
class Data;

class DumTimer : public Message
{
   public:
      typedef enum
      {
         Session,
         Registration,
         Provisional1,
         Provisional2,
         Publication,
         Retransmit200
      } Type;

      DumTimer(Type type, unsigned long duration, int cseq, int rseq=-1);
      ~DumTimer();
      
      Type type() const;
      int cseq() const;
      int rseq() const;
      
      virtual const Data& getTransactionId() const;
      virtual bool isClientTransaction() const;
      
      virtual Data brief() const;
      virtual std::ostream& encode(std::ostream& strm) const;
      
   private:
      Type mType;
      unsigned long mDuration;
      int mCseq;
      int mRseq;
};

}

#endif
