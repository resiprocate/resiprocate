#if !defined InviteSessionEvent_hxx
#define InviteSessionEvent_hxx

#include "resip/stack/SipMessage.hxx"
#include "tfm/Event.hxx"
#include "tfm/tfdum/DumEvent.hxx"

class DumUserAgent;

typedef enum
{
   Invite_NewClientSession,
   Invite_NewServerSession,
   Invite_Failure,
   Invite_EarlyMedia,
   Invite_Provisional,
   Invite_Connected,
   Invite_StaleCallTimeout,
   Invite_Terminated,
   Invite_ForkDestroyed,
   Invite_Redirected,
   Invite_ReadyToSend,
   Invite_Answer,
   Invite_Offer,
   Invite_RemoteSdpChanged,
   Invite_OfferRequestRejected,
   Invite_OfferRequired,
   Invite_OfferRejected,
   //DialogModified,
   Invite_Info,
   Invite_InfoSuccess,
   Invite_InfoFailure,
   Invite_Message,
   Invite_MessageSuccess,
   Invite_MessageFailure,
   Invite_Refer,
   Invite_ReferAccepted,
   Invite_ReferRejected,
   Invite_AckNotReceived,
   Invite_IllegalNegotiation,
   Invite_SessionExpired,
   Invite_ReferNoSub,
   Invite_Prack
} InviteEventType;

static const char* InviteEventTypeText[] = 
{
   "New Client Session",
   "New Server Session",
   "Failure",
   "Early Media",
   "Provisional",
   "Connected",
   "Stale Call Timeout",
   "Terminated",
   "Fork Destroyed",
   "Redirected",
   "Ready-To-Send",
   "Answer",
   "Offer",
   "Remote Sdp Changed",
   "Offer Request Rejected",
   "Offer Required",
   "Offer Rejected",
   //DialogModified,
   "Info",
   "Info Success",
   "Info Failure",
   "Message",
   "Message Success",
   "Message Failure",
   "Refer",
   "Refer Accepted",
   "Refer Rejected",
   "Ack Not Received",
   "Illegal Negotiation",
   "Session Expired",
   "Refer No Sub",
   "Prack"
};

class InviteEvent : public DumEvent
{
   public:
      typedef InviteEventType Type;
      typedef resip::InviteSessionHandle HandleType;
      
      InviteEvent(DumUserAgent* dua, Type type, resip::InviteSessionHandle h) 
         : DumEvent(dua),
           mType(type),
           mHandle(h)
      {
      }
      InviteEvent(DumUserAgent* dua, Type type, resip::InviteSessionHandle h, const resip::SipMessage& msg)
         : DumEvent(dua, msg),
           mType(type),
           mHandle(h)
      {
      }
      InviteEvent(DumUserAgent* dua, Type type, resip::InviteSessionHandle h, const resip::SipMessage& msg, resip::ServerSubscriptionHandle sh)
         : DumEvent(dua, msg),
           mType(type),
           mHandle(h),
           mServerSubscription(sh)
      {
      }
      InviteEvent(DumUserAgent* dua, Type type, resip::InviteSessionHandle h, const resip::SipMessage& msg, resip::ClientSubscriptionHandle sh)
         : DumEvent(dua, msg),
           mType(type),
           mHandle(h),
           mClientSubscription(sh)
      {
      }

      virtual resip::Data toString() const
      {
         resip::Data buffer;
         {
            resip::DataStream strm(buffer);
            strm << "InviteEvent - " << InviteEventTypeText[mType];
         }
         return buffer;
      }

      virtual resip::Data briefString() const
      {
         return toString();
      }

      static resip::Data getName() { return "InviteEvent"; }
      static resip::Data getTypeName(Type type) { return InviteEventTypeText[type]; }

      Type getType() const { return mType; }
      
      resip::InviteSessionHandle& getHandle() { return mHandle; }
      

   protected:
      Type mType;
      resip::InviteSessionHandle mHandle;
      resip::ServerSubscriptionHandle mServerSubscription;
      resip::ClientSubscriptionHandle mClientSubscription;
};

class ClientInviteEvent : public InviteEvent
{
   public:
      typedef resip::ClientInviteSessionHandle HandleType;

      ClientInviteEvent(DumUserAgent* dua, Type type, resip::ClientInviteSessionHandle h) 
         : InviteEvent(dua, type, h->getSessionHandle()),
           mClientHandle(h)
      {
      }

      ClientInviteEvent(DumUserAgent* dua, Type type, resip::ClientInviteSessionHandle h, const resip::SipMessage& msg)
         : InviteEvent(dua, type, h->getSessionHandle(), msg),
           mClientHandle(h)
      {
      }

      resip::ClientInviteSessionHandle& getHandle() { return mClientHandle; }
      
   protected:
      resip::ClientInviteSessionHandle mClientHandle;
};

class ServerInviteEvent : public InviteEvent
{
   public:
      typedef resip::ServerInviteSessionHandle HandleType;

      ServerInviteEvent(DumUserAgent* dua, Type type, resip::ServerInviteSessionHandle h)
         : InviteEvent(dua, type, h->getSessionHandle()),
           mServerHandle(h)
      {
      }

      ServerInviteEvent(DumUserAgent* dua, Type type, resip::ServerInviteSessionHandle h, const resip::SipMessage& msg)
         : InviteEvent(dua, type, h->getSessionHandle(), msg),
           mServerHandle(h)
      {
      }

      resip::ServerInviteSessionHandle& getHandle() { return mServerHandle; }


   protected:
      resip::ServerInviteSessionHandle mServerHandle;
};


#endif
