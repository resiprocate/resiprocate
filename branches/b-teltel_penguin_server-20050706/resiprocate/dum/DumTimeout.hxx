#if !defined(RESIP_DUMTIMER_HXX)
#define RESIP_DUMTIMER_HXX 

#include <iosfwd>
#include "resiprocate/ApplicationMessage.hxx"
#include "resiprocate/dum/Handles.hxx"

namespace resip
{

class DumTimeout : public ApplicationMessage
{
   public:
      typedef enum
      {
         SessionExpiration,
         SessionRefresh,
         Registration,
         Provisional1,
         Provisional2,
         Publication,
         Retransmit200,
         Retransmit1xx,
         WaitForAck, // UAS gets no ACK
         CanDiscardAck,
         StaleCall, // UAC gets no final response
         Subscription,
         StaleReInvite,
         Glare,
         Cancelled,
         Forked
      } Type;
      static const unsigned long StaleCallTimeout;

      DumTimeout(Type type, unsigned long duration, BaseUsageHandle target,  int seq, int aseq = -1);
      DumTimeout(const DumTimeout&);      
      ~DumTimeout();

      Message* clone() const;
      
      Type type() const;
      int seq() const;
      int secondarySeq() const;

      BaseUsageHandle getBaseUsage() const;

      virtual const Data& getTransactionId() const;
      virtual bool isClientTransaction() const;
      
      virtual Data brief() const;
      virtual std::ostream& encode(std::ostream& strm) const;
      virtual std::ostream& encodeBrief(std::ostream& str) const;
     
   private:
      Type mType;
      unsigned long mDuration;
      BaseUsageHandle mUsageHandle;
      int mSeq;
      int mSecondarySeq;

};

}

#endif
