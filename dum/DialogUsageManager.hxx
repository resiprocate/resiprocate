#if !defined(RESIP_DIALOGUSAGEMANAGER_HXX)
#define RESIP_DIALOGUSAGEMANAGER_HXX

#include <vector>

#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/Headers.hxx"
#include "resiprocate/dum/BaseUsage.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"
#include "resiprocate/dum/ClientOutOfDialogReq.hxx"
#include "resiprocate/dum/ClientPublication.hxx"
#include "resiprocate/dum/ClientRegistration.hxx"
#include "resiprocate/dum/ClientSubscription.hxx"
#include "resiprocate/dum/DialogSet.hxx"
#include "resiprocate/dum/DumTimer.hxx"
#include "resiprocate/dum/InviteSession.hxx"
#include "resiprocate/dum/ServerInviteSession.hxx"
#include "resiprocate/dum/ServerOutOfDialogReq.hxx"
#include "resiprocate/dum/ServerPublication.hxx"
#include "resiprocate/dum/ServerRegistration.hxx"
#include "resiprocate/dum/ServerSubscription.hxx"
#include "resiprocate/dum/UInt64Hash.hxx"

namespace resip 
{

class SipStack;
class FdSet;
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
      Profile* getProfile();
      
      /// There should probably be a default redirect manager
      void setRedirectManager(RedirectManager* redirect);

      /// If there is no ClientAuthManager, when the client receives a 401/407,
      /// pass it up through the normal BaseUsageHandler
      void setClientAuthManager(ClientAuthManager* client);

      /// If there is no ServerAuthManager, the server does not authenticate requests
      void setServerAuthManager(ServerAuthManager* client);

      /// If there is no such handler, calling makeInviteSession will throw and
      /// receiving an INVITE as a UAS will respond with 405 Method Not Allowed
      void setInviteSessionHandler(InviteSessionHandler*);
      
      /// If there is no such handler, calling makeRegistration will throw
      void setClientRegistrationHandler(ClientRegistrationHandler*);

      /// If no such handler, UAS will respond to REGISTER with 405 Method Not Allowed
      void setServerRegistrationHandler(ServerRegistrationHandler*);

      /// If there is no such handler, calling makeSubscription will throw
      void addClientSubscriptionHandler(const Data& eventType, ClientSubscriptionHandler*);

      /// If there is no such handler, calling makePublication will throw
      void addClientPublicationHandler(const Data& eventType, ClientPublicationHandler*);
      
      void addServerSubscriptionHandler(const Data& eventType, ServerSubscriptionHandler*);
      void addServerPublicationHandler(const Data& eventType, ServerPublicationHandler*);
      
      void addOutOfDialogHandler(MethodTypes&, OutOfDialogHandler*);
      
      // The message is owned by the underlying datastructure and may go away in
      // the future. If the caller wants to keep it, it should make a copy. The
      // memory will exist at least up until the point where the application
      // calls DialogUsageManager::send(msg);
      SipMessage& makeInviteSession(const Uri& target, const SdpContents* initialOffer);
      SipMessage& makeSubscription(const Uri& aor, const Data& eventType);
      SipMessage& makeRefer(const Uri& aor, const H_ReferTo::Type& referTo);
      SipMessage& makePublication(const Uri& aor, const Data& eventType);
      SipMessage& makeRegistration(const NameAddr& aor);
      SipMessage& makeOutOfDialogRequest(const Uri& aor, const MethodTypes& meth);

      // all can be done inside of INVITE or SUBSCRIBE only
      SipMessage& makeSubscribe(DialogId, const Uri& aor, const Data& eventType);
      SipMessage& makeRefer(DialogId, const Uri& aor, const H_ReferTo::Type& referTo);
      SipMessage& makePublish(DialogId, const Uri& aor, const Data& eventType); 
      SipMessage& makeOutOfDialogRequest(DialogId, const Uri& aor, const MethodTypes& meth);
      
      void cancel(DialogSetId invSessionId);
      void send(const SipMessage& request); 
      
      void process(FdSet& fdset);
      
