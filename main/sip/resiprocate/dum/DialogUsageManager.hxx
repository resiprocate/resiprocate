#if !defined(RESIP_DIALOGUSAGEMANAGER_HXX)
#define RESIP_DIALOGUSAGEMANAGER_HXX

#include <list>

#include "resiprocate/os/BaseException.hxx"
#include "DialogSet.hxx"

#include "BaseUsage.hxx"
#include "InviteSession.hxx"
#include "ClientInviteSession.hxx"
#include "ServerInviteSession.hxx"
#include "ClientSubscription.hxx"
#include "ServerSubscription.hxx"
#include "ClientRegistration.hxx"
#include "ServerRegistration.hxx"
#include "ServerPublication.hxx"
#include "ClientPublication.hxx"
#include "ServerOutOfDialogReq.hxx"
#include "ClientOutOfDialogReq.hxx"
#include "resiprocate/Headers.hxx"

namespace resip 
{

class SipStack;
class Profile;
class RedirectManager;
class ClientAuthManager;
class ServerAuthManager;
class ClientRegistrationHandler;
class ServerRegistrationHandler;
class InviteSessionHandler;
class ClientSubscriptionHandler;
class ServerSubscriptionHandler;
class ClientPublicationHandler;
class ServerPublicationHandler;
class OutOfDialogHandler;

class DialogUsageManager
{
   public:
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg,
                      const Data& file,
                      int line)
               : BaseException(msg, file, line)
            {}
            
            virtual const char* name() const {return "DialogUsageManager::Exception";}
      };
      
      DialogUsageManager(SipStack& stack);
      ~DialogUsageManager();
      
      void setProfile(Profile* profile);
      
      void setManager(RedirectManager* redirect);
      void setManager(ClientAuthManager* client);
      void setManager(ServerAuthManager* client);
      
      void setClientRegistrationHandler(ClientRegistrationHandler*);
      void setServerRegistrationHandler(ServerRegistrationHandler*);
      void setInviteSessionHandler(InviteSessionHandler*);
  
      void addHandler(const Data& eventType, ClientSubscriptionHandler*);
      void addHandler(const Data& eventType, ServerSubscriptionHandler*);
  
      void addHandler(const Data& eventType, ClientPublicationHandler*);
      void addHandler(const Data& eventType, ServerPublicationHandler*);
  
      void addHandler(MethodTypes&, OutOfDialogHandler*);
      
      // The message is owned by the underlying datastructure and may go away in
      // the future. If the caller wants to keep it, it should make a copy. The
      // memory will exist at least up until the point where the application
      // calls DialogUsageManager::send(msg);
      SipMessage& makeInviteSession(const Uri& target);
      SipMessage& makeSubscription(const Uri& aor, const Data& eventType);
      SipMessage& makeRefer(const Uri& aor, const H_ReferTo::Type& referTo);
      SipMessage& makePublication(const Uri& aor, const Data& eventType);
      SipMessage& makeRegistration(const Uri& aor);
      SipMessage& makeOutOfDialogRequest(const Uri& aor, const MethodTypes& meth);

      SipMessage& makeInviteSession(DialogId, const Uri& target);
      SipMessage& makeSubscription(DialogId, const Uri& aor, const Data& eventType);
      SipMessage& makeRefer(DialogId, const Uri& aor, const H_ReferTo::Type& referTo);
      SipMessage& makePublication(DialogId, const Uri& aor, const Data& eventType);
      SipMessage& makeRegistration(DialogId, const Uri& aor);
      SipMessage& makeOutOfDialogRequest(DialogId, const Uri& aor, const MethodTypes& meth);
      
      void cancel(DialogIdSet invSessionId);
      DialogSetId send(SipMessage* newClientRequest);

      void process(FdSet& fdset);
      
      DialogIdSet findAllDialogs();
      UsageSet    findAllUsages();
      
      InviteSession::Handle findInviteSession(DialogId id);
      std::list<ClientSubscription::Handle>& findClientSubscriptions(DialogId id);
      ServerSubscription::Handle findServerSubscription(DialogId id);
      ClientRegistration::Handle findClientRegistration(DialogId id);
      ServerRegistration::Handle findServerRegistration(DialogId id);
      ClientPublication::Handle findClientPublication(DialogId id);
      ServerPublication::Handle findServerPublication(DialogId id);
      ClientOutOfDialogReq::Handle findClientOutOfDialog(DialogId id);
      ServerOutOfDialogReq::Handle findServerOutOfDialog(DialogId id);
      
   private:
      // !dlb! interfaces certainly incomplete...
      ClientInviteSession* createClientInviteSession();
      ServerInviteSession* createServerInviteSession(const SipMessage& msg);
      ClientSubscription* createClientSubscription();
      ServerSubscription* createServerSubscription(const SipMessage& msg);
      ClientRegistration* createClientRegistration(const SipMessage& msg);
      ServerRegistration* createServerRegistration(const SipMessage& msg);
      ClientPublication* createClientPublication(const SipMessage& msg);
      ServerPublication* createServerPublication(const SipMessage& msg);
      ClientOutOfDialogReq* createClientOutOfDialog(const SipMessage& msg);
      ServerOutOfDialogReq* createServerOutOfDialog(const SipMessage& msg);

      // delete the usage, remove from usage handle map
      void destroyUsage(BaseUsage* usage);

      typedef HashMap<BaseUsage::Handle::Id, BaseUsage*> UsageHandleMap;
      UsageHandleMap mUsageMap;
	  
      // throws if not found
      BaseUsage* getUsage(const BaseUsage::Handle& handle);

      bool validateRequest(const SipMessage& request);
      bool validateTo(const SipMessage& request);
      bool mergeRequest(const SipMessage& request);
      
      Dialog& findOrCreateDialog(SipMessage* msg);
      Dialog& findDialog(DialogId id);
      DialogSet& findDialogSet(DialogSetId id);
      
      BaseCreator& findCreator(DialogId id);

      void prepareInitialRequest(SipMessage& request);
      
      HashMap<DialogSetId, DialogSet* > mDialogSetMap;

      Profile* mProfile;
      RedirectManager* mRedirectManager;
      ClientAuthManager* mClientAuthManager;
      ServerAuthManager* mServerAuthManager;  
    
      InviteSessionHandler* mInviteSessionHandler;
      ClientRegistrationHandler* mClientRegistrationHandler;
      ServerRegistrationHandler* mServerRegistrationHandler;      

      std::list<ClientSubscriptionHandler*> mClientSubscriptionHandler;
      std::list<ServerSubscriptionHandler*> mServerSubscriptionHandler;      
      std::list<ClientPublicationHandler*> mClientPublicationHandler;
      std::list<ServerPublicationHandler*> mServerPublicationHandler;      
      OutOfDialogHandler* mOutOfDialogHandler;

      SipStack& mStack;
};

}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
