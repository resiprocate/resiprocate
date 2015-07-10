
#if defined( WIN32 )
#include <time.h>
#endif

#include <sstream>
#include "rutil/ResipAssert.h"
#include <algorithm>
#include "../Version.hxx"
#include "JabberComponent.hxx"
#include "IChatUser.hxx"

#ifndef RESIP_CONTRIB_GLOOX
#include <gloox/disco.h>
#include <gloox/mutex.h>
#else
#include <src/disco.h>
#include <src/mutex.h>
#endif

using namespace gateway;
using namespace gloox;
using namespace std;

extern void sleepSeconds(unsigned int seconds);

void
IChatCallRequest::sendIChatVCRequest(const std::string& fullTo)
{
   // Only allow one VCRequest per resource - this guards against receiving multiple
   // presence messages from the same resource
   if(mPendingVCRequestSet.find(fullTo) == mPendingVCRequestSet.end())
   {
      mJabberComponent->notifyIChatCallProceeding(mB2BSessionHandle, fullTo); 

      resip_assert(mJabberComponent);
      mJabberComponent->sendPresence(fullTo, mJabberComponent->mControlJID, false /* advertiseIChatSupport */, true /* available */);  // Doing this let's us push the control presence as well send calls

      std::string id = mJabberComponent->mComponent->getID();
   
      Tag *iq = new Tag("iq");
      iq->addAttribute("type", "set");
      iq->addAttribute("id", id);
      iq->addAttribute("to", fullTo.c_str());
      iq->addAttribute("from", mFrom.c_str());
      Tag *query = new Tag(iq, "query");
      query->addAttribute("xmlns", "apple:iq:vc:request");
      new Tag(query, "VCNewCallerIPPortData", mJabberComponent->mLocalIChatPortListBlob);
      new Tag(query, "isAudioOnly", "1");
      new Tag(query, "extSIPPort", "0");
      new Tag(query, "extIPAddr", "127.0.0.1");
      new Tag(query, "VCProtocolVersion", "1");
      mJabberComponent->mComponent->send(iq);

      mPendingVCRequestSet.insert(fullTo);
   }
}

void 
IChatCallRequest::receivedIChatVCResponse(const std::string& from)
{
   std::set<std::string>::iterator it = mPendingVCRequestSet.find(from);
   mPendingVCRequestSet.erase(it);  // Remove responded JID from list and cancel all others
   sendIChatVCCancelToAll();
}

void
IChatCallRequest::sendIChatVCCancelToAll()
{
   std::set<std::string>::iterator it = mPendingVCRequestSet.begin();
   std::string id = mJabberComponent->mComponent->getID();

   // Send a cancel out for each VCRequest
   for(;it!=mPendingVCRequestSet.end();it++)
   {
      Tag *iq = new Tag("iq");
      iq->addAttribute("type", "set");
      iq->addAttribute("id", id);
      iq->addAttribute("to", it->c_str());
      iq->addAttribute("from", mFrom.c_str());
      Tag *query = new Tag(iq, "query");
      query->addAttribute("xmlns", "apple:iq:vc:cancel");
      new Tag(query, "VCProtocolVersion", "1");
      mJabberComponent->mComponent->send(iq);
   }
   mPendingVCRequestSet.clear();
}

void 
IChatCallRequest::sendIChatVCResponse(bool accept)
{
   Tag *iq = new Tag( "iq" );
   iq->addAttribute( "type", "set" );
   iq->addAttribute( "id", mJabberComponent->mComponent->getID() );
   iq->addAttribute( "to", mFrom );
   iq->addAttribute( "from", mTo );
   Tag *query = new Tag( iq, "query" );
   query->addAttribute( "xmlns", "apple:iq:vc:response");
   if(accept)
   {
     new Tag( query, "response", "0" );
     new Tag( query, "connectData", mJabberComponent->mLocalIChatPortListBlob);
   }
   else
   {
     new Tag( query, "response", "1" );
     new Tag( query, "connectData", "");
   }
   //new Tag( query, "responseData", "1" );  // doesn't appear to be required
   new Tag( query, "VCProtocolVersion", "1" );
   mJabberComponent->mComponent->send(iq);
}

class IPCMutexGloox : public IPCMutex
{
public:
   IPCMutexGloox() {}
   virtual ~IPCMutexGloox() {}
   virtual void lock() { mMutex.lock(); }
   virtual void unlock() { mMutex.unlock(); }
private:
   gloox::Mutex mMutex;
};
IPCMutexGloox g_IPCGlooxMutex;

