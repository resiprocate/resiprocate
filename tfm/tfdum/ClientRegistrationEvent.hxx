#if !defined ClientRegistrationEvent_hxx
#define ClientRegistrationEvent_hxx

#include "resip/stack/SipMessage.hxx"
#include "tfm/Event.hxx"
#include "tfm/tfdum/DumEvent.hxx"
#include "resip/dum/Handles.hxx"

class DumUserAgent;

typedef enum
{
   Register_Success,
   Register_Failure,
   Register_Removed

} RegisterEventType;
  
static const char* RegisterEventTypeText[] =
{
   "Success",
   "Failure",
   "Removed"
};

class ClientRegistrationEvent : public DumEvent
{
   public:
      typedef RegisterEventType Type;
      typedef resip::ClientRegistrationHandle HandleType;
      
      ClientRegistrationEvent(DumUserAgent* dua, 
                              Type type,
                              resip::ClientRegistrationHandle h)
         : DumEvent(dua),
           mType(type),
           mHandle(h)
      {
      }

      ClientRegistrationEvent(DumUserAgent* dua, Type type, resip::ClientRegistrationHandle h, 
                              const resip::SipMessage& msg)
         : DumEvent(dua, msg),
           mType(type),
           mHandle(h)
      {
      }

      virtual resip::Data toString() const
      {
         resip::Data buffer;
         {
            resip::DataStream strm(buffer);
            strm << "ClientRegistrationEvent - " << RegisterEventTypeText[mType];
         }
         return buffer;
      }

      virtual resip::Data briefString() const
      {
         return toString();
      }

      static resip::Data getName() { return "ClientRegistrationEvent"; }
      static resip::Data getTypeName(Type type) { return RegisterEventTypeText[type]; }

      Type getType() const { return mType; }

      resip::ClientRegistrationHandle& getHandle() { return mHandle; }

   protected:
      Type mType;      
      resip::ClientRegistrationHandle mHandle;
};


#endif
