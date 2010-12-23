#if !defined DumEvent_hxx
#define DumEvent_hxx

#include "resip/stack/SipMessage.hxx"
#include "tfm/Event.hxx"
#include "resip/dum/InviteSession.hxx"
#include "resip/dum/ClientInviteSession.hxx"
#include "resip/dum/ServerInviteSession.hxx"
#include "resip/dum/Handles.hxx"
#include <boost/shared_ptr.hpp>

class DumUserAgent;

class DumEvent : public Event
{
   public:
      DumEvent(DumUserAgent* dua);
      DumEvent(DumUserAgent* dua, const resip::SipMessage* msg);
      DumEvent(DumUserAgent* dua, const resip::SipMessage& msg);
      
      bool hasMessage() const { return mMessage.get() != 0; }
      boost::shared_ptr<resip::SipMessage> const getMessage() { return mMessage; }
   private:
      boost::shared_ptr<resip::SipMessage> mMessage;
};

/*
// ClientPagerMessage
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

      ClientPagerMessageEvent(DumUserAgent* dua, Type type, resip::ClientPagerMessageHandle h, const resip::SipMessage& msg) 
         : DumEvent(dua, msg),
           mType(type),
           mHandle(h)
           //mMessage(msg)
      {
      }

      ClientPagerMessageEvent(DumUserAgent* dua, Type type, resip::ClientPagerMessageHandle h, const resip::SipMessage& msg, 
                              std::auto_ptr<resip::Contents> constents)
         : DumEvent(dua, msg),
           mType(type),
           mHandle(h)
           //mMessage(msg)
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
      
      resip::ClientPagerMessageHandle& getUsageHandle() { return mHandle; }
      

   protected:
      Type mType;
      resip::ClientPagerMessageHandle mHandle;
      //const resip::SipMessage mMessage;

};

// ServerPagerMessage
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

      ServerPagerMessageEvent(DumUserAgent* dua, Type type, resip::ServerPagerMessageHandle h, const resip::SipMessage& msg) 
         : DumEvent(dua, msg),
           mType(type),
           mHandle(h)
           //mMessage(msg)
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
      
      resip::ServerPagerMessageHandle& getUsageHandle() { return mHandle; }
      

   protected:
      Type mType;
      resip::ServerPagerMessageHandle mHandle;
      //const resip::SipMessage mMessage;

};

// ServerOutOfDialogReq
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
      
      resip::ServerOutOfDialogReqHandle& getUsageHandle() { return mHandle; }

   protected:
      Type mType;
      resip::ServerOutOfDialogReqHandle mHandle;
      //const resip::SipMessage mMessage;
};

// ClientOutOfDialogReq
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

      ClientOutOfDialogReqEvent(DumUserAgent* dua, Type type, resip::ClientOutOfDialogReqHandle h, const resip::SipMessage& msg)
         : DumEvent(dua, msg),
           mType(type),
           mHandle(h)
           //mMessage(msg)
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
      
      resip::ClientOutOfDialogReqHandle& getUsageHandle() { return mHandle; }

   protected:
      Type mType;
      resip::ClientOutOfDialogReqHandle mHandle;
      //const resip::SipMessage mMessage;
};
*/

#endif