JabberComponent::JabberComponent(unsigned short jabberConnectorIPCPort,
                                 unsigned short gatewayIPCPort,
                                 const std::string& server, 
                                 const std::string& component, 
                                 const std::string& password, 
                                 int port, 
                                 unsigned int serverPingDuration,
                                 const std::string& controlUser,
                                 const std::string& localIChatPortListBlob) 
   : mStopping(false),
     mServerPingDuration(serverPingDuration),
     mLocalIChatPortListBlob(localIChatPortListBlob),
     mIPCThread(jabberConnectorIPCPort, gatewayIPCPort, this, &g_IPCGlooxMutex)
{
   mIPCThread.run();

   mComponent = new Component("jabber:component:accept", server, component, password, port);

   mComponent->registerMessageHandler(this);
   mComponent->registerConnectionListener(this);
   mComponent->registerPresenceHandler(this);
   mComponent->registerSubscriptionHandler(this);
   mComponent->registerIqHandler(this, "apple:iq:vc:request");
   mComponent->registerIqHandler(this, "apple:iq:vc:cancel");
   mComponent->registerIqHandler(this, "apple:iq:vc:response");
   mComponent->registerIqHandler(this, "apple:iq:vc:counterProposal");
   mComponent->logInstance().registerLogHandler(LogLevelDebug, LogAreaAll, this);

   mComponent->disco()->registerNodeHandler(this, "apple:ichat:caps#avavail");
   mComponent->disco()->registerNodeHandler(this, "apple:ichat:caps#audio");
   mComponent->disco()->registerNodeHandler(this, "apple:ichat:caps#avcap");
   mComponent->disco()->registerNodeHandler(this, "apple:ichat:caps#448");
   mComponent->disco()->setIdentity("server", "ichat-gw");
   mComponent->disco()->setVersion("ichat-gw", ICHATGW_VERSION_STRING);

   //mComponent->setTls(tlsPolicy);  // This setting appears to have no effect on Component connections

   mControlJID = controlUser + "@" + component;
}

JabberComponent::~JabberComponent()
{
   mIPCThread.shutdown();
   mIPCThread.join();
}

void 
JabberComponent::thread()
{
   handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::thread - starting...");
   while(!mStopping)
   {
      mComponent->connect(false);
      ConnectionError rc;
      time_t lastRecvTime=time(0);
      while((rc=mComponent->recv(mServerPingDuration * 1000000)) == ConnNoError)
      {
         time_t now = time(0);
         if(now-lastRecvTime >= mServerPingDuration)
         {
            mComponent->whitespacePing();
         }
         lastRecvTime = now;
      }

      if(!mStopping)
      {
         std::ostringstream oss;
         oss << "JabberComponent::thread - recv error, rc=" << rc;
         handleLog(gloox::LogLevelError, gloox::LogAreaUser, oss.str());
         // Wait 10 seconds then try again
         sleepSeconds(10);
         if(mComponent->state() != gloox::StateDisconnected)
         {
            mComponent->disconnect();
         }
      }
   }
   handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::thread - shutdown.");
}

void 
JabberComponent::stop()
{
   mStopping = true;
   disconnect();
}

void 
JabberComponent::disconnect()
{
   if(mComponent->state() == gloox::StateDisconnected) return;

   mIChatUserMutex.lock();

   // Tell users that control user is now offline
   IChatUserMap::iterator it = mIChatUsers.begin();
   for(;it!=mIChatUsers.end();it++)
   {
      // Notify user that subscribed users are now offline
      const IChatUser::SubscribedGatewayUserList& subscribedUserList = it->second->getSubscribedGatewayUserList();
      IChatUser::SubscribedGatewayUserList::const_iterator it2 = subscribedUserList.begin();
      for(;it2!=subscribedUserList.end();it2++)
      {
         sendPresence(it->first, *it2, false, false /* available? */);
      }
      delete it->second;
   }
   mIChatUsers.clear();

   mIChatUserMutex.unlock();

   mComponent->disconnect();
}

void
JabberComponent::initiateIChatCall(const std::string& to, const std::string& from, unsigned int handle, bool alertOneOnly)
{
   JID jid(to);
   mOutstandingClientIChatCallRequestsMutex.lock();

   // Add call request data to map
   std::string key = makeVCRequestKey(jid.bare(),from);
   IChatCallRequest* iChatCallRequest = &(mOutstandingClientIChatCallRequests[key] = IChatCallRequest(this, to, from, handle));
   handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::initiateiChatCall - key=" + key);

   // If there is no resource, then we need to query the bare JID and find iChat resources
   if(jid.resource().empty())
   {
      bool found = false;
      if(alertOneOnly)
      {
         // First check if we have any info cached locally
         std::string fullJID;
         if(getMostAvailableIChatUserFullJID(jid, fullJID))  
         {
            found = true;
            iChatCallRequest->sendIChatVCRequest(fullJID);
         }
      }
      else
      {
         std::list<std::string> fullJIDList;
         if(getMostAvailableIChatUserFullJIDList(jid, fullJIDList))
         {
            found = true;
            std::list<std::string>::iterator it = fullJIDList.begin();
            for(;it!=fullJIDList.end();it++)
            {
               iChatCallRequest->sendIChatVCRequest(*it);
            }
         }
      }

      if(!found)
      {
         // No local info - try to probe
         Tag *iq = new Tag("presence");
         iq->addAttribute("type", "probe");
         iq->addAttribute("to", to.c_str());
         iq->addAttribute("from", mControlJID);
         mComponent->send(iq);
      }
   }
   else // We have full JID - send the vc-request directly to the resource
   {
      iChatCallRequest->sendIChatVCRequest(to);
   }
   mOutstandingClientIChatCallRequestsMutex.unlock();
}

