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

      virtual Data& getTransactionId() const;

   private:
      Data mTransactionId;
      Timer::Type mType;
};

}


#endif
