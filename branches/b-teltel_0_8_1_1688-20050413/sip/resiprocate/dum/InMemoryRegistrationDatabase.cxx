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