void 
JabberComponent::cancelIChatCall(const std::string& to, const std::string& from)
{
   JID jid(to);
   mOutstandingClientIChatCallRequestsMutex.lock();

   IChatCallRequestMap::iterator it = findOutstandingClientIChatCallRequest(jid.bare(), from);
   if(it != mOutstandingClientIChatCallRequests.end())
   {
      it->second.sendIChatVCCancelToAll();
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::cancelIChatCall - success");
      mOutstandingClientIChatCallRequests.erase(it);
   }
   else
   {
      handleLog(gloox::LogLevelWarning, gloox::LogAreaUser, "JabberComponent::cancelIChatCall - not found");
   }
   mOutstandingClientIChatCallRequestsMutex.unlock();
}

void 
JabberComponent::proceedingIChatCall(const std::string& to, const std::string& from, unsigned int handle)
{
   mOutstandingServerIChatCallRequestsMutex.lock();
   IChatCallRequestMap::iterator it = findOutstandingServerIChatCallRequest(to, from);
   if(it!=mOutstandingServerIChatCallRequests.end())
   {
      // Store session handle for notifications to SIP layer
      resip_assert(it->second.mB2BSessionHandle == 0);
      std::ostringstream oss;
      oss << "JabberComponent::proceedingIChatCall - set handle to " << handle;
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, oss.str());
      it->second.mB2BSessionHandle = handle;

      // If already cancelled, then notify SIP layer now
      if(it->second.mCancelled)
      {
         notifyIChatCallCancelled(it->second.mB2BSessionHandle); 
         mOutstandingServerIChatCallRequests.erase(it);
      }
   }    
   mOutstandingServerIChatCallRequestsMutex.unlock();
}

void 
JabberComponent::acceptIChatCall(const std::string& to, const std::string& from)
{
   mOutstandingServerIChatCallRequestsMutex.lock();
   IChatCallRequestMap::iterator it = findOutstandingServerIChatCallRequest(to, from);
   if(it!=mOutstandingServerIChatCallRequests.end())
   {
      it->second.sendIChatVCResponse(true);
      mOutstandingServerIChatCallRequests.erase(it);
   }    
   mOutstandingServerIChatCallRequestsMutex.unlock();
}

void 
JabberComponent::rejectIChatCall(const std::string& to, const std::string& from)
{
   mOutstandingServerIChatCallRequestsMutex.lock();
   IChatCallRequestMap::iterator it = findOutstandingServerIChatCallRequest(to, from);
   if(it!=mOutstandingServerIChatCallRequests.end())
   {
      it->second.sendIChatVCResponse(false);
      mOutstandingServerIChatCallRequests.erase(it);
   }    
   mOutstandingServerIChatCallRequestsMutex.unlock();
}

std::string 
JabberComponent::makeVCRequestKey(const std::string& bareTo, const std::string& bareFrom)
{
   std::string key = bareTo + "|" + bareFrom;
#ifdef WIN32
   std::transform(key.begin(), key.end(), key.begin(), tolower);
#else
   std::transform(key.begin(), key.end(), key.begin(),  (int(*)(int))std::tolower);
#endif
   return key;
}

JabberComponent::IChatCallRequestMap::iterator
JabberComponent::findOutstandingClientIChatCallRequest(const std::string& bareTo, const std::string& bareFrom)
{
   std::string key = makeVCRequestKey(bareTo, bareFrom);
   handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::findOutstandingClientIChatCallRequest - key=" + key);
   return mOutstandingClientIChatCallRequests.find(key);
}

void
JabberComponent::failOutstandingClientIChatCallRequest(const std::string& bareTo, const std::string& bareFrom, unsigned int code)
{
   mOutstandingClientIChatCallRequestsMutex.lock();
   IChatCallRequestMap::iterator it = findOutstandingClientIChatCallRequest(bareTo, bareFrom);
   if(it!=mOutstandingClientIChatCallRequests.end())
   {
      notifyIChatCallFailed(it->second.mB2BSessionHandle, code); 
      mOutstandingClientIChatCallRequests.erase(it);
   }    
   mOutstandingClientIChatCallRequestsMutex.unlock();
}

void
JabberComponent::failOutstandingClientIChatCallRequest(const std::string& bareTo, unsigned int code)
{
   mOutstandingClientIChatCallRequestsMutex.lock();
   IChatCallRequestMap::iterator it = mOutstandingClientIChatCallRequests.begin();
   while(it != mOutstandingClientIChatCallRequests.end())
   {
      if(it->second.mTo == bareTo)
      {
         notifyIChatCallFailed(it->second.mB2BSessionHandle, code);   
         mOutstandingClientIChatCallRequests.erase(it++);
      }
      else
      {
         it++;
      }
   }
   mOutstandingClientIChatCallRequestsMutex.unlock();
}

