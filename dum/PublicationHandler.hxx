#if !defined(RESIP_REGISTRATIONHANDLER_HXX)
#define RESIP_REGISTRATIONHANDLER_HXX

namespace resip
{

class PublicationHandler
{
   public:
      virtual void onSuccess(ClientPublication::Handle, const SipMessage& status)=0;
      virtual void onStaleUpdate(ClientPublication::Handle, const SipMessage& status)=0;
      virtual void onFailure(ClientPublication::Handle, const SipMessage& status)=0;
      
      virtual void onInitial(ServerPublication::Handle, const SipMessage& pub)=0;
      virtual void onExpired(ServerPublication::Handle)=0;
      virtual void onRefresh(ServerPublication::Handle, int expires)=0;
      virtual void onUpdate(ServerPublication::Handle, const SipMessage& pub)=0;
      virtual void onRemoved(ServerPublication::Handle);
};
 
}

#endif
