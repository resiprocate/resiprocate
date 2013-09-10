#if !defined(RESIP_SERVERREGISTRATION_HXX)
#define RESIP_SERVERREGISTRATION_HXX

#include "resip/dum/NonDialogUsage.hxx"
#include "resip/dum/RegistrationPersistenceManager.hxx"
#include "resip/stack/SipMessage.hxx"

namespace resip
{

class ServerRegistration: public NonDialogUsage 
{
   public:
      ServerRegistrationHandle getHandle();

      /** Accept a SIP registration with a specific response.  Any contacts in this message will be deleted and replaced with the list created during REGISTER processing.

         !Warning! After calling this function from a ServerRegistrationHandle, do not access the handle as this function
         may delete this object. Use ServerRegistrationHandle::isValidHandle() to test if it's deleted.
      */
      void accept(SipMessage& ok);

      /** Accept a SIP registration.

        !Warning! After calling this function from a ServerRegistrationHandle, do not access the handle as this function
        may delete this object. Use ServerRegistrationHandle::isValidHandle() to test if it's deleted.
     */
      void accept(int statusCode = 200);

      /** Reject a SIP registration.  
  
        !Warning! After calling this function from a ServerRegistrationHandle, do not access the handle as this function
        may delete this object. Use ServerRegistrationHandle::isValidHandle() to test if it's deleted.
      */
      void reject(int statusCode);

      virtual void end();
      virtual void dispatch(const SipMessage& msg);
      virtual void dispatch(const DumTimeout& timer);

      virtual EncodeStream& dump(EncodeStream& strm) const;

      /** Used when useAsyncProcessing() is true.  Provide the current set of contacts for this registration for
        * processing.  This is required during initial registration processing to provide a local copy of the registered contacts, which
          are then updated by the registration processing.  At the end of the registration processing this function must provide a
          final list of contacts after any user-defined manipulation.

          !CAUTION! This function must be called from the DUM thread.
        */
      bool asyncProvideContacts(std::auto_ptr<resip::ContactPtrList> contacts);

      resip::SharedPtr<ContactList> getOriginalContacts() { return mOriginalContacts; }  // WARNING - use this only if async mode is not used
      const ContactList& getRequestContacts() { return mRequestContacts; }

   protected:
      virtual ~ServerRegistration();
   private:
      friend class DialogSet;
      ServerRegistration(DialogUsageManager& dum, DialogSet& dialogSet, const SipMessage& request);

      bool tryFlow(ContactInstanceRecord& rec,
                   const resip::SipMessage& msg);
      bool flowTokenNeededForTls(const ContactInstanceRecord &rec) const;
      bool flowTokenNeededForSigcomp(const ContactInstanceRecord &rec) const;
      bool testFlowRequirements(ContactInstanceRecord &rec,
                                const resip::SipMessage& msg,
                                bool hasFlow) const;

      SipMessage mRequest;
      Uri mAor;
      resip::SharedPtr<ContactList> mOriginalContacts;  // Used only for non-async processing
      ContactList mRequestContacts;  // Contains the Contacts from the REGISTER request
      bool mDidOutbound;

      // disabled
      ServerRegistration(const ServerRegistration&);
      ServerRegistration& operator=(const ServerRegistration&);

