#include "precompile.h"
#include <ctime>
#include "resip/dum/InMemoryRegistrationDatabase.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "rutil/Logger.hxx"

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
    RegistrationPersistenceManager::ContactRecordList contacts)
{
  Lock g(mDatabaseMutex);
  mDatabase[aor] = new ContactRecordList(contacts);
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


InMemoryRegistrationDatabase::UriList 
InMemoryRegistrationDatabase::getAors()
{
   UriList retList;   
   getAors(retList);
   return retList;
}

void
InMemoryRegistrationDatabase::getAors(InMemoryRegistrationDatabase::UriList& container)
{
   container.clear();
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
InMemoryRegistrationDatabase::updateContact(const Uri& aor, 
                                             const Uri& contact, 
                                             time_t expires,
                                             unsigned int cid,
                                             short q)
{
  ContactRecordList *contactList = 0;

  {
    Lock g(mDatabaseMutex);

    database_map_t::iterator i;
    i = mDatabase.find(aor);
    if (i == mDatabase.end() || i->second == 0)
    {
      contactList = new ContactRecordList();
      mDatabase[aor] = contactList;
    }
    else
    {
      contactList = i->second;
    }

  }

  assert(contactList);

  ContactRecordList::iterator j;

  // See if the contact is already present. We use URI matching rules here.
  for (j = contactList->begin(); j != contactList->end(); j++)
  {
    if ((*j).uri == contact)
    {
      (*j).uri = contact;
      (*j).expires = expires;

      if(q>=0)
      {
         (*j).useQ=true;
         (*j).q=(unsigned short)q;
      }
      else
      {
         (*j).useQ=false;
         (*j).q=0;
      }

      (*j).cid=cid;
      return CONTACT_UPDATED;
    }
  }

   ContactRecord newRec;
   newRec.uri=contact;
   newRec.expires=expires;

   if(q>=0)
   {
      newRec.useQ=true;
      newRec.q=(unsigned short)q;
   }
   else
   {
      newRec.useQ=false;
      newRec.q=0;
   }

   newRec.cid=cid;
   
  // This is a new contact, so we add it to the list.
  contactList->push_back(newRec);
  return CONTACT_CREATED;
}

void 
InMemoryRegistrationDatabase::removeContact(const Uri& aor, const Uri& contact)
{
  ContactRecordList *contactList = 0;

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

  ContactRecordList::iterator j;

  // See if the contact is present. We use URI matching rules here.
  for (j = contactList->begin(); j != contactList->end(); j++)
  {
    if ((*j).uri == contact)
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

RegistrationPersistenceManager::ContactRecordList
InMemoryRegistrationDatabase::getContacts(const Uri& aor)
{
   ContactRecordList result;
  Lock g(mDatabaseMutex);
  getContacts(aor,result);
  return result;
   
}

void
InMemoryRegistrationDatabase::getContacts(const Uri& aor,RegistrationPersistenceManager::ContactRecordList& container)
{
  database_map_t::iterator i = findNotExpired(aor);
  if (i == mDatabase.end() || i->second == 0)
  {
      return;
  }
  container= *(i->second);

}

class RemoveIfExpired
{
protected:
    time_t now;
public:
    RemoveIfExpired()
    {
       time(&now);
    }
    bool operator () (RegistrationPersistenceManager::ContactRecord rec)
    {
        if(rec.expires < now) 
        {
		DebugLog(<< "ContactRecord expired: " << rec.uri);
		return true;
	}
        return false;
    }
};

InMemoryRegistrationDatabase::database_map_t::iterator
InMemoryRegistrationDatabase::findNotExpired(const Uri& aor) {
   database_map_t::iterator i;
   i = mDatabase.find(aor);
   if (i == mDatabase.end() || i->second == 0) 
   {
      return i;
   }
   if(mCheckExpiry)
   {
      ContactRecordList *contacts = i->second;
      contacts->remove_if(RemoveIfExpired());
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
