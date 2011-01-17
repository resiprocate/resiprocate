#if !defined DialogEventHandlerEvent_hxx
#define DialogEventHandlerEvent_hxx

#include "tfm/Event.hxx"
#include "tfm/tfdum/DumEvent.hxx"
#include "resip/dum/DialogEventInfo.hxx"

typedef enum
{
   DialogEvent_Trying,
   DialogEvent_Proceeding,
   DialogEvent_Early,
   DialogEvent_Confirmed,
   DialogEvent_Terminated
} DialogEventHandlerEventType;

static const char* DialogEventTypeText[] = 
{
   "Trying",
   "Proceeding",
   "Early",
   "Confirmed",
   "Terminated"
};

class DialogEventHandlerEvent : public DumEvent
{
   public:
      typedef DialogEventHandlerEventType Type;
      
      DialogEventHandlerEvent(DumUserAgent* dua, Type type, const resip::DialogEventInfo& dialogEventInfo) 
         : DumEvent(dua),
           mType(type),
           mDialogEventInfo(dialogEventInfo),
           mTerminatedReason(resip::InviteSessionHandler::Error),
           mCode(0)
      {
      }

      DialogEventHandlerEvent(DumUserAgent* dua, Type type, const resip::DialogEventInfo& dialogEventInfo,
                              resip::InviteSessionHandler::TerminatedReason reason, int code) 
         : DumEvent(dua),
           mType(type),
           mDialogEventInfo(dialogEventInfo),
           mTerminatedReason(reason),
           mCode(code)
      {
      }

      virtual resip::Data toString() const
      {
         resip::Data buffer;
         {
            resip::DataStream strm(buffer);
            strm << "DialogEventHandlerEvent - " << DialogEventTypeText[mType];
         }
         return buffer;
      }

      virtual resip::Data briefString() const
      {
         return toString();
      }

      static resip::Data getName() { return "DialogEventHandlerEvent"; }
      static resip::Data getTypeName(Type type) { return DialogEventTypeText[type]; }

      Type getType() const { return mType; }
      
      resip::DialogEventInfo& getDialogEventInfo() { return mDialogEventInfo; }
      resip::InviteSessionHandler::TerminatedReason getTerminatedReason() { return mTerminatedReason; }
      int getCode() { return mCode; }

   protected:
      Type mType;
      resip::DialogEventInfo mDialogEventInfo;
      resip::InviteSessionHandler::TerminatedReason mTerminatedReason;
      int mCode;
};

#endif