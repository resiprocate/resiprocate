#if !defined(JabberComponent_hxx)
#define JabberComponent_hxx

#include "JabberUserDb.hxx"
#include "../IPCThread.hxx"

#include <string>
#include <map>
#include <set>
#include <list>

#ifdef WIN32
#define RESIP_CONTRIB_GLOOX
#endif

// Gloox includes
#ifndef RESIP_CONTRIB_GLOOX
#include <gloox/component.h>
#include <gloox/mutex.h>
#include <gloox/messagehandler.h>
#include <gloox/presencehandler.h>
#include <gloox/loghandler.h>
#include <gloox/rostermanager.h>
#include <gloox/connectionlistener.h>
#include <gloox/stanzaextension.h>
#include <gloox/iqhandler.h>
#include <gloox/disco.h>
#include <gloox/disconodehandler.h>
#include <gloox/subscriptionhandler.h>
#else
#include <src/component.h>
#include <src/mutex.h>
#include <src/messagehandler.h>
#include <src/presencehandler.h>
#include <src/loghandler.h>
#include <src/rostermanager.h>
#include <src/connectionlistener.h>
#include <src/stanzaextension.h>
#include <src/iqhandler.h>
#include <src/disco.h>
#include <src/disconodehandler.h>
#include <src/subscriptionhandler.h>
#endif

namespace gateway
{

class JabberComponent;
class IChatUser;

class IChatCallRequest
{
public:
   IChatCallRequest() : mJabberComponent(0), mB2BSessionHandle(0), mCancelled(false) {}
   IChatCallRequest(JabberComponent* jabberComponent, const std::string& to, const std::string& from, unsigned int handle) :
      mJabberComponent(jabberComponent), mTo(to), mFrom(from), mB2BSessionHandle(handle), mCancelled(false) {}

   // Client Methods
   void sendIChatVCRequest(const std::string& fullTo);
   void sendIChatVCCancelToAll();
   void receivedIChatVCResponse(const std::string& from);

   // Server Methods
   void sendIChatVCResponse(bool accept);

   JabberComponent* mJabberComponent;

   std::string mTo;
   std::string mFrom;
   unsigned int mB2BSessionHandle;
   bool mCancelled;
   std::set<std::string> mPendingVCRequestSet;
};

class JabberComponent : public Thread,
                        IPCHandler,
                        gloox::ConnectionListener,
                        gloox::PresenceHandler, 
                        gloox::MessageHandler, 
                        gloox::LogHandler, 
                        gloox::DiscoNodeHandler, 
                        gloox::IqHandler, 
                        gloox::SubscriptionHandler
{
public:
   JabberComponent(unsigned short jabberConnectorIPCPort,
                   unsigned short gatewayIPCPort,
                   const std::string& server, 
                   const std::string& component, 
                   const std::string& password, 
                   int port, 
                   unsigned int serverPingDuration,
                   const std::string& controlUser,
                   const std::string& localIChatPortListBlob);  
   ~JabberComponent();

   void stop();
   void disconnect();

   // Client Methods
   void initiateIChatCall(const std::string& to, const std::string& from, unsigned int handle, bool alertOneOnly=true);
   void cancelIChatCall(const std::string& to, const std::string& from);
      
   // Server Methods
   void proceedingIChatCall(const std::string& to, const std::string& from, unsigned int handle);
   void acceptIChatCall(const std::string& to, const std::string& from);  
   void rejectIChatCall(const std::string& to, const std::string& from);  

private:
   void probePresence(const std::string& to);
   void sendPresenceForRequest(gloox::Stanza* stanza);
   void sendPresence(const std::string& to, const std::string& from, bool advertiseIChatSupport, bool available);
   void sendSubscriptionResponse(const std::string& to, const std::string& from, bool success);

   // Interfaces to send IPC messages
   friend class IChatUser;
   void notifyIChatCallRequest(const std::string& to, const std::string& from);  
   void notifyIChatCallCancelled(unsigned int handle); 
   void notifyIChatCallProceeding(unsigned int handle, const std::string& to);
   void notifyIChatCallFailed(unsigned int handle, unsigned int statusCode);
   void continueIChatCall(unsigned int, const std::string& remoteIPPortListBlob);
   void sipRegisterJabberUser(const std::string& jidToRegister);
   void sipUnregisterJabberUser(const std::string& jidToUnregister);
   void checkSubscription(const std::string& to, const std::string& from);

   // Handlers
   virtual void onNewIPCMsg(const IPCMsg& msg);
   virtual void handleLog(gloox::LogLevel level, gloox::LogArea area, const std::string& message);
   virtual void onConnect();
   virtual void onDisconnect(gloox::ConnectionError e);
   virtual bool onTLSConnect(const gloox::CertInfo& info);
   virtual void handleSubscription(gloox::Stanza *stanza);
   virtual void handlePresence(gloox::Stanza *stanza);
   virtual void handleMessage(gloox::Stanza* stanza, gloox::MessageSession* session = 0);
   virtual bool handleIq(gloox::Stanza *stanza);
   virtual bool handleIqID(gloox::Stanza *stanza, int context);
   virtual gloox::StringList handleDiscoNodeFeatures(const std::string& node);
   virtual gloox::StringMap handleDiscoNodeIdentities(const std::string& node, std::string& name);
   virtual gloox::DiscoNodeItemList handleDiscoNodeItems(const std::string& node);

   virtual void thread();

   gloox::Component* mComponent;
   bool mStopping;
   unsigned int mServerPingDuration;
   std::string mControlJID;
   std::string mLocalIChatPortListBlob;

   // Outstanding IChat call request maps
   typedef std::map<std::string, IChatCallRequest> IChatCallRequestMap;
   friend class IChatCallRequest;
   std::string makeVCRequestKey(const std::string& bareTo, const std::string& bareFrom);

   IChatCallRequestMap mOutstandingClientIChatCallRequests;
   gloox::Mutex mOutstandingClientIChatCallRequestsMutex;
   IChatCallRequestMap::iterator findOutstandingClientIChatCallRequest(const std::string& bareTo, const std::string& bareFrom);
   void failOutstandingClientIChatCallRequest(const std::string& bareTo, const std::string& bareFrom, unsigned int code);
   void failOutstandingClientIChatCallRequest(const std::string& bareTo, unsigned int code);

   IChatCallRequestMap mOutstandingServerIChatCallRequests;
   gloox::Mutex mOutstandingServerIChatCallRequestsMutex;
   IChatCallRequestMap::iterator findOutstandingServerIChatCallRequest(const std::string& bareTo, const std::string& bareFrom);
   void cancelOutstandingServerIChatCallRequest(const std::string& bareTo, const std::string& bareFrom);

   gloox::Mutex mIChatUserMutex;
   typedef std::map<std::string, IChatUser*> IChatUserMap;
   IChatUserMap mIChatUsers;
   void storeIChatSubscribedUser(const std::string& user, const std::string& subscribedJID);
   void removeIChatSubscribedUser(const std::string& user, const std::string& subscribedJID);
   void storeIChatPresence(const gloox::JID& jid, const gloox::Presence& presence, int priority, bool avAvail);
   bool getMostAvailableIChatUserFullJID(const gloox::JID& jid, std::string& fullJID);
   bool getMostAvailableIChatUserFullJIDList(const gloox::JID& jid, std::list<std::string>& fullJIDList);

   JabberUserDb mUserDb;
   IPCThread mIPCThread;
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