      DialogIdSet findAllDialogs();
      UsageSet    findAllUsages();
      
      InviteSession::Handle findInviteSession(DialogId id);
      std::vector<ClientSubscription::Handle> findClientSubscriptions(DialogId id);
      std::vector<ClientSubscription::Handle> findClientSubscriptions(DialogSetId id);
      std::vector<ClientSubscription::Handle> findClientSubscriptions(DialogSetId id, const Data& eventType, const Data& subId);
      ServerSubscription::Handle findServerSubscription(DialogId id);
      ClientRegistration::Handle findClientRegistration(DialogId id);
      ServerRegistration::Handle findServerRegistration(DialogId id);
      ClientPublication::Handle findClientPublication(DialogId id);
      ServerPublication::Handle findServerPublication(DialogId id);
      std::vector<ClientOutOfDialogReq::Handle> findClientOutOfDialog(DialogId id);
      ServerOutOfDialogReq::Handle findServerOutOfDialog(DialogId id);
      
   private:
      friend class Dialog;
      friend class ClientRegistration;
      friend class ClientInviteSession;
      friend class ServerInviteSession;
      friend class BaseUsage::Handle;

      SipMessage& makeNewSession(BaseCreator* creator);
      void addTimer(DumTimer::Type type, unsigned long duration, int cseq, int rseq=-1);

      ClientInviteSession* makeClientInviteSession(Dialog& dialog,const SipMessage& msg);
      ClientSubscription* makeClientSubscription(Dialog& dialog,const SipMessage& msg);
      ClientRegistration* makeClientRegistration(Dialog& dialog,const SipMessage& msg);
      ClientPublication* makeClientPublication(Dialog& dialog, const SipMessage& msg);
      ClientOutOfDialogReq* makeClientOutOfDialogReq(Dialog& dialog,const SipMessage& msg);
      
      ServerInviteSession* makeServerInviteSession(Dialog& dialog,const SipMessage& msg);
      ServerSubscription* makeServerSubscription(Dialog& dialog,const SipMessage& msg);
      ServerRegistration* makeServerRegistration(Dialog& dialog,const SipMessage& msg);
      ServerPublication* makeServerPublication(Dialog& dialog,const SipMessage& msg);
      ServerOutOfDialogReq* makeServerOutOfDialog(Dialog& dialog,const SipMessage& msg);

      // delete the usage, remove from usage handle map
      void destroyUsage(BaseUsage* usage);

      typedef HashMap<BaseUsage::Handle::Id, BaseUsage*> UsageHandleMap;
      UsageHandleMap mUsageMap;
	  
      bool isValid(const BaseUsage::Handle& handle);
      // throws if not found
      BaseUsage* getUsage(const BaseUsage::Handle& handle);
      
      Dialog& findOrCreateDialog(const SipMessage* msg);
      Dialog& findDialog(const DialogId& id);
      DialogSet& findDialogSet(const DialogSetId& id);
      
      // return 0, if no matching BaseCreator
      BaseCreator* findCreator(const DialogId& id);

      void prepareInitialRequest(SipMessage& request);
      void processRequest(const SipMessage& request);
      void processResponse(const SipMessage& response);
      bool validateRequest(const SipMessage& request);
      bool validateTo(const SipMessage& request);
      bool mergeRequest(const SipMessage& request);

      typedef HashMap<DialogSetId, DialogSet*> DialogSetMap;
      DialogSetMap mDialogSetMap;

      Profile* mProfile;
      RedirectManager* mRedirectManager;
      ClientAuthManager* mClientAuthManager;
      ServerAuthManager* mServerAuthManager;  
    
      InviteSessionHandler* mInviteSessionHandler;
      ClientRegistrationHandler* mClientRegistrationHandler;
      ServerRegistrationHandler* mServerRegistrationHandler;      

      std::vector<ClientSubscriptionHandler*> mClientSubscriptionHandler;
      std::vector<ServerSubscriptionHandler*> mServerSubscriptionHandler;
      std::vector<ClientPublicationHandler*> mClientPublicationHandler;
      std::vector<ServerPublicationHandler*> mServerPublicationHandler;
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