JabberComponent::IChatCallRequestMap::iterator
JabberComponent::findOutstandingServerIChatCallRequest(const std::string& bareTo, const std::string& bareFrom)
{
   std::string key = makeVCRequestKey(bareTo, bareFrom);
   handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::findOutstandingServerIChatCallRequest - key=" + key);
   return mOutstandingServerIChatCallRequests.find(key);
}

void 
JabberComponent::cancelOutstandingServerIChatCallRequest(const std::string& bareTo, const std::string& bareFrom)
{
   mOutstandingServerIChatCallRequestsMutex.lock();
   IChatCallRequestMap::iterator it = findOutstandingServerIChatCallRequest(bareTo, bareFrom);
   if(it!=mOutstandingServerIChatCallRequests.end())
   {
      if(it->second.mB2BSessionHandle != 0)
      {
         handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::cancelOutstandingServerIChatCallRequest - to=" + bareTo + " from=" + bareFrom);
         notifyIChatCallCancelled(it->second.mB2BSessionHandle); 
         mOutstandingServerIChatCallRequests.erase(it);
      }
      else
      {
         // We don't have a session handle yet, so flag request as cancelled, 
         // when session handle arrives, call notifyIChatCallCancelled and remove entry
         it->second.mCancelled = true;  
         handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::cancelOutstandingServerIChatCallRequest - no session handle yet, pending - to=" + bareTo + " from=" + bareFrom);
      }
   }    
   mOutstandingServerIChatCallRequestsMutex.unlock();
}

void 
JabberComponent::probePresence(const std::string& to)
{
   Tag *presence = new Tag("presence");
   presence->addAttribute("type", "probe");
   presence->addAttribute("to", to);
   presence->addAttribute("from", mControlJID);
   mComponent->send(presence);
}

void 
JabberComponent::sendPresenceForRequest(Stanza* stanza)
{
   sendPresence(stanza->from().bare(), stanza->to().full(), stanza->to().bare() != mControlJID, true /* available */);
}

void 
JabberComponent::sendPresence(const std::string& to, const std::string& from, bool advertiseIChatSupport, bool available)
{
   Tag *presence = new Tag("presence");
   if(!available)
   {
      presence->addAttribute("type", "unavailable");
   }
   presence->addAttribute("to", to);
   presence->addAttribute("from", from);
   new Tag(presence, "priority", "0");
   if(advertiseIChatSupport)
   {
      Tag* c = new Tag(presence, "c");
      c->addAttribute("xmlns", "http://jabber.org/protocol/caps");
      c->addAttribute("node", "apple:ichat:caps"); 
      c->addAttribute("ver", "448");   // Note:  Use version 448 so we don't collide with actual iChat version
      c->addAttribute("ext", "avavail avcap audio");
   }
   mComponent->send(presence);
}

void
JabberComponent::sendSubscriptionResponse(const std::string& to, const std::string& from, bool success)
{
   if(success)
   {
      Tag *p = new Tag( "presence" );
      p->addAttribute( "type", "subscribed" );
      p->addAttribute( "to", to );
      p->addAttribute( "from", from );
      mComponent->send( p );
   
      mUserDb.addSubscribedJID(to.c_str(), from.c_str());
      storeIChatSubscribedUser(to, from);
      sendPresence(to, from, from != mControlJID, true);

      // Subscribe back to client to ensure we are subscribed to client
      Tag *iq = new Tag("presence");
      iq->addAttribute("type", "subscribe");
      iq->addAttribute("to", to);
      iq->addAttribute("from", mControlJID);
      mComponent->send(iq);
   }
   else
   {
      Tag *p = new Tag( "presence" );
      p->addAttribute( "type", "unsubscribed" );
      p->addAttribute( "to", to );
      p->addAttribute( "from", from );
      mComponent->send( p );
   }
}

void 
JabberComponent::removeIChatSubscribedUser(const std::string& user, const std::string& subscribedJID)
{
   mIChatUserMutex.lock();

   IChatUserMap::iterator it = mIChatUsers.find(user);
   if(it != mIChatUsers.end())
   {
      it->second->removeSubscribedGatewayUser(subscribedJID);
   }

   mIChatUserMutex.unlock();
}

void 
JabberComponent::storeIChatSubscribedUser(const std::string& user, const std::string& subscribedJID)
{
   mIChatUserMutex.lock();

   IChatUserMap::iterator it = mIChatUsers.find(user);
   if(it == mIChatUsers.end())
   {
      // User is not yet present in map
      IChatUser* iChatUser = new IChatUser(*this, user);
      iChatUser->addSubscribedGatewayUser(subscribedJID);
      mIChatUsers[user] = iChatUser;
   }
   else
   {
      it->second->addSubscribedGatewayUser(subscribedJID);
   }

   mIChatUserMutex.unlock();
}

