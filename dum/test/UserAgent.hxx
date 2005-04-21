#if !defined(DUM_UserAgent_hxx)
#define DUM_UserAgent_hxx

#include "CommandLineParser.hxx"

#include "resiprocate/StackThread.hxx"
#include "resiprocate/dum/MasterProfile.hxx"
#include "resiprocate/dum/RegistrationHandler.hxx"
#include "resiprocate/dum/SubscriptionHandler.hxx"
#include "resiprocate/dum/PublicationHandler.hxx"
#include "resiprocate/dum/OutOfDialogHandler.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"

namespace resip
{

class UserAgent : public CommandLineParser, 
                  public ClientRegistrationHandler, 
                  public ClientSubscriptionHandler, 
                  public ClientPublicationHandler,
                  public OutOfDialogHandler, 
                  public InviteSessionHandler
{
   public:
      UserAgent(int argc, char** argv);
      virtual ~UserAgent();

      void startup();
      void shutdown();

      void process();
      
   public:
      // Invite Session Handler /////////////////////////////////////////////////////
      virtual void onNewSession(ClientInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg);
      virtual void onNewSession(ServerInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg);
      virtual void onFailure(ClientInviteSessionHandle h, const SipMessage& msg);
      virtual void onEarlyMedia(ClientInviteSessionHandle, const SipMessage&, const SdpContents&);
      virtual void onProvisional(ClientInviteSessionHandle, const SipMessage& msg);
      virtual void onConnected(ClientInviteSessionHandle h, const SipMessage& msg);
      virtual void onConnected(InviteSessionHandle, const SipMessage& msg);
      virtual void onStaleCallTimeout(ClientInviteSessionHandle);
      virtual void onTerminated(InviteSessionHandle h, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg);
      virtual void onRedirected(ClientInviteSessionHandle, const SipMessage& msg);
      virtual void onAnswer(InviteSessionHandle, const SipMessage& msg, const SdpContents&);
      virtual void onOffer(InviteSessionHandle handle, const SipMessage& msg, const SdpContents& offer);
      virtual void onOfferRequired(InviteSessionHandle, const SipMessage& msg);
      virtual void onOfferRejected(InviteSessionHandle, const SipMessage& msg);
      virtual void onDialogModified(InviteSessionHandle, InviteSession::OfferAnswerType oat, const SipMessage& msg);
      virtual void onInfo(InviteSessionHandle, const SipMessage& msg);
      virtual void onInfoSuccess(InviteSessionHandle, const SipMessage& msg);
      virtual void onInfoFailure(InviteSessionHandle, const SipMessage& msg);
      virtual void onRefer(InviteSessionHandle, ServerSubscriptionHandle, const SipMessage& msg);
      virtual void onReferAccepted(InviteSessionHandle, ClientSubscriptionHandle, const SipMessage& msg);
      virtual void onReferRejected(InviteSessionHandle, const SipMessage& msg);
      
      // Registration Handler ////////////////////////////////////////////////////////
      virtual void onSuccess(ClientRegistrationHandle h, const SipMessage& response);
      virtual void onFailure(ClientRegistrationHandle h, const SipMessage& response);
      virtual void onRemoved(ClientRegistrationHandle h);
      virtual int onRequestRetry(ClientRegistrationHandle h, int retryMinimum, const SipMessage& msg);

      // ClientSubscriptionHandler ///////////////////////////////////////////////////
      virtual void onRefreshRejected(ClientSubscriptionHandle h, const SipMessage& rejection);
      virtual void onUpdatePending(ClientSubscriptionHandle h, const SipMessage& notify);
      virtual void onUpdateActive(ClientSubscriptionHandle h, const SipMessage& notify);
      virtual void onUpdateExtension(ClientSubscriptionHandle, const SipMessage& notify);
      virtual void onTerminated(ClientSubscriptionHandle h, const SipMessage& notify);
      virtual void onNewSubscription(ClientSubscriptionHandle h, const SipMessage& notify);
      virtual int onRequestRetry(ClientSubscriptionHandle h, int retryMinimum, const SipMessage& notify);

      // ClientPublicationHandler ////////////////////////////////////////////////////
      virtual void onSuccess(ClientPublicationHandle h, const SipMessage& status);
      virtual void onRemove(ClientPublicationHandle h, const SipMessage& status);
      virtual void onFailure(ClientPublicationHandle h, const SipMessage& response);
      virtual int onRequestRetry(ClientPublicationHandle h, int retryMinimum, const SipMessage& response);
      
      // OutOfDialogHandler //////////////////////////////////////////////////////////
      virtual void onSuccess(ClientOutOfDialogReqHandle, const SipMessage& response);
      virtual void onFailure(ClientOutOfDialogReqHandle, const SipMessage& response);
      virtual void onReceivedRequest(ServerOutOfDialogReqHandle, const SipMessage& request);
      virtual void onForkDestroyed(ClientInviteSessionHandle);

   protected:
      void addTransport(TransportType type, int port);

   private:
      MasterProfile mProfile;
      Security* mSecurity;
      SipStack mStack;
      DialogUsageManager mDum;
      StackThread mStackThread;
};
 
}

#endif
