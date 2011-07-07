#if !defined ClientSubscriptionEvent_hxx
#define ClientSubscriptionEvent_hxx

#include "resip/stack/SipMessage.hxx"
#include "tfm/Event.hxx"
#include "tfm/tfdum/DumEvent.hxx"

#include "resip/dum/Handles.hxx"

class DumUserAgent;

typedef enum
{
   ClientSubscription_RefreshRejected,
   ClientSubscription_UpdatePending,
   ClientSubscription_UpdateActive, 
   ClientSubscription_UpdateExtension,
   //ClientSubscription_RequestRetry,
   ClientSubscription_Terminated,
   ClientSubscription_NewSubscription,
   ClientSubscription_ReadyToSend
} ClientSubscriptionEventType;

static const char* ClientSubscriptionEventTypeText[] =
{
   "Refresh Rejected",
   "Update Pending",
   "Update Active",
   "Update Extension",
   //ClientSubscription_RequestRetry,
   "Terminated",
   "New Subscription",
   "Ready-To-Send"
};

class ClientSubscriptionEvent : public DumEvent
{
   public:
      typedef ClientSubscriptionEventType Type;
      typedef resip::ClientSubscriptionHandle HandleType;

      ClientSubscriptionEvent(DumUserAgent* dua, Type type, resip::ClientSubscriptionHandle h, const resip::SipMessage* msg) 
         : DumEvent(dua, msg),
           mType(type),
           mHandle(h)
      {
      }

      ClientSubscriptionEvent(DumUserAgent* dua, Type type, resip::ClientSubscriptionHandle h, const resip::SipMessage* msg, bool outOfOrder)
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
            strm << "ClientSubscriptionEvent - " << ClientSubscriptionEventTypeText[mType];
         }
         return buffer;
      }

      virtual resip::Data briefString() const
      {
         return toString();
      }

      static resip::Data getName() { return "ClientSubscriptionEvent"; }
      static resip::Data getTypeName(Type type) { return ClientSubscriptionEventTypeText[type]; }

      Type getType() const { return mType; }
      
      resip::ClientSubscriptionHandle& getHandle() { return mHandle; }
      

   protected:
      Type mType;
      resip::ClientSubscriptionHandle mHandle;
};

#endif
