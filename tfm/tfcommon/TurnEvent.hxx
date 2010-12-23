#if !defined TurnEvent_hxx
#define TurnEvent_hxx

#include "tfm/Event.hxx"
#include "rutil/Logger.hxx"

#include <boost/shared_ptr.hpp>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

class TurnEndPoint;

typedef enum
{
   Turn_AllocateResponse,
   Turn_AllocateErrorResponse,
   Turn_SendResponse,
   Turn_SendErrorResponse,
   Turn_DataIndication,
   Turn_SetActiveDestinationResponse,
   Turn_SetActiveDestinationErrorResponse,
   Turn_AlternateServer,
   Turn_BindingFailure
   
} TurnEventType;

static const char* TurnEventTypeText[] = 
{
   "TURN Allocate Response",
   "TURN Allocate Error Response",
   "TURN Send Response",
   "TURN Send Error Response",
   "TURN Data Indication",
   "TURN Set Active Destination Response",
   "TURN Set Active Destination Error Response",
   "TURN Alternate Server",
   "TURN Binding Failure"
};

class TurnEvent : public Event
{
   public:
      typedef TurnEventType Type;

      TurnEvent(TurnEndPoint* tua, Type event);

      virtual resip::Data toString() const
      {
         resip::Data buffer;
         {
            resip::DataStream strm(buffer);
            strm << "TurnEvent - " << TurnEventTypeText[mType];
         }
         return buffer;
      }

      virtual resip::Data briefString() const
      {
         return toString();
      }

      static resip::Data getName(){ return "TurnEvent"; }

      Type getType() const { return mType; }

      static resip::Data getTypeName(Type type) { return TurnEventTypeText[type]; }

   private:
      Type mType;
};
#endif
