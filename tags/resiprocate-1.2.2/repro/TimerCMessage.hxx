#ifndef TIMER_C_MESSAGE_HXX
#define TIMER_C_MESSAGE_HXX 1

#include "resip/stack/ApplicationMessage.hxx"

#include "rutil/Data.hxx"
#include <time.h>

namespace repro
{

class TimerCMessage : public resip::ApplicationMessage
{
   public:
      TimerCMessage(resip::Data tid,int serial):
         mSerial(serial),
         mTid(tid)
      {}

      ~TimerCMessage(){}

      virtual const resip::Data& getTransactionId() const { return mTid; }
      virtual TimerCMessage* clone() const {return new TimerCMessage(mTid,mSerial);}
      virtual std::ostream& encode(std::ostream& ostr) const { ostr << "TimerCMessage("<<mTid<<") "; return ostr; }
      virtual std::ostream& encodeBrief(std::ostream& ostr) const { return encode(ostr);}

      int mSerial;

   private:
      TimerCMessage(){}
      resip::Data mTid;

};
}
#endif
