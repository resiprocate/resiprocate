#if !defined(Server_hxx)
#define Server_hxx

#include <map>

#include <resip/stack/TransactionUser.hxx>
#include <resip/stack/InterruptableStackThread.hxx>
#include <rutil/SelectInterruptor.hxx>
#include <resip/stack/UdpTransport.hxx>
#include <resip/dum/MasterProfile.hxx>
#include <resip/dum/DumShutdownHandler.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/InviteSessionHandler.hxx>
#include <resip/dum/DialogSetHandler.hxx>
#include <resip/dum/OutOfDialogHandler.hxx>
#include <resip/dum/RedirectHandler.hxx>
#include <resip/dum/SubscriptionHandler.hxx>
#include <resip/dum/RegistrationHandler.hxx>
#include <rutil/Log.hxx>
#include <rutil/SharedPtr.hxx>
#include <rutil/Mutex.hxx>

#include "ConfigParser.hxx"
#include "B2BSession.hxx"
#include "SipRegistration.hxx"
#include "AddressTranslator.hxx"
#include "MediaRelay.hxx"
#include "IChatIPPortData.hxx"
#include "IPCThread.hxx"

#ifdef WIN32
   #define sleepMs(t) Sleep(t)
#else
   #define sleepMs(t) usleep(t*1000)
#endif

#define SDP_ICHATGW_ORIGIN_USER "iChat-gw"

