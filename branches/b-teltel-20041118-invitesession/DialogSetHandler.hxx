#if !defined(RESIP_DIALOGSETHANDLER_HXX)
#define RESIP_DIALOGSETHANDLER_HXX

#include "resiprocate/dum/DialogSet.hxx"
#include "resiprocate/dum/Handles.hxx"

namespace resip
{

class SdpContents;
class SipMessage;


class DialogSetHandler
{
   public:
      //called when a 100 is received 
      virtual void onTrying(AppDialogSetHandle, const SipMessage& msg)=0;
      
      //called when a provisional response that does not create a dialog arrives
      //usually, this is a 180 w/out a contact
      virtual void onNonDialogCreatingProvisional(AppDialogSetHandle, const SipMessage& msg)=0;
};


}

#endif

   
