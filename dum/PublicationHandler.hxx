#if !defined(RESIP_REGISTRATIONHANDLER_HXX)
#define RESIP_REGISTRATIONHANDLER_HXX

#include "resiprocate/dum/Handles.hxx"

namespace resip
{
class ClientPublication;
class ServerPublication;
class SipMessage;

class PublicationHandler
{
   public:
      virtual void onSuccess(ClientPublicationHandle, const SipMessage& status)=0;
      virtual void onStaleUpdate(ClientPublicationHandle, const SipMessage& status)=0;
      virtual void onFailure(ClientPublicationHandle, const SipMessage& status)=0;
      
      virtual void onInitial(ServerPublicationHandle, const SipMessage& pub)=0;
      virtual void onExpired(ServerPublicationHandle)=0;
      virtual void onRefresh(ServerPublicationHandle, int expires)=0;
      virtual void onUpdate(ServerPublicationHandle, const SipMessage& pub)=0;
      virtual void onRemoved(ServerPublicationHandle);
};
 
}

#endif
