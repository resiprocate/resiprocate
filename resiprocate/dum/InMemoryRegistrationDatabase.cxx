#include "resiprocate/dum/InMemoryRegistrationDatabase.hxx"

using namespace resip;

InMemoryRegistrationDatabase::InMemoryRegistrationDatabase()
{
}

InMemoryRegistrationDatabase::~InMemoryRegistrationDatabase()
{
}

void 
InMemoryRegistrationDatabase::addAor(Uri &aor,
    RegistrationPersistenceManager::contact_list_t contacts)
{
  mDatabase[aor] = contacts;
}

void 
InMemoryRegistrationDatabase::removeAor(Uri &aor)
{
  database_map_t::iterator i;
  i = mDatabase.find(aor);
  if (i != mDatabase.end())
  {
    mDatabase.erase(i);
  }
}

bool 
InMemoryRegistrationDatabase::aorIsRegistered(Uri &aor)
{
  database_map_t::iterator i;
  i = mDatabase.find(aor);
  if (i == mDatabase.end())
  {
    return false;
  }
  return true;
}

void
InMemoryRegistrationDatabase::lockRecord(Uri &aor)
{
  // Currently doesn't do any locking.
}

void
InMemoryRegistrationDatabase::unlockRecord(Uri &aor)
{
  // Currently doesn't do any locking.
}

RegistrationPersistenceManager::update_status_t 
InMemoryRegistrationDatabase::updateContact(Uri &aor, Uri &contact, time_t expires)
{
  database_map_t::iterator i;
  i = mDatabase.find(aor);
  if (i == mDatabase.end())
  {
    addAor(aor);
    i = mDatabase.find(aor);
  }

  assert(i != mDatabase.end());

  contact_list_t::iterator j;

  // See if the contact is already present. We use URI matching rules here.
  for (j = i->second.begin(); j != i->second.end(); j++)
  {
    if ((*j).first == contact)
    {
      (*j).first = contact;
      (*j).second = expires;
      return CONTACT_UPDATED;
    }
  }

  // This is a new contact, so we add it to the list.
  i->second.push_back(std::make_pair<Uri,time_t>(contact,expires));
  return CONTACT_CREATED;
}

void 
InMemoryRegistrationDatabase::removeContact(Uri &aor, Uri &contact)
{
  database_map_t::iterator i;
  i = mDatabase.find(aor);
  if (i == mDatabase.end())
  {
    return;
  }

  contact_list_t::iterator j;

  // See if the contact is present. We use URI matching rules here.
  for (j = i->second.begin(); j != i->second.end(); j++)
  {
    if ((*j).first == contact)
    {
      i->second.erase(j);
      if (i->second.empty())
      {
        removeAor(aor);
      }
      return;
    }
  }
}

RegistrationPersistenceManager::contact_list_t
InMemoryRegistrationDatabase::getContacts(Uri &aor)
{
  database_map_t::iterator i;
  i = mDatabase.find(aor);
  if (i == mDatabase.end())
  {
    return contact_list_t();
  }
  return i->second;
}