namespace gateway
{

class ShutdownCmd;
class B2BSession;

class Server : public ConfigParser, 
               public IPCHandler,
               public resip::DumShutdownHandler,
               public resip::InviteSessionHandler,
               public resip::DialogSetHandler,
               public resip::OutOfDialogHandler,
               public resip::RedirectHandler,
               public resip::ClientSubscriptionHandler,
               public resip::ServerSubscriptionHandler,
               public resip::ClientRegistrationHandler,
               public resip::ExternalUnknownDatagramHandler
{
public:
   Server(int argc, char** argv);
   virtual ~Server();

   /**
     Starts the SIP stack thread.
     
     @note This should be called before calling process() in a loop
   */
   void startup(); 

   /**
     This should be called in a loop to give process cycles to the server.

     @param timeoutMs Will return after timeoutMs if nothing to do.
                      Application can do some work, but should call
                      process again ASAP.
   */
   void process(int timeoutMs); // call this in a loop

   /**
     Used to initiate a shutdown of the server.  This function blocks 
     until the shutdown is complete.  

     @note There should not be an active process request when this 
           is called.
   */
   void shutdown();

   // Utility Methods ////////////////////////////////////////////////////////////
   typedef enum 
   {
      SubsystemAll,
      SubsystemContents,
      SubsystemDns,
      SubsystemDum,
      SubsystemSdp,
      SubsystemSip,
      SubsystemTransaction,
      SubsystemTransport,
      SubsystemStats,
      SubsystemGateway
   } LoggingSubsystem;

   /**
     Static method that sets the logging level for a particular subsystem, 
     or all subsystems.

     @param level Logging level to set
     @param subsystem Subsystem to set level on

     @note: Use Server::SubsystemAll to set the logging level on all 
            subsystems
   */
   static void setLogLevel(resip::Log::Level level, LoggingSubsystem subsystem=SubsystemAll);

   /**
     Called by Jabber side of gateway to notify SIP side of iChat call request.

     @param to Bare JID of endpoint we are proceeding to ring
     @param from Full JID of endpoint requesting the call
   */
   void notifyIChatCallRequest(const std::string& to, const std::string& from);

   /**
     Called by Jabber side of gateway to notify SIP side of cancelled iChat call request.

     @param handle Handle of the B2BSession object for the call
   */
   void notifyIChatCallCancelled(const B2BSessionHandle& handle);

   /**
     Called by Jabber side of gateway to notify B2BSession that iChat call is proceeding.
     This call will cause a timeout timer to be deactiviated.

     @param handle Handle of the B2BSession object for the call
     @param to Full JID of endpoint we are proceeding to ring
   */
   void notifyIChatCallProceeding(const B2BSessionHandle& handle, const std::string& to);

   /**
     Called by Jabber side of gateway to notify B2BSession that iChat call has failed.

     @param handle Handle of the B2BSession object for the call
     @param statusCode Code representing the reason for the failure
   */
   void notifyIChatCallFailed(const B2BSessionHandle& handle, unsigned int statusCode);

   /**
     Called by Jabber side of gateway to notify B2BSession that iChat call setup
     has completed on the Jabber side, and that it now needs to continue via SIP.

     @param handle Handle of the B2BSession object for the call
     @param remoteIPPortListBlob List of transports received from the iChat client via 
                             Jabber messaging - in proprietary iChat blob format
   */
   void continueIChatCall(const B2BSessionHandle& handle, const std::string& remoteIPPortListBlob);

   /**
     Called by Jabber side of gateway to request that a jabber user be registered on the
     configured SIP server.  Note:  The jabber user's JID is first transformed into a
     SIP URI using the configured translation rules.  Note:  calling this multiple times
     with the same jidToRegister is safe and will result in a no op.

     @param jidToRegister - Jabber user's JID to register on the SIP server.
   */
   void sipRegisterJabberUser(const std::string& jidToRegister);

   /**
     Called by Jabber side of gateway to request that a jabber user be unregistered from the
     configured SIP server.  Note:  The jabber user's JID is first transformed into a
     SIP URI using the configured translation rules.

     @param jidToUnregister - Jabber user's JID to unregister from the SIP server.
   */
   void sipUnregisterJabberUser(const std::string& jidToUnregister);

   /**
      Called by Jabber side of gateway to check if the JID to be subscribed will pass the
      translation rules.

      @param to - JID of address being subscribed to
      @param from - JID of subscriber
   */
   void checkSubscription(const std::string& to, const std::string& from);

   B2BSession* findMatchingIChatB2BSession(const resip::SipMessage& msg);

protected:
   resip::SharedPtr<resip::MasterProfile>& getMasterProfile() { return mProfile; }
   bool translateAddress(const resip::Data& address, resip::Data& translation, bool failIfNoRule=false);

   // IPC Handler
   virtual void onNewIPCMsg(const IPCMsg& msg);

   // External Unknown Packet Handler//////////////////////////////////////////////
   virtual void operator()(resip::UdpTransport* transport, const resip::Tuple& source, std::auto_ptr<resip::Data> unknownPacket);

   // Shutdown Handler ////////////////////////////////////////////////////////////
   void onDumCanBeDeleted();

   // Invite Session Handler /////////////////////////////////////////////////////
   virtual void onNewSession(resip::ClientInviteSessionHandle h, resip::InviteSession::OfferAnswerType oat, const resip::SipMessage& msg);
   virtual void onNewSession(resip::ServerInviteSessionHandle h, resip::InviteSession::OfferAnswerType oat, const resip::SipMessage& msg);
   virtual void onFailure(resip::ClientInviteSessionHandle h, const resip::SipMessage& msg);
   virtual void onEarlyMedia(resip::ClientInviteSessionHandle, const resip::SipMessage&, const resip::SdpContents&);
   virtual void onProvisional(resip::ClientInviteSessionHandle, const resip::SipMessage& msg);
   virtual void onConnected(resip::ClientInviteSessionHandle h, const resip::SipMessage& msg);
   virtual void onConnected(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onStaleCallTimeout(resip::ClientInviteSessionHandle);
   virtual void onTerminated(resip::InviteSessionHandle h, resip::InviteSessionHandler::TerminatedReason reason, const resip::SipMessage* msg);
   virtual void onRedirected(resip::ClientInviteSessionHandle, const resip::SipMessage& msg);
   virtual void onAnswer(resip::InviteSessionHandle, const resip::SipMessage& msg, const resip::SdpContents&);
   virtual void onOffer(resip::InviteSessionHandle handle, const resip::SipMessage& msg, const resip::SdpContents& offer);
   virtual void onOfferRequired(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onOfferRejected(resip::InviteSessionHandle, const resip::SipMessage* msg);
   virtual void onOfferRequestRejected(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onRemoteSdpChanged(resip::InviteSessionHandle, const resip::SipMessage& msg, const resip::SdpContents& sdp);
   virtual void onInfo(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onInfoSuccess(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onInfoFailure(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onRefer(resip::InviteSessionHandle, resip::ServerSubscriptionHandle, const resip::SipMessage& msg);
   virtual void onReferAccepted(resip::InviteSessionHandle, resip::ClientSubscriptionHandle, const resip::SipMessage& msg);
   virtual void onReferRejected(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onReferNoSub(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onMessage(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onMessageSuccess(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onMessageFailure(resip::InviteSessionHandle, const resip::SipMessage& msg);
   virtual void onForkDestroyed(resip::ClientInviteSessionHandle);

   // DialogSetHandler  //////////////////////////////////////////////
   virtual void onTrying(resip::AppDialogSetHandle, const resip::SipMessage& msg);
   virtual void onNonDialogCreatingProvisional(resip::AppDialogSetHandle, const resip::SipMessage& msg);

   // ClientSubscriptionHandler ///////////////////////////////////////////////////
   virtual void onUpdatePending(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify, bool outOfOrder);
   virtual void onUpdateActive(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify, bool outOfOrder);
   virtual void onUpdateExtension(resip::ClientSubscriptionHandle, const resip::SipMessage& notify, bool outOfOrder);
   virtual void onTerminated(resip::ClientSubscriptionHandle h, const resip::SipMessage* notify);
   virtual void onNewSubscription(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify);
   virtual int  onRequestRetry(resip::ClientSubscriptionHandle h, int retryMinimum, const resip::SipMessage& notify);

   // ServerSubscriptionHandler ///////////////////////////////////////////////////
   virtual void onNewSubscription(resip::ServerSubscriptionHandle, const resip::SipMessage& sub);
   virtual void onNewSubscriptionFromRefer(resip::ServerSubscriptionHandle, const resip::SipMessage& sub);
   virtual void onRefresh(resip::ServerSubscriptionHandle, const resip::SipMessage& sub);
   virtual void onTerminated(resip::ServerSubscriptionHandle);
   virtual void onReadyToSend(resip::ServerSubscriptionHandle, resip::SipMessage&);
   virtual void onNotifyRejected(resip::ServerSubscriptionHandle, const resip::SipMessage& msg);      
   virtual void onError(resip::ServerSubscriptionHandle, const resip::SipMessage& msg);      
   virtual void onExpiredByClient(resip::ServerSubscriptionHandle, const resip::SipMessage& sub, resip::SipMessage& notify);
   virtual void onExpired(resip::ServerSubscriptionHandle, resip::SipMessage& notify);
   virtual bool hasDefaultExpires() const;
   virtual UInt32 getDefaultExpires() const;

   // OutOfDialogHandler //////////////////////////////////////////////////////////
   virtual void onSuccess(resip::ClientOutOfDialogReqHandle, const resip::SipMessage& response);
   virtual void onFailure(resip::ClientOutOfDialogReqHandle, const resip::SipMessage& response);
   virtual void onReceivedRequest(resip::ServerOutOfDialogReqHandle, const resip::SipMessage& request);

   // RedirectHandler /////////////////////////////////////////////////////////////
   virtual void onRedirectReceived(resip::AppDialogSetHandle, const resip::SipMessage& response);
   virtual bool onTryingNextTarget(resip::AppDialogSetHandle, const resip::SipMessage& request);

   // Registration Handler ////////////////////////////////////////////////////////
   virtual void onSuccess(resip::ClientRegistrationHandle h, const resip::SipMessage& response);
   virtual void onFailure(resip::ClientRegistrationHandle h, const resip::SipMessage& response);
   virtual void onRemoved(resip::ClientRegistrationHandle h, const resip::SipMessage& response);
   virtual int onRequestRetry(resip::ClientRegistrationHandle h, int retryMinimum, const resip::SipMessage& msg);

private:
   friend class ShutdownCmd;
   friend class B2BSession;
   friend class IChatCallTimeout;

   resip::DialogUsageManager& getDialogUsageManager();

   void post(resip::ApplicationMessage& message, unsigned int ms=0);
   void shutdownImpl(); 

   friend class NotifyIChatCallRequestCmd;
   void notifyIChatCallRequestImpl(const std::string& to, const std::string& from);
   friend class NotifyIChatCallCancelledCmd;
   void notifyIChatCallCancelledImpl(const B2BSessionHandle& handle);
   friend class NotifyIChatCallProceedingCmd;
   void notifyIChatCallProceedingImpl(const B2BSessionHandle& handle, const std::string& to);
   friend class NotifyIChatCallFailedCmd;
   void notifyIChatCallFailedImpl(const B2BSessionHandle& handle, unsigned int statusCode);
   friend class ContinueIChatCallCmd;
   void continueIChatCallImpl(const B2BSessionHandle& handle, const std::string& remoteIPPortListBlob);
   friend class SipRegisterJabberUserCmd;
   void sipRegisterJabberUserImpl(const std::string& jidToRegister);
   friend class SipUnregisterJabberUserCmd;
   void sipUnregisterJabberUserImpl(const std::string& jidToUnregister);

   resip::SharedPtr<resip::MasterProfile> mProfile;
   resip::Security* mSecurity;
   resip::SelectInterruptor mSelectInterruptor;
   resip::SipStack mStack;
   resip::DialogUsageManager mDum;
   resip::InterruptableStackThread mStackThread;
   volatile bool mDumShutdown;

   typedef std::map<B2BSessionHandle, B2BSession*> B2BSessionMap;
   B2BSessionMap mB2BSessions;
   B2BSessionHandle mCurrentB2BSessionHandle;
   B2BSession* getB2BSession(const B2BSessionHandle& handle) const;
   B2BSessionHandle registerB2BSession(B2BSession *);
   void unregisterB2BSession(const B2BSessionHandle& handle);

   friend class SipRegistration;
   typedef std::map<resip::Uri, SipRegistration*> RegistrationMap;
   RegistrationMap mRegistrations;
   void registerRegistration(SipRegistration *);
   void unregisterRegistration(SipRegistration *);

   bool mIsV6Avail;
   IChatIPPortData mLocalIPPortData;

   MediaRelay* mMediaRelay;

   IPCThread* mIPCThread;

   AddressTranslator mAddressTranslator;
   std::map<resip::DialogSetId,B2BSession*> mActiveSessions;
};
 
}

#endif

/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */

