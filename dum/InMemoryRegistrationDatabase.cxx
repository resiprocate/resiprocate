#include "dum/InMemoryRegistrationDatabase.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

InMemoryRegistrationDatabase::InMemoryRegistrationDatabase()
{
}

InMemoryRegistrationDatabase::~InMemoryRegistrationDatabase()
{
}

void 
InMemoryRegistrationDatabase::addAor(Uri &aor,
    RegistrationPersistenceManager::ContactPairList contacts)
{
  Lock g(mDatabaseMutex);
  mDatabase[aor] = new ContactPairList(contacts);
}

void 
InMemoryRegistrationDatabase::removeAor(Uri &aor)
{
  database_map_t::iterator i;

  Lock g(mDatabaseMutex);
  i = mDatabase.find(aor);
  if (i != mDatabase.end())
  {
    delete i->second;

    // Setting this to 0 causes it to be removed when we unlock the AOR.
    i->second = 0;
  }
}


InMemoryRegistrationDatabase::UriList 
InMemoryRegistrationDatabase::getAors()
{
   UriList retList;   
   for( database_map_t::const_iterator it = mDatabase.begin();
        it != mDatabase.end(); it++)
   {
      retList.push_back(it->first);
   }
   return retList;
}


bool 
InMemoryRegistrationDatabase::aorIsRegistered(Uri &aor)
{
  database_map_t::iterator i;

  Lock g(mDatabaseMutex);
  i = mDatabase.find(aor);
  if (i == mDatabase.end() || i->second == 0)
  {
    return false;
  }
  return true;
}

void
InMemoryRegistrationDatabase::lockRecord(Uri &aor)
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
InMemoryRegistrationDatabase::unlockRecord(Uri &aor)
{
  Lock g2(mLockedRecordsMutex);

  {
    Lock g1(mDatabaseMutex);
    // If the pointer is null, we remove the record from the map.
    database_map_t::iterator i = mDatabase.find(aor);

    // The record must have been inserted when we locked it in the first place
    assert (i != mDatabase.end());

    if (i->second == 0)
    {
      mDatabase.erase(i);
    }
  }

  mLockedRecords.erase(aor);
  mRecordUnlocked.broadcast();
}

RegistrationPersistenceManager::update_status_t 
InMemoryRegistrationDatabase::updateContact(Uri &aor, Uri &contact, time_t expires)
{
  ContactPairList *contactList = 0;

  {
    Lock g(mDatabaseMutex);

    database_map_t::iterator i;
    i = mDatabase.find(aor);
    if (i == mDatabase.end() || i->second == 0)
    {
      contactList = new ContactPairList();
      mDatabase[aor] = contactList;
    }
    else
    {
      contactList = i->second;
    }

  }

  assert(contactList);

  ContactPairList::iterator j;

  // See if the contact is already present. We use URI matching rules here.
  for (j = contactList->begin(); j != contactList->end(); j++)
  {
    if ((*j).first == contact)
    {
      (*j).first = contact;
      (*j).second = expires;
      return CONTACT_UPDATED;
    }
  }

  // This is a new contact, so we add it to the list.
  contactList->push_back(std::make_pair<Uri,time_t>(contact,expires));
  return CONTACT_CREATED;
}

void 
InMemoryRegistrationDatabase::removeContact(Uri &aor, Uri &contact)
{
  ContactPairList *contactList = 0;

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

  ContactPairList::iterator j;

  // See if the contact is present. We use URI matching rules here.
  for (j = contactList->begin(); j != contactList->end(); j++)
  {
    if ((*j).first == contact)
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

RegistrationPersistenceManager::ContactPairList
InMemoryRegistrationDatabase::getContacts(Uri &aor)
{
  Lock g(mDatabaseMutex);

  database_map_t::iterator i;
  i = mDatabase.find(aor);
  if (i == mDatabase.end() || i->second == 0)
  {
    return ContactPairList();
  }
  return *(i->second);
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
