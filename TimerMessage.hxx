#ifndef TimerMessage_hxx
#define TimerMessage_hxx

#include <sip2/Message.hxx>
#include <sip2/Data.hxx>

namespace Vocal2
{
class TimerMessage : public Message
{
   public:
      TimerMessage(Data transactionId,
                   Name name)
         : mTransactionId(transactionId),
           mName(name)
      {}
      
      enum Name 
      {
         Timer_A,
         Timer_B,
         Timer_C,
         Timer_D,
         Timer_E,
         Timer_F,
         Timer_G,
         Timer_H
      };
};


#endif
