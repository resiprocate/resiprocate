#if !defined(RESIP_CLIENTREGISTRATION_HXX)
#define RESIP_CLIENTREGISTRATION_HXX

#include "resip/dum/NonDialogUsage.hxx"
#include "resip/stack/NameAddr.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/dum/NetworkAssociation.hxx"

namespace resip
{

class SipMessage;
class BaseCreator;

//!dcm! -- shutdown/deletion API -- end?
class ClientRegistration: public NonDialogUsage
{
   public:
      //ClientRegistration(DialogUsageManager& dum, DialogSet& dialog,
      //SipMessage& req);
      ClientRegistration(DialogUsageManager& dum, DialogSet& dialog, std::shared_ptr<SipMessage> req);
      ClientRegistration(const ClientRegistration&) = delete;
      ClientRegistration(ClientRegistration&&) = delete;

      ClientRegistration& operator=(const ClientRegistration&) = delete;
      ClientRegistration& operator=(ClientRegistration&&) = delete;

      ClientRegistrationHandle getHandle();

      /** Adds a registration binding, using the registration from the UserProfile */
      void addBinding(const NameAddr& contact);

      /** Adds a registration binding, using the specified registration time */
      void addBinding(const NameAddr& contact, uint32_t registrationTime);

      /** Removes one particular binding */
      void removeBinding(const NameAddr& contact);

      /** Removes all bindings associated with the AOR - even those that may have been
          by a another UA.  Note:  Set's contact header to '*'.  Set flag to true
          to end the usage when complete.  */
      void removeAll(bool stopRegisteringWhenDone=false);

      /** Removes all bindings added by addBinding.  Set flag to true to end the usage 
          when complete */
      void removeMyBindings(bool stopRegisteringWhenDone=false);

      /** Request a manual refresh of the registration.  If 0 then default to using original 
          expires value (to remove use removeXXX() instead) */
      void requestRefresh(uint32_t expires = 0);  
      
      /** kills the usgage, does not unregister, call removeMyBindings to unregister */
      void stopRegistering(); 
      
      /** returns a list of contacts added by addBinding */
      const NameAddrs& myContacts() const noexcept;

      /** returns a list of all contacts for this AOR - may include those added by other UA's */
      const NameAddrs& allContacts() const noexcept;

      /** returns the number of seconds until the registration expires - relative, returns 0 if already expired */
      uint32_t whenExpires() const; 
      
      /** Calls removeMyBindings and ends usage when complete */
      void end() override;

      /** Returns true if a REGISTER request is currently pending and we are waiting for the SIP Response */
      bool isRequestPending() const noexcept { return mState != Registered && mState != RetryAdding && mState != RetryRefreshing; }

      /**
       * Provide asynchronous method access by using command
       */
      void removeMyBindingsCommand(bool stopRegisteringWhenDone=false);
      virtual void endCommand();


      void dispatch(const SipMessage& msg) override;
      void dispatch(const DumTimeout& timer) override;

      EncodeStream& dump(EncodeStream& strm) const override;

      static void tagContact(NameAddr& contact, DialogUsageManager& dum, const std::shared_ptr<UserProfile>& userProfile);

   protected:
      virtual ~ClientRegistration();

   private:
      typedef enum
      {
         Querying,
         Adding,
         Refreshing,
         Registered,
         Removing,
         RetryAdding,    // Waiting to retry an add
         RetryRefreshing, // Waiting to retry a refresh
         None // for queued only
      } State;

      std::shared_ptr<SipMessage> tryModification(ClientRegistration::State state);
      void internalRequestRefresh(uint32_t expires = 0);  // 0 defaults to using original expires value (to remove use removeXXX() instead)
      unsigned int checkProfileRetry(const SipMessage& msg);
      void tagContact(NameAddr& contact) const;
      unsigned long calculateExpiry(const SipMessage& reg200) const;
      bool contactIsMine(const NameAddr& contact) const;
      bool rinstanceIsMine(const Data& rinstance) const;
      bool searchByUri(const Uri& cUri) const;

      friend class DialogSet;
      void flowTerminated();

      //SipMessage& mLastRequest;
      std::shared_ptr<SipMessage> mLastRequest;
      NameAddrs mMyContacts; // Contacts that this UA is requesting 
      NameAddrs mAllContacts; // All the contacts Registrar knows about 
      NameAddrs mLastMyContacts;
      NameAddrs mLastAllContacts;
      unsigned int mTimerSeq; // expected timer seq (all < are stale)

      State mState;
      bool mEnding;
      bool mEndWhenDone;
      bool mUserRefresh;
      uint32_t mRegistrationTime;
      uint64_t mExpires;
      uint64_t mRefreshTime;
      State mQueuedState;
      std::shared_ptr<SipMessage> mQueuedRequest;

      NetworkAssociation mNetworkAssociation;
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
