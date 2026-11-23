#if !defined(resip_ContactInstanceRecord_hxx)
#define resip_ContactInstanceRecord_hxx

#include <vector>
#include <list>
#include <deque>

#include "resip/stack/NameAddr.hxx"
#include "rutil/Data.hxx"
#include "resip/stack/Tuple.hxx"

#include <memory>
#include <utility>

namespace resip
{

class XMLCursor;

static const uint64_t NeverExpire = 0xFFFFFFFFFFFFFFFFULL;

/** A single contact record, bound to an Aor during registration.
*/
class ContactInstanceRecord 
{
   public:
      ContactInstanceRecord();
      ContactInstanceRecord(const ContactInstanceRecord& rhs);
      ContactInstanceRecord& operator=(const ContactInstanceRecord& rhs);
      virtual ~ContactInstanceRecord();

      static ContactInstanceRecord makeRemoveDelta(const NameAddr& contact);
      static ContactInstanceRecord makeUpdateDelta(const NameAddr& contact, 
                                                   uint64_t expires,  // absolute time in secs
                                                   const SipMessage& msg);

      // Stream ContactInstanceRecord in XML format
      void stream(std::iostream& ss) const;

      // Deserialize off xml tree
      bool deserialize(resip::XMLCursor& xml, uint64_t now = 0);
      /* @returns true if successfully deserialized
       */
      
      NameAddr mContact;    // can contain callee caps and q-values
      uint64_t mRegExpires; // in seconds
      uint64_t mLastUpdated;// in seconds
      Tuple mReceivedFrom;  // source transport, IP address, and port
      Tuple mPublicAddress; // Public IP address closest to the client (from Via headers): note: Tuple::getType == UNKNOWN_TRANPORT if no public address was found
      NameAddrs mSipPath;   // Value of SIP Path header from the request
      Data mInstance;       // From the instance parameter; usually a UUID URI
      uint32_t mRegId;      // From regid parameter of Contact header
      Data mUserAgent;      // From User-Agent header
      Data mCallId;         // Call-ID of the REGISTER that created or last refreshed this binding; empty if unknown
      uint32_t mCSeq;       // CSeq sequence number of that REGISTER; only meaningful when mCallId is not empty
      bool mSyncContact;    // This contact came from registration sync process, instead of direct SIP registration
      bool mUseFlowRouting; // Set to true when routing to this contact should use flow routing 
                            // Note:  There is no need to regsync this field, since such records will also have 
                            //        onlyUseExistingConnection set, and such contacts are not replicated.
      // Data mServerSessionId;// if there is no SIP Path header, the connection/session identifier 
      // Uri gruu;  (GRUU is currently derived)
      void      *mUserInfo;       //!< can be used to map user record information (database record id for faster updates?)
      Data* mUserData;      // Optional user/application specific string
      
      bool operator==(const ContactInstanceRecord& rhs) const;

};

typedef std::list<ContactInstanceRecord> ContactList;

/** Used to reduce copying ContactInstanceRecord objects when processing registration.
*/
typedef std::list<std::shared_ptr<ContactInstanceRecord>> ContactPtrList;
	
/** Records a log of the database transacations that were performed when processing a local registration using the
    ServerRegistration::AsyncLocalStore.
*/
class ContactRecordTransaction
{
   public:

   typedef enum Operation
   {
      none,
      update,
      create,
      remove,
      removeAll
   } Operation;

   ContactRecordTransaction()
      :mOp(none)
      {}

   ContactRecordTransaction(Operation op, std::shared_ptr<ContactInstanceRecord> rec)
      :mOp(op),mRec(rec)
      {}

   Operation mOp;  //!< the operation that was performed in this transaction.
   /** For create & update: the newly modified record; for remove: the removed record; for removeAll: 0.
   */
   std::shared_ptr<ContactInstanceRecord> mRec;
};

/** Contains a collection of database operations that were performed during REGISTER processing.
*/
typedef std::deque<std::shared_ptr<ContactRecordTransaction>> ContactRecordTransactionLog;

class RegistrationBinding 
{
   public:
      Data mAor;                      // canonical URI for this AOR and its aliases
      ContactList mContacts;
      std::vector<Uri> mAliases;     
};

struct RegistrationBindingDelta
{
      Data mAor;
      std::vector<ContactInstanceRecord> mInserts;
      std::vector<ContactInstanceRecord> mUpdates;
      std::vector<ContactInstanceRecord> mRemoves;
};
}
#endif


/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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