void 
JabberComponent::storeIChatPresence(const gloox::JID& jid, const gloox::Presence& presence, int priority, bool avAvail)
{
   mIChatUserMutex.lock();

   IChatUserMap::iterator it = mIChatUsers.find(jid.bare());
   if(it == mIChatUsers.end())
   {
      if(presence != gloox::PresenceUnavailable && !jid.resource().empty())
      {
         // User is not yet present in map
         IChatUser* iChatUser = new IChatUser(*this, jid.bare());
         iChatUser->updateResourceInfo(jid.resource(), presence, priority, avAvail);
         mIChatUsers[jid.bare()] = iChatUser;
      }
   }
   else
   {
      it->second->updateResourceInfo(jid.resource(), presence, priority, avAvail);
   }

   mIChatUserMutex.unlock();
}

bool 
JabberComponent::getMostAvailableIChatUserFullJID(const gloox::JID& jid, std::string& fullJID)
{                
   bool ret = false;
   mIChatUserMutex.lock();

   IChatUserMap::iterator it = mIChatUsers.find(jid.bare());
   if(it != mIChatUsers.end())
   {
      std::string resource = it->second->getMostAvailableResource();
      if(!resource.empty())
      {
         JID temp(jid);
         temp.setResource(resource);
         fullJID = temp.full();
         ret = true;
      }
   }

   mIChatUserMutex.unlock();

   return ret;
}

bool 
JabberComponent::getMostAvailableIChatUserFullJIDList(const gloox::JID& jid, std::list<std::string>& fullJIDList)
{
   bool ret = false;
   mIChatUserMutex.lock();

   IChatUserMap::iterator it = mIChatUsers.find(jid.bare());
   if(it != mIChatUsers.end())
   {
      std::list<std::string> resourceList;
      if(it->second->getMostAvailableResourceList(resourceList))
      {
         std::list<std::string>::iterator listIt = resourceList.begin();
         for(;listIt!=resourceList.end();listIt++)
         {
            if(!listIt->empty())
            {
               JID temp(jid);
               temp.setResource(*listIt);
               fullJIDList.push_back(temp.full());
               ret = true;
            }
         }
      }
   }

   mIChatUserMutex.unlock();

   return ret;
}

void 
JabberComponent::notifyIChatCallRequest(const std::string& to, const std::string& from)
{
   IPCMsg msg;
   msg.addArg("notifyIChatCallRequest");
   msg.addArg(to.c_str());
   msg.addArg(from.c_str());
   mIPCThread.sendIPCMsg(msg);
}

void 
JabberComponent::notifyIChatCallCancelled(unsigned int handle)
{
   IPCMsg msg;
   msg.addArg("notifyIChatCallCancelled");
   msg.addArg(handle);
   mIPCThread.sendIPCMsg(msg);
}

void 
JabberComponent::notifyIChatCallProceeding(unsigned int handle, const std::string& to)
{
   IPCMsg msg;
   msg.addArg("notifyIChatCallProceeding");
   msg.addArg(handle);
   msg.addArg(to.c_str());
   mIPCThread.sendIPCMsg(msg);
}

void 
JabberComponent::notifyIChatCallFailed(unsigned int handle, unsigned int statusCode)
{
   IPCMsg msg;
   msg.addArg("notifyIChatCallFailed");
   msg.addArg(handle);
   msg.addArg(statusCode);
   mIPCThread.sendIPCMsg(msg);
}

void 
JabberComponent::continueIChatCall(unsigned int handle, const std::string& remoteIPPortListBlob)
{
   IPCMsg msg;
   msg.addArg("continueIChatCall");
   msg.addArg(handle);
   msg.addArg(remoteIPPortListBlob.c_str());
   mIPCThread.sendIPCMsg(msg);
}
 
void 
JabberComponent::sipRegisterJabberUser(const std::string& jidToRegister)
{
   IPCMsg msg;
   msg.addArg("sipRegisterJabberUser");
   msg.addArg(jidToRegister.c_str());
   mIPCThread.sendIPCMsg(msg);
}

void 
JabberComponent::sipUnregisterJabberUser(const std::string& jidToUnregister)
{
   IPCMsg msg;
   msg.addArg("sipUnregisterJabberUser");
   msg.addArg(jidToUnregister.c_str());
   mIPCThread.sendIPCMsg(msg);
}

void 
JabberComponent::checkSubscription(const std::string& to, const std::string& from)
{
   IPCMsg msg;
   msg.addArg("checkSubscription");
   msg.addArg(to.c_str());
   msg.addArg(from.c_str());
   mIPCThread.sendIPCMsg(msg);
}

