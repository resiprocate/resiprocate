#if !defined ClientPagerMessageEvent_hxx
#define ClientPagerMessageEvent_hxx

#include "resip/stack/SipMessage.hxx"
#include "tfm/Event.hxx"
#include "tfm/tfdum/DumEvent.hxx"

class DumUserAgent;

typedef enum
{
   ClientPagerMessage_Success,
   ClientPagerMessage_Failure
} ClientPagerMessageEventType;

static const char* ClientPagerMessageEventTypeText[] = 
{
   "Success",
   "Failure"
};

class ClientPagerMessageEvent : public DumEvent
{
   public:
      typedef ClientPagerMessageEventType Type;
      typedef resip::ClientPagerMessageHandle HandleType;

      ClientPagerMessageEvent(DumUserAgent* dua, Type type, resip::ClientPagerMessageHandle h, const resip::SipMessage& msg) 
         : DumEvent(dua, msg),
           mType(type),
           mHandle(h)
      {
      }

      ClientPagerMessageEvent(DumUserAgent* dua, Type type, resip::ClientPagerMessageHandle h, const resip::SipMessage& msg, 
                              std::auto_ptr<resip::Contents> constents)
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
            strm << "ClientPagerMessageEvent - " << ClientPagerMessageEventTypeText[mType];
         }
         return buffer;
      }

      virtual resip::Data briefString() const
      {
         return toString();
      }

      static resip::Data getName() { return "ClientPagerMessgeEvent"; }
      static resip::Data getTypeName(Type type) { return ClientPagerMessageEventTypeText[type]; }

      Type getType() const { return mType; }
      
      resip::ClientPagerMessageHandle& getHandle() { return mHandle; }
      

   protected:
      Type mType;
      resip::ClientPagerMessageHandle mHandle;

};

#endif
