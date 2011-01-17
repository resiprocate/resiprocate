#if !defined ServerSubscriptionEvent_hxx
#define ServerSubscriptionEvent_hxx

#include "resip/stack/SipMessage.hxx"
#include "tfm/Event.hxx"
#include "tfm/tfdum/DumEvent.hxx"

class DumUserAgent;

typedef enum
{
   ServerSubscription_Refresh,
   ServerSubscription_Published,
   ServerSubscription_Error,
   ServerSubscription_ExpiredByClient,
   ServerSubscription_Expired,
   ServerSubscription_Terminated,
   ServerSubscription_NewSubscription,
   ServerSubscription_NewSubscriptionFromRefer,
   ServerSubscription_ReadyToSend
} ServerSubscriptionEventType;

static const char* ServerSubscriptionEventTypeText[] =
{
   "Refresh",
   "Published",
   "Error",
   "Expired By Client",
   "Expired",
   "Terminated",
   "New Subscription",
   "New Subscription From Refer",
   "Ready-To-Send"
};

class ServerSubscriptionEvent : public DumEvent
{
   public:
      typedef ServerSubscriptionEventType Type;
      typedef resip::ServerSubscriptionHandle HandleType;

      ServerSubscriptionEvent(DumUserAgent* dua, Type type, resip::ServerSubscriptionHandle h) 
         : DumEvent(dua),
           mType(type),
           mHandle(h)
      {
      }

      ServerSubscriptionEvent(DumUserAgent* dua, Type type, resip::ServerSubscriptionHandle h, const resip::SipMessage& msg)
         : DumEvent(dua, msg),
           mType(type),
           mHandle(h)
      {
      }

      ServerSubscriptionEvent(DumUserAgent* dua, Type type, resip::ServerSubscriptionHandle h, resip::ServerPublicationHandle pub,
                              const resip::Contents* contents, const resip::SecurityAttributes* attrs)
         :DumEvent(dua),
          mType(type),
          mHandle(h),
          mServerPublication(pub)
      {
      }

      virtual resip::Data toString() const
      {
         resip::Data buffer;
         {
            resip::DataStream strm(buffer);
            strm << "ServerSubscriptionEvent - " << ServerSubscriptionEventTypeText[mType];
         }
         return buffer;
      }

      virtual resip::Data briefString() const
      {
         return toString();
      }

      static resip::Data getName() { return "ServerSubscriptionEvent"; }
      static resip::Data getTypeName(Type type) { return ServerSubscriptionEventTypeText[type]; }

      Type getType() const { return mType; }
      
      resip::ServerSubscriptionHandle& getHandle() { return mHandle; }
      

   protected:
      Type mType;
      resip::ServerSubscriptionHandle mHandle;
      resip::ServerPublicationHandle mServerPublication;
};

#endif
