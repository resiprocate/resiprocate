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
      ClientRegistration(DialogUsageManager& dum, DialogSet& dialog, SharedPtr<SipMessage> req);
      ClientRegistrationHandle getHandle();

      /** Adds a registration binding, using the registration from the UserProfile */
      void addBinding(const NameAddr& contact);

      /** Adds a registration binding, using the specified registration time */
      void addBinding(const NameAddr& contact, UInt32 registrationTime);

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
      void requestRefresh(UInt32 expires = 0);  
      
      /** kills the usgage, does not unregister, call removeMyBindings to unregister */
      void stopRegistering(); 
      
      /** returns a list of contacts added by addBinding */
      const NameAddrs& myContacts();

      /** returns a list of all contacts for this AOR - may include those added by other UA's */
      const NameAddrs& allContacts();

      /** returns the number of seconds until the registration expires - relative, returns 0 if already expired */
      UInt32 whenExpires() const; 
      
      /** Calls removeMyBindings and ends usage when complete */
      virtual void end();

      /**
       * Provide asynchronous method access by using command
       */
      void removeMyBindingsCommand(bool stopRegisteringWhenDone=false);
      virtual void endCommand();


      virtual void dispatch(const SipMessage& msg);
      virtual void dispatch(const DumTimeout& timer);
   
      virtual EncodeStream& dump(EncodeStream& strm) const;

      static void tagContact(NameAddr& contact, DialogUsageManager& dum, SharedPtr<UserProfile>& userProfile);

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

      SharedPtr<SipMessage> tryModification(ClientRegistration::State state);
      void internalRequestRefresh(UInt32 expires = 0);  // 0 defaults to using original expires value (to remove use removeXXX() instead)
      unsigned int checkProfileRetry(const SipMessage& msg);
      void tagContact(NameAddr& contact) const;
      unsigned long calculateExpiry(const SipMessage& reg200) const;
      bool contactIsMine(const NameAddr& contact) const;
      bool rinstanceIsMine(const Data& rinstance) const;
      bool searchByUri(const Uri& cUri) const;

      friend class DialogSet;
      void flowTerminated();

      //SipMessage& mLastRequest;
      SharedPtr<SipMessage> mLastRequest;
      NameAddrs mMyContacts; // Contacts that this UA is requesting 
      NameAddrs mAllContacts; // All the contacts Registrar knows about 
      unsigned int mTimerSeq; // expected timer seq (all < are stale)

      State mState;
      bool mEnding;
      bool mEndWhenDone;
      bool mUserRefresh;
      UInt32 mRegistrationTime;
      UInt64 mExpires;
      UInt64 mRefreshTime;
      State mQueuedState;
      SharedPtr<SipMessage> mQueuedRequest;

      NetworkAssociation mNetworkAssociation;
      
      // disabled
      ClientRegistration(const ClientRegistration&);
      ClientRegistration& operator=(const ClientRegistration&);
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
