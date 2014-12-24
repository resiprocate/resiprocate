#include <ctime>

#include "resip/dum/InMemoryRegistrationDatabase.hxx"
#include "rutil/Timer.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

InMemoryRegistrationDatabase::InMemoryRegistrationDatabase(bool checkExpiry) :
   mCheckExpiry(checkExpiry)
{
}

InMemoryRegistrationDatabase::~InMemoryRegistrationDatabase()
{
   for( database_map_t::const_iterator it = mDatabase.begin();
        it != mDatabase.end(); it++)
   {
      delete it->second;
   }
   mDatabase.clear();
}

void 
InMemoryRegistrationDatabase::addAor(const Uri& aor,
                                       const ContactList& contacts)
{
  Lock g(mDatabaseMutex);
  mDatabase[aor] = new ContactList(contacts);
}

void 
InMemoryRegistrationDatabase::removeAor(const Uri& aor)
{
  database_map_t::iterator i;

  Lock g(mDatabaseMutex);
  i = mDatabase.find(aor);
  //DebugLog (<< "Removing registration bindings " << aor);
  if (i != mDatabase.end())
  {
     if (i->second)
     {
        DebugLog (<< "Removed " << i->second->size() << " entries");
        delete i->second;
        // Setting this to 0 causes it to be removed when we unlock the AOR.
        i->second = 0;
     }
  }
}

void
InMemoryRegistrationDatabase::getAors(InMemoryRegistrationDatabase::UriList& container)
{
   container.clear();
   Lock g(mDatabaseMutex);
   for( database_map_t::const_iterator it = mDatabase.begin();
        it != mDatabase.end(); it++)
   {
      container.push_back(it->first);
   }
}

bool 
InMemoryRegistrationDatabase::aorIsRegistered(const Uri& aor)
{
  Lock g(mDatabaseMutex);
  database_map_t::iterator i = findNotExpired(aor);
  if (i == mDatabase.end() || i->second == 0)
  {
    return false;
  }
  return true;
}

void
InMemoryRegistrationDatabase::lockRecord(const Uri& aor)
{
  Lock g2(mLockedRecordsMutex);

  {
    Lock g1(mDatabaseMutex);
    // This forces insertion if the record does not yet exist.
    mDatabase[aor];
  }

  while (mLockedRecords.count(aor))
  {
    mRecordUnlocked.wait(mLockedRecordsMutex);
  }

  mLockedRecords.insert(aor);
}

void
InMemoryRegistrationDatabase::unlockRecord(const Uri& aor)
{
  Lock g2(mLockedRecordsMutex);

  {
    Lock g1(mDatabaseMutex);
    // If the pointer is null, we remove the record from the map.
    database_map_t::iterator i = mDatabase.find(aor);

    // The record must have been inserted when we locked it in the first place
    resip_assert (i != mDatabase.end());

    if (i->second == 0)
    {
      mDatabase.erase(i);
    }
  }

  mLockedRecords.erase(aor);
  mRecordUnlocked.broadcast();
}

RegistrationPersistenceManager::update_status_t 
InMemoryRegistrationDatabase::updateContact(const resip::Uri& aor, 
                                             const ContactInstanceRecord& rec) 
{
  ContactList *contactList = 0;

  {
    Lock g(mDatabaseMutex);

    database_map_t::iterator i;
    i = mDatabase.find(aor);
    if (i == mDatabase.end() || i->second == 0)
    {
      contactList = new ContactList();
      mDatabase[aor] = contactList;
    }
    else
    {
      contactList = i->second;
    }

  }

  resip_assert(contactList);

  ContactList::iterator j;

  // See if the contact is already present. We use URI matching rules here.
  for (j = contactList->begin(); j != contactList->end(); j++)
  {
    if (*j == rec)
    {
      *j=rec;
      return CONTACT_UPDATED;
    }
  }

  // This is a new contact, so we add it to the list.
  contactList->push_back(rec);
  return CONTACT_CREATED;
}

void 
InMemoryRegistrationDatabase::removeContact(const Uri& aor, 
                                             const ContactInstanceRecord& rec)
{
  ContactList *contactList = 0;

  {
    Lock g(mDatabaseMutex);

    database_map_t::iterator i;
    i = mDatabase.find(aor);
    if (i == mDatabase.end() || i->second == 0)
    {
      return;
    }
    contactList = i->second;
  }

  ContactList::iterator j;

  // See if the contact is present. We use URI matching rules here.
  for (j = contactList->begin(); j != contactList->end(); j++)
  {
    if (*j == rec)
    {
      contactList->erase(j);
      if (contactList->empty())
      {
        removeAor(aor);
      }
      return;
    }
  }
}

void
InMemoryRegistrationDatabase::getContacts(const Uri& aor, ContactList& container)
{
  Lock g(mDatabaseMutex);
  database_map_t::iterator i = findNotExpired(aor);
  if (i == mDatabase.end() || i->second == 0)
  {
      container.clear();
      return;
  }
   container = *(i->second);
}

class RemoveIfExpired
{
protected:
    UInt64 now;
public:
    RemoveIfExpired()
    {
       now = Timer::getTimeSecs();
    }
    bool operator () (const ContactInstanceRecord& rec)
    {
       return expired(rec);
    }
    bool expired(const ContactInstanceRecord& rec)
    {
      if(rec.mRegExpires <= now) 
      {
         DebugLog(<< "ContactInstanceRecord expired: " << rec.mContact);
         return true;
      }
      return false;
    }
};

bool expired(const ContactInstanceRecord& rec)
{
   RemoveIfExpired rei;
   return rei.expired(rec);
}

InMemoryRegistrationDatabase::database_map_t::iterator
InMemoryRegistrationDatabase::findNotExpired(const Uri& aor) 
{
   database_map_t::iterator i;
   i = mDatabase.find(aor);
   if (i == mDatabase.end() || i->second == 0) 
   {
      return i;
   }
   if(mCheckExpiry)
   {
      ContactList *contacts = i->second;
#ifdef __SUNPRO_CC
      contacts->remove_if(expired);
#else
      contacts->remove_if(RemoveIfExpired());
#endif
   }
   return i;
}

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
