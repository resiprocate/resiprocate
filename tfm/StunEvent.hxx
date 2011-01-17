#if !defined StunEvent_hxx
#define StunEvent_hxx

#include "tfm/Event.hxx"
#include "rutil/Logger.hxx"

#include "tfm/StunServer.hxx"

#include <boost/shared_ptr.hpp>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

class StunEndPoint;

typedef enum
{
   Stun_BindingRequest,
   Turn_AllocateRequest,
   Turn_SendRequest,
   Turn_SetActiveDestinationRequest,
   
} StunEventType;

static const char* StunEventTypeText[] = 
{
   "STUN Binding Request",
   "TURN Allocate Request",
   "TURN Send Request",
   "TURN Active Destination Request",
};

class StunEvent : public Event
{
   public:
      typedef StunEventType Type;

      StunEvent(StunEndPoint* tua, Type event);

      virtual resip::Data toString() const
      {
         resip::Data buffer;
         {
            resip::DataStream strm(buffer);
            strm << "StunEvent - " << StunEventTypeText[mType];
         }
         return buffer;
      }

      virtual resip::Data briefString() const
      {
         return toString();
      }

      static resip::Data getName(){ return "StunEvent"; }

      Type getType() const { return mType; }

      static resip::Data getTypeName(Type type) { return StunEventTypeText[type]; }

   private:
      Type mType;
};
#endif
