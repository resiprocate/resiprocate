#if !defined ServerPagerMessageEvent_hxx
#define ServerPagerMessageEvent_hxx

#include "resip/stack/SipMessage.hxx"
#include "tfm/Event.hxx"
#include "tfm/tfdum/DumEvent.hxx"

class DumUserAgent;

typedef enum
{
   ServerPagerMessage_MessageArrived
} ServerPagerMessageEventType;

static const char* ServerPagerMessageEventTypeText[] = 
{
   "Message Arrived"
};

class ServerPagerMessageEvent : public DumEvent
{
   public:
      typedef ServerPagerMessageEventType Type;
      typedef resip::ServerPagerMessageHandle HandleType;

      ServerPagerMessageEvent(DumUserAgent* dua, Type type, resip::ServerPagerMessageHandle h, const resip::SipMessage& msg) 
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
            strm << "ServerPagerMessageEvent - " << ServerPagerMessageEventTypeText[mType];
         }
         return buffer;
      }

      virtual resip::Data briefString() const
      {
         return toString();
      }

      static resip::Data getName() { return "ServerPagerMessgeEvent"; }
      static resip::Data getTypeName(Type type) { return ServerPagerMessageEventTypeText[type]; }
      Type getType() const { return mType; }
      
      resip::ServerPagerMessageHandle& getHandle() { return mHandle; }
      

   protected:
      Type mType;
      resip::ServerPagerMessageHandle mHandle;
};

#endif
