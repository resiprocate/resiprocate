#if !defined(RESIP_REGISTRATIONHANDLER_HXX)
#define RESIP_REGISTRATIONHANDLER_HXX

#include "resip/dum/Handles.hxx"
#include "rutil/SharedPtr.hxx"
#include "resip/dum/ContactInstanceRecord.hxx"

namespace resip
{

class SipMessage;
class NameAddr;
class MasterProfile;

class ClientRegistrationHandler
{
   public:
      virtual ~ClientRegistrationHandler() { }
      /// Called when registraion succeeds or each time it is sucessfully
      /// refreshed (manual refreshes only). 
      virtual void onSuccess(ClientRegistrationHandle, const SipMessage& response)=0;

      // Called when all of my bindings have been removed
      virtual void onRemoved(ClientRegistrationHandle, const SipMessage& response) = 0;
      
      /// call on Retry-After failure. 
      /// return values: -1 = fail, 0 = retry immediately, N = retry in N seconds
      virtual int onRequestRetry(ClientRegistrationHandle, int retrySeconds, const SipMessage& response)=0;
      
      /// Called if registration fails, usage will be destroyed (unless a 
      /// Registration retry interval is enabled in the Profile)
      virtual void onFailure(ClientRegistrationHandle, const SipMessage& response)=0;

      /// Called when a TCP or TLS flow to the server has terminated.  This can be caused by socket
      /// errors, or missing CRLF keep alives pong responses from the server.
      //  Called only if clientOutbound is enabled on the UserProfile and the first hop server 
      /// supports RFC5626 (outbound).
      /// Default implementation is to immediately re-Register in an attempt to form a new flow.
      virtual void onFlowTerminated(ClientRegistrationHandle);
};

class ServerRegistrationHandler
{
   public:
      virtual ~ServerRegistrationHandler() {}

      /// Called when registration is refreshed
      virtual void onRefresh(ServerRegistrationHandle, const SipMessage& reg)=0;
      
      /// called when one or more specified contacts is removed
      virtual void onRemove(ServerRegistrationHandle, const SipMessage& reg)=0;
      
      /// Called when all the contacts are removed using "Contact: *"
      virtual void onRemoveAll(ServerRegistrationHandle, const SipMessage& reg)=0;
      
      /** Called when one or more contacts are added. This is after 
          authentication has all succeeded */
      virtual void onAdd(ServerRegistrationHandle, const SipMessage& reg)=0;

      /// Called when a client queries for the list of current registrations
      virtual void onQuery(ServerRegistrationHandle, const SipMessage& reg)=0;

      /// When processing a REGISTER request, return the desired expires value when processing the "Expires" header.
      ///@param expires Set this to the desired expiration value for the set of contacts that do not explicitely 
      ///   set the "expires" param.  
      ///@param returnCode If the REGISTER should be rejected, use this return code.  A value of 423 will result in
      ///the Min-Expires header added to the response.
      ///
      virtual void getGlobalExpires(const SipMessage& msg, 
                                    SharedPtr<MasterProfile> masterProfile, 
                                    UInt32 &expires, 
                                    UInt32 &returnCode);

      /// When processing a REGISTER request, return the desired expires value by processing this contact's expires
      /// parameter.  If the expires value is not modified in this function the global expires will be used.
      /// @param expires Set this to the desired expiration value for this contact.
      /// @param returnCode If the REGISTER should be rejected, use this return code.  A value of 423 will result in
      ///   the Min-Expires header added to the response.
      ///
      virtual void getContactExpires(const NameAddr &contact, 
                                     SharedPtr<MasterProfile> masterProfile, 
                                     UInt32 &expires, 
                                     UInt32 &returnCode);

       /** If true, the registration processing will use the async* functions here and will not use the RegistrationPersistenceManager.
        */
      virtual bool asyncProcessing(void) const
      {
         return false;
      } 

      /** Called when a REGISTER is first received to retrieve the current list.  This list is then updated by the
          registration logic in DUM.  When the list is ready, call asyncProvideContacts() on the specified ServerRegistration.
          */
      virtual void asyncGetContacts(ServerRegistrationHandle, const Uri& aor)
      {
      }

      /** Notifies the handler to update the current contact list for the AOR to the specified list that has been updated by
          DUM's registration processing.  This is normally called after the REGISTER has been processed and accepted by the user
          (ServerRegistration::accept())
       */
      virtual void asyncUpdateContacts(ServerRegistrationHandle,
                                       const Uri& aor,
                                       std::auto_ptr<ContactPtrList> modifiedContactList, 
                                       std::auto_ptr<ContactRecordTransactionLog> transactionLog)
      {
      }

      /** Notifies the handler to remove the entries specified in the contacts parameter.
          No further processing is required after receiving this message.
      */
      virtual void asyncRemoveExpired(ServerRegistrationHandle, 
                                      const resip::Uri& aor,
                                      std::auto_ptr<resip::ContactPtrList> contacts)
      {
      }
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
