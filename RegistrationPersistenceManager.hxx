#if !defined(RESIP_REGISTRATIONPERSISTENCEMANAGER_HXX)
#define RESIP_REGISTRATIONPERSISTENCEMANAGER_HXX

#include <list>
#include "resiprocate/Uri.hxx"

namespace resip
{

class RegistrationPersistenceManager
{
  public:
    typedef std::pair<Uri,time_t> contact_t;
    typedef std::list<contact_t> contact_list_t;

    typedef enum
    {
      CONTACT_CREATED,
      CONTACT_UPDATED
    } update_status_t;

    RegistrationPersistenceManager() {}
    virtual ~RegistrationPersistenceManager() {}

    virtual void addAor(Uri &aor, contact_list_t contacts = contact_list_t()) = 0;
    virtual void removeAor(Uri &aor) = 0;
    virtual bool aorIsRegistered(Uri &aor) = 0;

    virtual void lockRecord(Uri &aor) = 0;
    virtual void unlockRecord(Uri &aor) = 0;

    /**
      @param expires Absolute time of expiration, measured in seconds
                     since midnight January 1st, 1970.
     */
    virtual update_status_t updateContact(Uri &aor, Uri &contact, time_t expires) = 0;

    virtual void removeContact(Uri &aor, Uri &contact) = 0;

    virtual contact_list_t getContacts(Uri &aor) = 0;
  private:
};

}

#endif
