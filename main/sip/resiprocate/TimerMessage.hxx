#ifndef TimerMessage_hxx
#define TimerMessage_hxx

#include <sipstack/Message.hxx>
#include <sipstack/Data.hxx>
#include <sipstack/Timer.hxx>

namespace Vocal2
{

class TimerMessage : public Message
{
   public:
      TimerMessage(Data transactionId, Timer::Type type)
         : mTransactionId(transactionId),
           mType(type)
      {}

      virtual const Data& getTransactionId() const
      {
         return mTransactionId;
      }

      Timer::Type getType()
      {
         return mType;
      }
      
   private:
      Data mTransactionId;
      Timer::Type mType;
};

}


#endif