      /** States are used to keep track of asynchronous requests made to the database.
         Typical activity & state progression for a successful registration (onQuery & reject() are slightly different):

         DUM                  ServerRegistration                RegistrationHandler
         _____________________________________________________________________________________________________
         
      1) dispatch()/asyncStateNone--->
      2)                              asyncGetContacts()/asyncStateWaitingForInitialContactList--->
         ======================================================================================================
      3)                              <---asyncProvideContacts()/asyncStateProcessingRegistration
      4)                      processRegistration()
      5)                              (onRefresh()|onRemove()|onAdd()|onRemoveAll()|onQuery)/asyncStateWaitingForAcceptReject--->
         ======================================================================================================
      6)                              <---accept()
      7)                              asyncUpdateContacts/asyncStateAcceptedWaitingForFinalContactList--->
         ======================================================================================================
      8)                              <---asyncProvideContacts()/asyncStateProvidedFinalContacts
      9)                      asyncProcessFinalContacts()/asyncStateNone
      10) <---send(200Ok)
         ______________________________________________________________________________________________________

         * onQuery processing is similar, except it immediately sends the 200Ok after handling the accept().

         * ServerRegistration::reject() does not require roll-back and the error is immediately sent back to DUM.

       * It is possible to call ServerRegistration::reject() after calling accept() (DB failure, app failure, etc)

      */
      typedef enum AsyncState
      {
         asyncStateNone,
         asyncStateWaitingForInitialContactList, //!< Sent an asynchronous request to get the current contact list from the DB.
         asyncStateProcessingRegistration, //!< received the initial contact list, processing the current REGISTER request and updating the contact list.
         asyncStateWaitingForAcceptReject, //!< RegistrationHandler called, waiting for accept() or reject() from user.
         asyncStateAcceptedWaitingForFinalContactList, //!< asyncUpdateContacts() has been called, waiting for final list.
         asyncStateProvidedFinalContacts, //!< After receiving the final contact list; process the accepted register and send response.
         asyncStateQueryOnly //!< The REGISTER is a query, so just send back the current list in accept().
      } AsyncState;

      AsyncState mAsyncState;

      /** Look at the contacts provided by the incoming REGISTER.
      */
      void processRegistration(const SipMessage& msg);

      void asyncProcessFinalOkMsg(SipMessage &msg, ContactPtrList &contacts);

      /** Add contacs to msg, adding expires param.  Also removes expired entries from contacts and calls the
        * appropriate DB functions to remove them.
      */
      void processFinalOkMsg(SipMessage &msg, ContactList &contacts);

      /** After the user calls accept(), ServerRegistration will apply the local contact list and wait for a final
        * contact list.  Once the final list is received via asyncProvideContacts(), this function finishes the REGISTER
        * processing.
        */
      void asyncProcessFinalContacts(std::auto_ptr<resip::ContactPtrList> contacts);

      /** Local datastore used to aggregate all changes to the current contact list when using the asynchronous logic.
      */
      class AsyncLocalStore
      {
         public:

            AsyncLocalStore(std::auto_ptr<ContactPtrList> originalContacts)
            {
               create(originalContacts);
            }

            ~AsyncLocalStore(void)
            {
               destroy();
            }

            /** Setup this object in preparation for updating the records.  Updates occur when processing a REGISTER
                message.
                */
            void create(std::auto_ptr<ContactPtrList> originalContacts);

            void destroy(void);

            void removeContact(const ContactInstanceRecord &rec);
            /** All original contacts are to be removed, move them all to the removed list
            */
            void removeAllContacts(void);

            /** Could be an addition or refresh.
            */
            RegistrationPersistenceManager::update_status_t
            updateContact(const ContactInstanceRecord &rec);

            /** Remove the transacation log and updated contact list.  This object should be considered destroyed and
                not used after releasing.
               */
            void releaseLog(std::auto_ptr<ContactRecordTransactionLog> &log, std::auto_ptr<ContactPtrList> &modifiedContacts)
            {
               log = mLog;
               modifiedContacts = mModifiedContacts;
            }

            unsigned int numContacts() { if(mModifiedContacts.get()) return (unsigned int)mModifiedContacts->size(); return 0; }
         private:
            std::auto_ptr<ContactRecordTransactionLog> mLog;
            std::auto_ptr<ContactPtrList> mModifiedContacts;
      };

      resip::SharedPtr<AsyncLocalStore> mAsyncLocalStore;

      /** Message stored during accept() call when waiting for final contact list from database.
        * Is eventually used as the 200Ok sent back to DUM when all is finished.
        */
      resip::SharedPtr<SipMessage> mAsyncOkMsg;
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
