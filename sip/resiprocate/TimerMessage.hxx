#ifndef TimerMessage_hxx
#define TimerMessage_hxx

#include <sipstack/Message.hxx>
#include <sipstack/Data.hxx>
#include <sipstack/Timer.hxx>
#include <iostream>

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
