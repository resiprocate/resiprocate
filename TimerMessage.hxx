#ifndef TimerMessage_hxx
#define TimerMessage_hxx

#include <iostream>
#include <sipstack/Message.hxx>
#include <util/Timer.hxx>
#include <util/Data.hxx>

namespace Vocal2
{

class TimerMessage : public Message
{
   public:
      TimerMessage(Data transactionId, Timer::Type type, unsigned long duration)
         : mTransactionId(transactionId),
           mType(type),
           mDuration(duration) {}
      ~TimerMessage();

      virtual const Data& getTransactionId() const
      {
         return mTransactionId;
      }

      Timer::Type getType() const
      {
         return mType;
      }

      unsigned long getDuration() const 
      {
         return mDuration;
      }
      

      virtual Data brief() const;
      virtual std::ostream& dump(std::ostream& strm) const;
      
   private:
      Data mTransactionId;
      Timer::Type mType;
      unsigned long mDuration;
};

}


#endif
