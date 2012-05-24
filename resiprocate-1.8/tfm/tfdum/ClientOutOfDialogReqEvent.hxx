#if !defined ClientOutOfDialogReqEvent_hxx
#define ClientOutOfDialogReqEvent_hxx

#include "resip/stack/SipMessage.hxx"
#include "tfm/Event.hxx"
#include "tfm/tfdum/DumEvent.hxx"
#include "resip/dum/Handles.hxx"

class DumUserAgent;

typedef enum
{
   ClientOutOfDialogReq_Success,
   ClientOutOfDialogReq_Failure
} ClientOutOfDialogReqEventType;

static const char* ClientOutOfDialogReqEventTypeText[] =
{
   "Success",
   "Failure"
};

class ClientOutOfDialogReqEvent : public DumEvent
{
   public:
      typedef ClientOutOfDialogReqEventType Type;
      typedef resip::ClientOutOfDialogReqHandle HandleType;

      ClientOutOfDialogReqEvent(DumUserAgent* dua, Type type, resip::ClientOutOfDialogReqHandle h, const resip::SipMessage& msg)
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
            strm << "ClientOutOfDialogReqEvent - " << ClientOutOfDialogReqEventTypeText[mType];
         }
         return buffer;
      }

      virtual resip::Data briefString() const
      {
         return toString();
      }

      static resip::Data getName() { return "ClientOutOfDialogReqEvent"; }
      static resip::Data getTypeName(Type type) { return ClientOutOfDialogReqEventTypeText[type]; }

      Type getType() const { return mType; }
      
      resip::ClientOutOfDialogReqHandle& getHandle() { return mHandle; }

   protected:
      Type mType;
      resip::ClientOutOfDialogReqHandle mHandle;
};

#endif
