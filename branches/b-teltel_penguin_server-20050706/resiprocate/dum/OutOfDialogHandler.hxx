#if !defined(RESIP_OutOfDialogHandler_hxx)
#define RESIP_OutOfDialogHandler_hxx

namespace resip
{

/** @file OutOfDialogHandler.hxx
 *   @todo This file is empty
 */

class OutOfDialogHandler
{
   public:
      // Client Handlers
      virtual void onSuccess(ClientOutOfDialogReqHandle, const SipMessage& successResponse)=0;
      virtual void onFailure(ClientOutOfDialogReqHandle, const SipMessage& errorResponse)=0;

      // Server Handlers
      virtual void onReceivedRequest(ServerOutOfDialogReqHandle, const SipMessage& request)=0;
};
 
}

#endif
