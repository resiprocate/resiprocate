#ifndef AppTransactionState_Include_Guard
#define AppTransactionState_Include_Guard

#include "rutil/Data.hxx"

namespace resip
{
/**
   Class that an application can use to store transaction-specific state that is
   not sent on the wire at any point. See SipMessage::appData().
*/
class AppTransactionState
{
   public:
      AppTransactionState(){}
      virtual ~AppTransactionState(){}
      virtual AppTransactionState* clone() const=0;

      /**
         @return Opaque identifier of the type of app state contained within. 
         For example, if you wanted to store the time at which you sent the 
         request to the stack, you might return something like "myapp_timestamp" 
         here. Alternately, you could create a unique subclass and use RTTI.
      */
      virtual const resip::Data& identifier() const
      {
         return resip::Data::Empty;
      }
};
}

#endif
