#if !defined(RESIP_REDIRECTHANDLER_HXX)
#define RESIP_REDIRECTHANDLER_HXX


namespace resip
{

class SipMessage;
class NameAddr;

class RedirectHandler
{
   public:
//3xx that isn't 380 or 305 has been received
      virtual void onRedirectReceived(AppDialogSetHandle, const SipMessage& response)=0;
      
      //return true if this target is acceptable
      virtual bool onTryingNextTarget(AppDialogSetHandle, const SipMessage& request)=0;
};
   

}

#endif
