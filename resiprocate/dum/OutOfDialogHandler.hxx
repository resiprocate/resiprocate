


/** @file OutOfDialogHandler.hxx
 *   @todo This file is empty
 */

class OutOfDialogHandler
{
   public:
      // Client Handlers
      virtual void onSuccess(ClientOutOfDialogReq::Handle, const SipMessage& successResponse)=0;
      virtual void onFailure(ClientOutOfDialogReq::Handle, const SipMessage& errorResponse)=0;

      // Server Handlers
      virtual void onReceivedRequest(ServerOutOfDialogReq::Handle, const SipMessage& request)=0;
};