void 
JabberComponent::onNewIPCMsg(const IPCMsg& msg)
{
   const std::vector<std::string>& args = msg.getArgs();
   resip_assert(args.size() >= 1);
   if(args.at(0) == "initiateIChatCall")
   {
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::onNewIPCMsg - initiateIChatCall");
      resip_assert(args.size() == 4);
      initiateIChatCall(args.at(1).c_str(), args.at(2).c_str(), atoi(args.at(3).c_str()), false /* TODO - make setting? */);
   }
   else if(args.at(0) == "cancelIChatCall")
   {
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::onNewIPCMsg - cancelIChatCall");
      resip_assert(args.size() == 3);
      cancelIChatCall(args.at(1).c_str(), args.at(2).c_str());
   }
   else if(args.at(0) == "proceedingIChatCall")
   {
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::onNewIPCMsg - proceedingIChatCall");
      resip_assert(args.size() == 4);
      proceedingIChatCall(args.at(1).c_str(), args.at(2).c_str(), atoi(args.at(3).c_str()));
   }
   else if(args.at(0) == "acceptIChatCall")
   {
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::onNewIPCMsg - acceptIChatCall");
      resip_assert(args.size() == 3);
      acceptIChatCall(args.at(1).c_str(), args.at(2).c_str());
   }
   else if(args.at(0) == "rejectIChatCall")
   {
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::onNewIPCMsg - rejectIChatCall");
      resip_assert(args.size() == 3);
      rejectIChatCall(args.at(1).c_str(), args.at(2).c_str());
   }
   else if(args.at(0) == "sendSubscriptionResponse")
   {
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::onNewIPCMsg - sendSubscriptionResponse");
      resip_assert(args.size() == 4);
      sendSubscriptionResponse(args.at(1).c_str(), args.at(2).c_str(), atoi(args.at(3).c_str()) != 0);
   }
   else
   {
      resip_assert(false);
   }
}

void 
JabberComponent::handleLog(LogLevel level, LogArea area, const std::string& message)
{
   IPCMsg msg;
   msg.addArg("log");

   switch(level)
   {
   case LogLevelWarning:  /**< Non-crititcal warning messages. */
      msg.addArg("warning");
      break;
   case LogLevelError:    /**< Critical, unrecoverable errors. */
      msg.addArg("error");
      break;
   case LogLevelDebug:    /**< Debug messages. */
   default:
      msg.addArg("info");
      break;
   }
   msg.addArg(message.c_str());
   mIPCThread.sendIPCMsg(msg);
}

void 
JabberComponent::onConnect()
{
   // connection established, auth done (see API docs for exceptions)
   handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::onConnect");

   // Get persisent User subscriptions - and send on-line message
   const JabberUserDb::UserMap& users = mUserDb.getUserSubscriptions();
   JabberUserDb::UserMap::const_iterator it = users.begin();
   for(;it!=users.end();it++)
   {
      probePresence(it->first.c_str());  // Probe for users presence
      JabberUserDb::SubscribeSet::const_iterator it2 = it->second.begin();
      for(;it2!=it->second.end();it2++)
      {
         sendPresence(it->first.c_str(), it2->c_str(), it2->c_str() != mControlJID /* advertise iChat audio support */, true /* available */);
         storeIChatSubscribedUser(it->first.c_str(), it2->c_str());
      }
   }
}


void 
JabberComponent::onDisconnect(ConnectionError e)
{
   // connection established, auth done (see API docs for exceptions)
   std::ostringstream oss;
   oss << "JabberComponent::onDisconnect - error=" << e;
   if(mStopping)
   {
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, oss.str());
   }
   else
   {
      handleLog(gloox::LogLevelWarning, gloox::LogAreaUser, oss.str());
   }
}

bool 
JabberComponent::onTLSConnect(const CertInfo& info)
{
   // Note:  currently gloox components to not support TLS connections

   // examine certificate info
   handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::onTLSConnect");
   return true;
}

void 
JabberComponent::handleSubscription( Stanza *stanza )
{
   switch( stanza->subtype() )
   {
      case StanzaS10nSubscribe:
      {
         handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::handleSubscription - StanzaS10nSubscribe");
         if(stanza->to().full() == mControlJID)
         {
            sendSubscriptionResponse(stanza->from().bare(), stanza->to().full(), true);
         }
         else
         {
            checkSubscription(stanza->to().full(), stanza->from().bare());
         }
         break;
      }
      case StanzaS10nUnsubscribe:
      {
         handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::handleSubscription - StanzaS10nUnsubscribe");
         Tag *p = new Tag( "presence" );
         p->addAttribute( "type", "unsubscribed" );
         p->addAttribute( "to", stanza->from().bare() );
         p->addAttribute( "from", stanza->to().full() );
         mComponent->send( p );

         mUserDb.removeSubscribedJID(stanza->from().bare().c_str(), stanza->to().bare().c_str());
         removeIChatSubscribedUser(stanza->from().bare(), stanza->to().bare());

         break;
      }
      default:
      {
         handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::handleSubscription - subtype=" + stanza->subtype());
         break;
      }
   }
}

