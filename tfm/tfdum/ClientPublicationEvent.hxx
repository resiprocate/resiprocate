#if !defined ClientPublicationEvent_hxx
#define ClientPublicationEvent_hxx

#include "resip/stack/SipMessage.hxx"
#include "tfm/Event.hxx"
#include "tfm/tfdum/DumEvent.hxx"

class DumUserAgent;

typedef enum
{
   ClientPublication_Success,
   ClientPublication_Remove,
   ClientPublication_Failure,
   //ClientPublication_RequestRetry,
   ClientPublication_StaleUpdate
} ClientPublicationEventType;

static const char* ClientPublicationEventTypeText[] =
{
   "Success",
   "Remove",
   "Failure",
   //ClientPublication_RequestRetry,
   "Stale Update"
};

class ClientPublicationEvent : public DumEvent
{
   public:
      typedef ClientPublicationEventType Type;
      typedef resip::ClientPublicationHandle HandleType;

      ClientPublicationEvent(DumUserAgent* dua, Type type, resip::ClientPublicationHandle h, const resip::SipMessage& msg) 
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
            strm << "ClientPublicationEvent - " << ClientPublicationEventTypeText[mType];
         }
         return buffer;
      }

      virtual resip::Data briefString() const
      {
         return toString();
      }

      static resip::Data getName() { return "ClientPublicationEvent"; }
      static resip::Data getTypeName(Type type) { return ClientPublicationEventTypeText[type]; }

      Type getType() const { return mType; }
      
      resip::ClientPublicationHandle& getHandle() { return mHandle; }

   protected:
      Type mType;
      resip::ClientPublicationHandle mHandle;
};

#endif
