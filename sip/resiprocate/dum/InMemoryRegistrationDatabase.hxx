#if !defined(RESIP_INMEMORYREGISTRATIONDATABASE_HXX)
#define RESIP_INMEMORYREGISTRATIONDATABASE_HXX

#include <map>

#include "resiprocate/dum/RegistrationPersistenceManager.hxx"

namespace resip
{

/**
  Trivial implementation of a persistence manager. This class keeps
  all registrations in memory, and has no schemes for disk storage
  or replication of any kind. It's good for testing, but probably
  inappropriate for any commercially deployable products.
 */
class InMemoryRegistrationDatabase : public RegistrationPersistenceManager
{
  public:
    InMemoryRegistrationDatabase();
    virtual ~InMemoryRegistrationDatabase();

    virtual void addAor(Uri &aor, contact_list_t contacts = contact_list_t());
    virtual void removeAor(Uri &aor);
    virtual bool aorIsRegistered(Uri &aor);

    virtual void lockRecord(Uri &aor);
    virtual void unlockRecord(Uri &aor);

    virtual update_status_t updateContact(Uri &aor, Uri &contact, time_t expires);
    virtual void removeContact(Uri &aor, Uri &contact);

    virtual contact_list_t getContacts(Uri &aor);

  private:
    typedef std::map<Uri,contact_list_t> database_map_t;
    database_map_t mDatabase;
};
 
}

#endif
