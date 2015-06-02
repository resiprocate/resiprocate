#if !defined(RESIP_CLIENTDIALOG_HXX)
#define RESIP_CLIENTDIALOG_HXX

#include <iosfwd>
#include <vector>
#include <list>
#include <map>

#include "resip/dum/DialogId.hxx"
#include "resip/dum/Handles.hxx"
#include "resip/stack/MethodTypes.hxx"
#include "resip/stack/NameAddr.hxx"
#include "resip/stack/CallId.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/dum/NetworkAssociation.hxx"
#include "resip/dum/DialogUsageManager.hxx"

namespace resip
{
class BaseUsage;
class SipMessage;
class DialogUsageManager;
class DialogSet;
class AppDialog;

//!dcm! -- kill typedef std::list<DialogId> DialogIdSet;

class Dialog 
{
   public:
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, int line);
            virtual const char* name() const {return "Dialog::Exception";}
      };
         
      // different behavior from request vs. response
      // (request creates to tag)
      Dialog(DialogUsageManager& dum, const SipMessage& msg, DialogSet& ds);

      const DialogId& getId() const;
      const NameAddr& getLocalNameAddr() const;
      const NameAddr& getLocalContact() const;
      const NameAddr& getRemoteNameAddr() const;
      const NameAddr& getRemoteTarget() const;
      const NameAddrs& getRouteSet() const;
      
      // pass dialog sip messages through dialog so we can cache the requests on
      // the way out to be able to respond to digest authenticate requests
      void send(SharedPtr<SipMessage> msg);
      //void send(SipMessage& msg);
      
      void makeRequest(SipMessage& request, MethodTypes method, bool incrementCSeq = true);
      void makeResponse(SipMessage& response, const SipMessage& request, int responseCode);
      void setRequestNextCSeq(SipMessage& request);

      //void setLocalContact(const NameAddr& localContact);
      //void setRemoteTarget(const NameAddr& remoteTarget);
      
      std::vector<ClientSubscriptionHandle> getClientSubscriptions();
      std::vector<ClientSubscriptionHandle> findClientSubscriptions(const Data& event);

      std::vector<ServerSubscriptionHandle> getServerSubscriptions();
      std::vector<ServerSubscriptionHandle> findServerSubscriptions(const Data& event);

      //returns an invalid handle if there is no session
      InviteSessionHandle getInviteSession();
      
      void end();
      void dispatch(const SipMessage& msg);
      void processNotify(const SipMessage& notify);
      
      //will end this dialog(if it makes sense)
      void redirected(const SipMessage& msg);      

      void onForkAccepted();      
      void cancel();

      bool isDestroying() { return mDestroying; };

   private:
      virtual ~Dialog();
      friend class DialogUsage;
      friend class DialogSet;
      friend class DialogUsageManager;
      friend class DestroyUsage;
      
      friend class ClientSubscription;
      friend class InviteSession;
      friend class ClientInviteSession;      
      friend class ServerInviteSession;      
      friend class ServerSubscription;
      friend class ClientRegistration;
      friend class ServerRegistration;
      friend class ClientPublication;
      friend class ServerPublication;
      friend class ClientOutOfDialogReq;
      friend class ServerOutOfDialogReq;

      friend class AppDialog;
      void possiblyDie();

      ClientSubscription* findMatchingClientSub(const SipMessage& msg);
      ServerSubscription* findMatchingServerSub(const SipMessage& msg);

      void addUsage(BaseUsage* usage);
      ClientInviteSession* makeClientInviteSession(const SipMessage& msg);
      ClientSubscription* makeClientSubscription(const SipMessage& msg);
      
      ServerInviteSession*  makeServerInviteSession(const SipMessage& msg);
      ServerSubscription* makeServerSubscription(const SipMessage& msg);

      //matches using tid of response
      bool matches(const SipMessage& msg);      
      void handleTargetRefresh(const SipMessage& msg);

      void flowTerminated();

      DialogUsageManager& mDum;
      DialogSet& mDialogSet;
      DialogId mId;  

      std::list<ClientSubscription*> mClientSubscriptions;
      std::list<ServerSubscription*> mServerSubscriptions;
      InviteSession* mInviteSession;

      NetworkAssociation mNetworkAssociation;

      //invariants
      typedef enum // need to add
      {
         Invitation,   // INVITE dialog
         Subscription, // Created by a SUBSCRIBE / NOTIFY / REFER
         Fake          // Not really a dialog (e.g. created by a REGISTER)
      } DialogType;
      
      DialogType mType; // !jf! is this necessary?
      NameAddrs mRouteSet;
      
      //variants
      NameAddr mLocalContact;
      unsigned int mLocalCSeq;
      unsigned int mRemoteCSeq;
      NameAddr mRemoteTarget;
      NameAddr mLocalNameAddr;
      NameAddr mRemoteNameAddr;
      CallID mCallId;

      // store until we get a response (non-401/407)
      // !jf! this shouldn't be necessary
      // !dcm! -- no longer used for subscriptions, INVITE will take more thought/work
      typedef std::map<int, SharedPtr<SipMessage> > RequestMap;
      RequestMap mRequests;

      AppDialog* mAppDialog;
      
      bool mDestroying;
      bool mReUseDialogSet;

      friend EncodeStream& operator<<(EncodeStream& strm, const Dialog& dialog);
};

EncodeStream& operator<<(EncodeStream& strm, const Dialog& dialog);

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
