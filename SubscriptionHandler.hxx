#if !defined(RESIP_SUBSCRIPTIONHANDLER_HXX)
#define RESIP_SUBSCRIPTIONHANDLER_HXX

#include "resiprocate/dum/Handles.hxx"
#include "resiprocate/Mime.hxx"

namespace resip
{
class SipMessage;
class SecurityAttributes;

class ClientSubscriptionHandler
{
  public:
      virtual void onRefreshRejected(ClientSubscriptionHandle, const SipMessage& rejection)=0;

      //Client must call acceptUpdate or rejectUpdate for any onUpdateFoo
      virtual void onUpdatePending(ClientSubscriptionHandle, const SipMessage& notify)=0;
      virtual void onUpdateActive(ClientSubscriptionHandle, const SipMessage& notify)=0;
      //unknown Subscription-State value
      virtual void onUpdateExtension(ClientSubscriptionHandle, const SipMessage& notify)=0;      

      //subscription can be ended through a notify or a failure response.
      virtual void onTerminated(ClientSubscriptionHandle, const SipMessage& msg)=0;   
      //not sure if this has any value.
      virtual void onNewSubscription(ClientSubscriptionHandle, const SipMessage& notify)=0;
};

class ServerSubscriptionHandler
{
  public:   
      virtual void onNewSubscription(ServerSubscriptionHandle, const SipMessage& sub)=0;
      virtual void onRefresh(ServerSubscriptionHandle, const SipMessage& sub);
      virtual void onPublished(ServerSubscriptionHandle associated, 
                               ServerPublicationHandle publication, 
                               const Contents* contents,
                               const SecurityAttributes* attrs);

      virtual void onNotifyRejected(ServerSubscriptionHandle, const SipMessage& msg);      

      //called when this usage is destroyed for any reason. One of the following
      //three methods will always be called before this, but this is the only
      //method that MUST be implemented by a handler
      virtual void onTerminated(ServerSubscriptionHandle)=0;

      //will be called when a NOTIFY is not delivered(with a usage terminating
      //statusCode), or the Dialog is destroyed
      virtual void onError(ServerSubscriptionHandle, const SipMessage& msg);      

      //app can synchronously decorate terminating NOTIFY messages. The only
      //graceful termination mechanism is expiration, but the client can
      //explicity end a subscription with an Expires header of 0.
      virtual void onExpiredByClient(ServerSubscriptionHandle, const SipMessage& sub, SipMessage& notify);
      virtual void onExpired(ServerSubscriptionHandle, SipMessage& notify);

      virtual bool hasDefaultExpires() const;
      virtual int getDefaultExpires() const;

      const Mimes& getSupportedMimeTypes() const;
};
 
}

#endif
