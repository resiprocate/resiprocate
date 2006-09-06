#if !defined(RESIP_CLIENTREGISTRATION_HXX)
#define RESIP_CLIENTREGISTRATION_HXX

#include "resiprocate/dum/NonDialogUsage.hxx"
#include "resiprocate/NameAddr.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/NetworkAssociation.hxx"
#include "resiprocate/dum/Win32ExportDum.hxx"

namespace resip
{

class SipMessage;
class BaseCreator;

//!dcm! -- shutdown/deletion API -- end?
class DUM_API ClientRegistration: public NonDialogUsage
{
   public:
      ClientRegistration(DialogUsageManager& dum, DialogSet& dialog, SipMessage& req);
      ClientRegistrationHandle getHandle();

      void addBinding(const NameAddr& contact);
      void addBinding(const NameAddr& contact, int registrationTime);
      void removeBinding(const NameAddr& contact);
      void removeAll(bool stopRegisteringWhenDone=false);
      void removeMyBindings(bool stopRegisteringWhenDone=false);        // sync.
      void removeMyBindingsAsync(bool stopRegisteringWhenDone=false);   // !polo! async.
      void requestRefresh(int expires = -1);  // default to using original expires value (0 is not allowed - call removeXXX() instead)
      
      //kills the usgage, call removeMyBindings to deregister
      void stopRegistering(); 
      
      const NameAddrs& myContacts();
      const NameAddrs& allContacts();
      int whenExpires() const; // relative in seconds
      
      virtual void end();
      virtual void dispatch(const SipMessage& msg);
      virtual void dispatch(const DumTimeout& timer);
   
      virtual std::ostream& dump(std::ostream& strm) const;

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
         None // for queued only
      } State;

      SipMessage& tryModification(ClientRegistration::State state);

      friend class DialogSet;

      SipMessage& mLastRequest;
      NameAddrs mMyContacts; // Contacts that this UA is requesting 
      NameAddrs mAllContacts; // All the contacts Registrar knows about 
      int mTimerSeq; // expected timer seq (all < are stale)

      State mState;
      bool mEndWhenDone;
      UInt64 mExpires;
      State mQueuedState;
      SipMessage mQueuedRequest;

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
