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
      uint64_t mRegExpires;   // in seconds
      uint64_t mLastUpdated;  // in seconds
      Tuple mReceivedFrom;  // source transport, IP address, and port
      Tuple mPublicAddress; // Public IP address closest to the client (from Via headers): note: Tuple::getType == UNKNOWN_TRANPORT if no public address was found
      NameAddrs mSipPath;   // Value of SIP Path header from the request
      Data mInstance;       // From the instance parameter; usually a UUID URI
      uint32_t mRegId;        // From regid parameter of Contact header
      Data mUserAgent;      // From User-Agent header
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
      :mOp(op),mRec(std::move(rec))
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