void 
JabberComponent::handlePresence(Stanza *stanza)
{
   bool iChatResource = false;
   bool avAvail=false;

   // Check if iChat endpoint
   Tag* c = stanza->findChild("c");
   if(c)
   {
      std::string node = c->findAttribute("node");
      if(node == "apple:ichat:caps" ||
         node == "http://www.apple.com/ichat/caps")
      {
         iChatResource = true;

         // Check if caps include AV Available capability (note:  if iChat is already on a call then it does not have avavail capability - since iChat only allows one call at a time)
         std::string ext = c->findAttribute("ext");
         if(ext.find("avavail") != std::string::npos)
         {
            avAvail = true;
         }
      }
   }

   // presence info
   switch(stanza->subtype())
   {
   case StanzaPresenceProbe:
      // A request for an entity's current presence
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::handlePresence - Probe from " + stanza->from().full());
      sendPresenceForRequest(stanza);
      break;

   case StanzaPresenceAvailable:
      // Signals to the server that the sender is online and available for communication.

      if(iChatResource)
      {
         handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::handlePresence - PresenceAvailable from " + stanza->from().full());
         // Ensure we are tracking client
         storeIChatPresence(stanza->from(), stanza->presence(), stanza->priority(), avAvail);

         // Check if we have an outstanding iChat Call request
         if(avAvail)
         {
            mOutstandingClientIChatCallRequestsMutex.lock();
            IChatCallRequestMap::iterator it = mOutstandingClientIChatCallRequests.begin();
            bool callRequestFound = false;
            while(it != mOutstandingClientIChatCallRequests.end())
            {
               if(it->second.mTo == stanza->from().bare())
               {
                  callRequestFound = true;
                  handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::handlePresence - Available for " + stanza->from().full() + " - outstanding iChat call request - continuing call");
   
                  it->second.sendIChatVCRequest(stanza->from().full());
               }
               it++;
            }
            mOutstandingClientIChatCallRequestsMutex.unlock();
         }
      }
      break;

   case StanzaPresenceUnavailable:
      // Signals that the entity is no longer available for communication.
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::handlePresence - Unavailable for " + stanza->from().full());

      // Ensure we are tracking client
      storeIChatPresence(stanza->from(), stanza->presence(), stanza->priority(), avAvail);

      // Check if we have an outstanding iChat Call request to fail
      failOutstandingClientIChatCallRequest(stanza->from().bare(), 404);
      break;

   case StanzaPresenceError:
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::handlePresence - Error for " + stanza->from().full());

      {
         unsigned int code = 0;
         Tag *error = stanza->findChild( "error" );
         if(error)
         {
            code = atoi(error->findAttribute("code").c_str());
         }

         // Check if we have an outstanding iChat Call request to fail
         failOutstandingClientIChatCallRequest(stanza->from().bare(), code);
      }
      break;

   default:
      break;
   }
}

void 
JabberComponent::handleMessage(Stanza* stanza, MessageSession* session)
{
   std::ostringstream oss;
   oss << "JabberComponent::handlePresence - " << *stanza;
   handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, oss.str());
   Stanza *s = Stanza::createMessageStanza(stanza->from().full(), "You have reached the ichat gateway!" );
   s->addAttribute("from", stanza->to().full());

   mComponent->send( s );
}

