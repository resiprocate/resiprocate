#if !defined(RESIP_CLIENTDIALOGSET_HXX)
#define RESIP_CLIENTDIALOGSET_HXX

#include <map>
#include <list>

#include "resip/dum/DialogId.hxx"
#include "resip/dum/DialogSetId.hxx"
#include "resip/dum/MergedRequestKey.hxx"
#include "resip/dum/Handles.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/SharedPtr.hxx"

namespace resip
{

class BaseCreator;
class Dialog;
class DialogUsageManager;
class AppDialogSet;
class ClientOutOfDialogReq;
class UserProfile;

class DialogSet
{
   public:
      DialogSet(BaseCreator* creator, DialogUsageManager& dum);
      DialogSet(const SipMessage& request, DialogUsageManager& dum);
      virtual ~DialogSet();
      
      DialogSetId getId() const;
      void addDialog(Dialog*);
      bool empty() const;
      BaseCreator* getCreator();

      SharedPtr<UserProfile> getUserProfile() const;
      void setUserProfile(SharedPtr<UserProfile> userProfile);

      void end();
      void dispatch(const SipMessage& msg);
      
      ClientRegistrationHandle getClientRegistration();
      ServerRegistrationHandle getServerRegistration();
      ClientPublicationHandle getClientPublication();
      ClientOutOfDialogReqHandle getClientOutOfDialog();
      ServerOutOfDialogReqHandle getServerOutOfDialog();

      bool isDestroying() { return mState == Destroying; };

   private:
      friend class Dialog;
      friend class DialogUsage;
      friend class ClientInviteSession;
      friend class NonDialogUsage;
      friend class DialogUsageManager;      
      friend class ClientRegistration;
      friend class ServerRegistration;
      friend class ClientOutOfDialogReq;
      friend class ServerOutOfDialogReq;
      friend class ClientPublication;
      friend class RedirectManager;
      friend class ClientPagerMessage;
      friend class ServerPagerMessage;
      
      typedef enum
      {
         Initial,  // No session setup yet
         WaitingToEnd,
         ReceivedProvisional,
         Established,
         Terminating,
         Cancelling,  // only used when cancelling and no dialogs exist
         Destroying
      } State;

      void possiblyDie();

      void onForkAccepted();
      bool handledByAuthOrRedirect(const SipMessage& msg);

      /// Abandon this dialog set, but keep the AppDialogSet around
      void appDissociate()
      {
         resip_assert(mAppDialogSet);
         mAppDialogSet = 0;
      }
      friend class AppDialogSet;

      Dialog* findDialog(const SipMessage& msg);
      Dialog* findDialog(const DialogId id);

      ClientOutOfDialogReq* findMatchingClientOutOfDialogReq(const SipMessage& msg);

      ClientRegistration* makeClientRegistration(const SipMessage& msg);
      ClientPublication* makeClientPublication( const SipMessage& msg);
      ClientOutOfDialogReq* makeClientOutOfDialogReq(const SipMessage& msg);

      ServerRegistration* makeServerRegistration(const SipMessage& msg);
      ServerOutOfDialogReq* makeServerOutOfDialog(const SipMessage& msg);
      
      ServerPagerMessage* makeServerPagerMessage(const SipMessage& request);      

      void dispatchToAllDialogs(const SipMessage& msg);      

      void flowTerminated(const Tuple& flow);

      MergedRequestKey mMergeKey;
      Data mCancelKey;
      typedef std::map<DialogId,Dialog*> DialogMap;
      DialogMap mDialogs;
      BaseCreator* mCreator;
      DialogSetId mId;
      DialogUsageManager& mDum;
      AppDialogSet* mAppDialogSet;
      State mState;
      ClientRegistration* mClientRegistration;
      ServerRegistration* mServerRegistration;
      ClientPublication* mClientPublication;
      std::list<ClientOutOfDialogReq*> mClientOutOfDialogRequests;
      ServerOutOfDialogReq* mServerOutOfDialogRequest;

      ClientPagerMessage* mClientPagerMessage;
      ServerPagerMessage* mServerPagerMessage;
      SharedPtr<UserProfile> mUserProfile;

      friend EncodeStream& operator<<(EncodeStream& strm, const DialogSet& ds);
};

EncodeStream& 
operator<<(EncodeStream& strm, const DialogSet& ds);

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
