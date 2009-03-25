#if !defined(resip_ContactInstanceRecord_hxx)
#define resip_ContactInstanceRecord_hxx

#include <vector>
#include <list>
#include <deque>

#include "resip/stack/NameAddr.hxx"
#include "rutil/Data.hxx"
#include "resip/stack/Tuple.hxx"
#include "rutil/SharedPtr.hxx"

namespace resip
{
/** A single contact record, bound to an Aor during registration.
*/
class ContactInstanceRecord 
{
   public:
      ContactInstanceRecord();
      static ContactInstanceRecord makeRemoveDelta(const NameAddr& contact);
      static ContactInstanceRecord makeUpdateDelta(const NameAddr& contact, 
                                                   UInt64 expires,  // absolute time in secs
                                                   const SipMessage& msg);
      
      NameAddr mContact;    // can contain callee caps and q-values
      UInt64 mRegExpires;   // in seconds
      UInt64 mLastUpdated;  // in seconds
      Tuple mReceivedFrom;  // source transport, IP address, and port 
      NameAddrs mSipPath;   // Value of SIP Path header from the request
      Data mInstance;       // From the instance parameter; usually a UUID URI
      UInt32 mRegId;        // From regid parameter of Contact header
      Data mServerSessionId;// if there is no SIP Path header, the connection/session identifier 
      // Uri gruu;  (GRUU is currently derived)
      void      *mUserInfo;       //!< can be used to map user record information (database record id for faster updates?)
      
      bool operator==(const ContactInstanceRecord& rhs) const;
};

typedef std::list<ContactInstanceRecord> ContactList;

/** Used to reduce copying ContactInstanceRecord objects when processing registration.
*/
typedef std::list<resip::SharedPtr<ContactInstanceRecord> > ContactPtrList;
	
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

   ContactRecordTransaction(Operation op, resip::SharedPtr<ContactInstanceRecord> rec)
      :mOp(op),mRec(rec)
      {}

   Operation mOp;  //!< the operation that was performed in this transaction.
   /** For create & update: the newly modified record; for remove: the removed record; for removeAll: 0.
   */
   resip::SharedPtr<ContactInstanceRecord> mRec;  
};

/** Contains a collection of database operations that were performed during REGISTER processing.
*/
typedef std::deque<resip::SharedPtr<ContactRecordTransaction> > ContactRecordTransactionLog;

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