bool 
JabberComponent::handleIq(Stanza *stanza)
{
   if(stanza->subtype() == StanzaIqSet && stanza->xmlns() == "apple:iq:vc:request")
   {
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "Jabber::handleIq - apple:iq:vc:request from=" + stanza->from().full() + ", to=" + stanza->to().full());
      Tag::TagList tlist = stanza->children();
      Tag* query = stanza->findChild("query");
      Tag* vcNewCallerIPPortDataTag;
      if(query)
      {
         vcNewCallerIPPortDataTag = query->findChild("VCNewCallerIPPortData");
         if(vcNewCallerIPPortDataTag)
         {               
            std::ostringstream oss;
            oss << "Jabber::handleIq: vc:request, VCNewCallerIPPortData=" << vcNewCallerIPPortDataTag->cdata() << " size=" << vcNewCallerIPPortDataTag->cdata().size();
            handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, oss.str());
         }
         Tag* extSIPPort = query->findChild("extSIPPort");
         if(extSIPPort)
         {
            handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "Jabber::handleIq - apple:iq:vc:request extSIPPort=" + extSIPPort->cdata());
         }
         Tag* extIPAddr = query->findChild("extIPAddr");
         if(extIPAddr)
         {
            handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "Jabber::handleIq - apple:iq:vc:request extIPAddr=" + extIPAddr->cdata());
         }
         Tag* vcProtocolVersion = query->findChild("VCProtocolVersion");
         if(vcProtocolVersion)
         {
            handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "Jabber::handleIq - apple:iq:vc:request VCProtocolVersion=" + vcProtocolVersion->cdata());
         }
      }

      // Create entry in outstanding server requests map
      std::string key = makeVCRequestKey(stanza->to().bare(),stanza->from().bare());
      mOutstandingServerIChatCallRequestsMutex.lock();
      IChatCallRequest* iChatCallRequest = &(mOutstandingServerIChatCallRequests[key] = IChatCallRequest(this, stanza->to().bare(), stanza->from().full(), 0));
      mOutstandingServerIChatCallRequestsMutex.unlock();

      // Pass request to SIP side
      notifyIChatCallRequest(stanza->to().bare(), stanza->from().bare());
   }
   else if(stanza->subtype() == StanzaIqSet && stanza->xmlns() == "apple:iq:vc:counterProposal")
   {
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "Jabber::handleIq - apple:iq:vc:counterProposal from=" + stanza->from().full() + ", to=" + stanza->to().full());

      // Build response - Do we even need to respond???
      {
         Tag *iq = new Tag( "iq" );
         iq->addAttribute( "type", "set" );
         iq->addAttribute( "id", mComponent->getID() );
         iq->addAttribute( "to", stanza->from().full() );
         iq->addAttribute( "from", stanza->to().full() );
         Tag *query = new Tag( iq, "query" );
         query->addAttribute( "xmlns", "apple:iq:vc:counterProposal");
         new Tag( query, "connectData", mLocalIChatPortListBlob);
         new Tag( query, "VCProtocolVersion", "1" );
         mComponent->send(iq);
      }
   }
   else if(stanza->subtype() == StanzaIqSet && stanza->xmlns() == "apple:iq:vc:cancel")
   {
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "Jabber::handleIq - apple:iq:vc:cancel from=" + stanza->from().full() + ", to=" + stanza->to().full());
      cancelOutstandingServerIChatCallRequest(stanza->to().bare(), stanza->from().bare());
   }
   else if(stanza->subtype() == StanzaIqSet && stanza->xmlns() == "apple:iq:vc:response")
   {
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "Jabber::handleIq - apple:iq:vc:response from=" + stanza->from().full() + ", to=" + stanza->to().full());
      Tag::TagList tlist = stanza->children();
      Tag* query = stanza->findChild("query");
      Tag* vcNewCallerIPPortDataTag;
      if(query)
      {
         vcNewCallerIPPortDataTag = query->findChild("connectData");
         if(vcNewCallerIPPortDataTag)
         {               
            mOutstandingClientIChatCallRequestsMutex.lock();

            std::ostringstream oss;
            oss << "Jabber::handleIq - connectData=" << vcNewCallerIPPortDataTag->cdata() << " size=" << vcNewCallerIPPortDataTag->cdata().size();
            handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, oss.str());

            IChatCallRequestMap::iterator it = findOutstandingClientIChatCallRequest(stanza->from().bare(), stanza->to().bare());
            if(it!=mOutstandingClientIChatCallRequests.end())
            {
               handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "Jabber::handleIq - apple:iq:vc:response received - continuing SIP portion of call...");
               it->second.receivedIChatVCResponse(stanza->from().full());
               continueIChatCall(it->second.mB2BSessionHandle, vcNewCallerIPPortDataTag->cdata());    
               mOutstandingClientIChatCallRequests.erase(it);
            }
            mOutstandingClientIChatCallRequestsMutex.unlock();
         }
      }
   }
   else if(stanza->subtype() == StanzaIqError && stanza->xmlns() == "apple:iq:vc:request")
   {
      unsigned int code = 0;
      Tag *error = stanza->findChild( "error" );
      if(error)
      {
        code = atoi(error->findAttribute("code").c_str());
      }

      failOutstandingClientIChatCallRequest(stanza->from().bare(), stanza->to().bare(), code);
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "Jabber::handleIq - apple:iq:vc:request error received - notifying SIP portion of call...");
   }
   else
   {
      std::ostringstream oss;
      oss << "Jabber::handleIq - " << *stanza;
      handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, oss.str());
   }
   return true;
}

bool 
JabberComponent::handleIqID(Stanza *stanza, int context)
{
   std::ostringstream oss;
   oss << "Jabber::handleIqID - context=" << context;
   handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, oss.str());
   return false;
}

StringList 
JabberComponent::handleDiscoNodeFeatures(const std::string& node)
{
   handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::handleDiscoNodeFeatures - node=" + node);
   StringList slist;
   if(node == "apple:ichat:caps#avavail")
   {
      slist.push_back("apple:iq:vc:available");
   }
   else if(node == "apple:ichat:caps#audio")
   {
      slist.push_back("apple:iq:vc:audio");
   }
   else if(node == "apple:ichat:caps#avcap")
   {
      slist.push_back("apple:iq:vc:capable");
   }
   else if(node == "apple:ichat:caps#448")
   {
      slist.push_back("jabber:iq:version");
   }
   return slist;
}

StringMap 
JabberComponent::handleDiscoNodeIdentities(const std::string& node, std::string& name)
{
   handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::handleDiscoNodeIdentities - node=" + node + ", name=" + name);
   StringMap smap;
   smap["client"] = "pc";
   return smap;
}

DiscoNodeItemList 
JabberComponent::handleDiscoNodeItems(const std::string& node)
{
   handleLog(gloox::LogLevelDebug, gloox::LogAreaUser, "JabberComponent::handleDiscoNodeItems - node=" + node);
   DiscoNodeItemList dlist;
   return dlist;
}


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

