#if !defined(RESIP_PAGERMESSAGEHANDLER_HXX)
#define RESIP_PAGERMESSAGEHANDLER_HXX

#include "resiprocate/dum/Handles.hxx"

namespace resip
{
class ClientMessage;
class ServerMessage;
class SipMessage;

class ClientPagerMessageHandler     
{
   public:
      virtual void onSuccess(ClientPagerMessageHandle, const SipMessage& status)=0;
      //call on failure. The usage will be destroyed.  Note that this may not
      //necessarily be 4xx...a malformed 200, etc. could also reach here.      
      virtual void onFailure(ClientPagerMessageHandle, const SipMessage& status)=0;
};

class ServerPagerMessageHandler
{
   public:
      virtual void onMessageArrived(ServerPagerMessageHandle, const SipMessage& message)=0;
};
 
}

#endif
