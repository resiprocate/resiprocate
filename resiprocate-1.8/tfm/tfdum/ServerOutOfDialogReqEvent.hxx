#if !defined ServerOutOfDialogReqEvent_hxx
#define ServerOutOfDialogReqEvent_hxx

#include "resip/stack/SipMessage.hxx"
#include "tfm/Event.hxx"
#include "tfm/tfdum/DumEvent.hxx"
#include "resip/dum/Handles.hxx"

class DumUserAgent;

typedef enum
{
   ServerOutOfDialogReq_ReceivedRequest
} ServerOutOfDialogReqEventType;

static const char* ServerOutOfDialogReqEventTypeText[] =
{
   "Received Request"
};

class ServerOutOfDialogReqEvent : public DumEvent
{
   public:
      typedef ServerOutOfDialogReqEventType Type;
      typedef resip::ServerOutOfDialogReqHandle HandleType;

      ServerOutOfDialogReqEvent(DumUserAgent* dua, Type type, resip::ServerOutOfDialogReqHandle h, const resip::SipMessage& msg)
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
            strm << "ServerOutOfDialogReqEvent - " << ServerOutOfDialogReqEventTypeText[mType];
         }
         return buffer;
      }

      virtual resip::Data briefString() const
      {
         return toString();
      }

      static resip::Data getName() { return "ServerOutOfDialogReqEvent"; }
      static resip::Data getTypeName(Type type) { return ServerOutOfDialogReqEventTypeText[type]; }

      Type getType() const { return mType; }
      
      resip::ServerOutOfDialogReqHandle& getHandle() { return mHandle; }

   protected:
      Type mType;
      resip::ServerOutOfDialogReqHandle mHandle;
};

#endif
